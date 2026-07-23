#include "engine.h"
#include "amortmodel.h"
#include "pdfreport.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

#include <cmath>

#ifdef Q_OS_WASM
#include <emscripten/val.h>   // synchronous access to the browser's localStorage
#endif

Engine::Engine(QObject* parent)
    : QObject(parent)
{
    m_bank = new AmortModel(QStringLiteral("Amortización préstamo bancario"), this);
    m_coop  = new AmortModel(QStringLiteral("Amortización cooperativa"), this);
    m_family  = new AmortModel(QStringLiteral("Amortización préstamo familiar"), this);
    m_properties  = new AmortModel(QStringLiteral("Amortización propiedades"), this);
    registerInputs();
    resolveDataPath();
    loadFromDisk();
    recalc();
}

// ---------------------------------------------------------------- persistence
void Engine::resolveDataPath()
{
#ifdef Q_OS_WASM
    // No filesystem in the browser: localStorage is used instead.
    m_dataPath = QStringLiteral("almacenamiento del navegador (localStorage)");
#else
    // Prefer a JSON file next to the executable (portable use). If that
    // folder isn't writable (e.g. Program Files), use AppData instead.
    const QString besideExe = QCoreApplication::applicationDirPath()
                        + QStringLiteral("/farmaciasim_data.json");
    QFile probe(besideExe);
    if (probe.open(QIODevice::ReadWrite)) {   // doesn't truncate; creates if missing
        probe.close();
        if (probe.size() == 0)
            QFile::remove(besideExe);             // don't leave an empty file
        m_dataPath = besideExe;
        return;
    }
    const QString appData =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    m_dataPath = appData + QStringLiteral("/farmaciasim_data.json");
#endif
}

void Engine::saveToDisk() const
{
    QJsonObject o;
    for (auto it = m_dbl.cbegin(); it != m_dbl.cend(); ++it)
        o[it.key()] = *it.value();
    for (auto it = m_int.cbegin(); it != m_int.cend(); ++it)
        o[it.key()] = *it.value();
    o["comparisonScenarios"] = QJsonArray::fromVariantList(m_comparisonScenarios);
    o["pdfSaveDir"] = m_pdfSaveDir;

#ifdef Q_OS_WASM
    const QByteArray json = QJsonDocument(o).toJson(QJsonDocument::Compact);
    emscripten::val::global("localStorage")
        .call<void>("setItem", std::string("farmaciasim_data"),
                    json.toStdString());
#else
    QSaveFile f(m_dataPath);                 // atomic write
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("No se pudo guardar en '%s'", qPrintable(m_dataPath));
        return;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.commit();
#endif
}

// Maps every pre-"Change to English" (Spanish) JSON key to its current
// English key, so sessions saved before that rename still load correctly.
// Covers the flat sim::Inputs keys, the top-level scenarios key and the
// financing-comparison sub-keys (nombre/proyeccion/financiacion are handled
// separately in translateLegacyObject()).
static const QHash<QString, QString>& legacyKeyMap()
{
    static const QHash<QString, QString> map = [] {
        QHash<QString, QString> m;
        m[QStringLiteral("ventaReceta")]    = QStringLiteral("prescriptionSales");
        m[QStringLiteral("ventaLibre")]     = QStringLiteral("otcSales");
        m[QStringLiteral("margenPct")]      = QStringLiteral("marginPct");
        m[QStringLiteral("alquilerLocal")]  = QStringLiteral("premisesRent");
        m[QStringLiteral("suministros")]    = QStringLiteral("utilities");
        m[QStringLiteral("asesoria")]       = QStringLiteral("advisoryFees");
        m[QStringLiteral("mantenimiento")]  = QStringLiteral("maintenance");
        m[QStringLiteral("seguros")]        = QStringLiteral("insurance");
        m[QStringLiteral("otrosGastos")]    = QStringLiteral("otherExpenses");
        m[QStringLiteral("escenarioCrecimiento")] = QStringLiteral("growthScenario");
        m[QStringLiteral("ipcOptimista")]         = QStringLiteral("ipcOptimistic");
        m[QStringLiteral("margenOptimistaAnio1")] = QStringLiteral("optimisticMarginYear1");
        m[QStringLiteral("margenOptimistaAnio2")] = QStringLiteral("optimisticMarginYear2");
        m[QStringLiteral("margenOptimistaAnio3")] = QStringLiteral("optimisticMarginYear3");
        m[QStringLiteral("coeficiente")]    = QStringLiteral("goodwillMultiple");
        m[QStringLiteral("localComercial")] = QStringLiteral("premisesPrice");
        m[QStringLiteral("existencias")]    = QStringLiteral("inventory");
        m[QStringLiteral("notario")]        = QStringLiteral("notaryFees");
        m[QStringLiteral("registro")]       = QStringLiteral("registryFees");
        m[QStringLiteral("gastosVarios")]   = QStringLiteral("miscExpenses");
        m[QStringLiteral("honorariosPct")]  = QStringLiteral("feesPct");
        m[QStringLiteral("tipoBanco")]      = QStringLiteral("bankRate");
        m[QStringLiteral("tipoCoop")]       = QStringLiteral("coopRate");
        m[QStringLiteral("tipoFamiliar")]   = QStringLiteral("familyRate");
        m[QStringLiteral("tipoPropiedades")] = QStringLiteral("propertiesRate");
        m[QStringLiteral("plazoBanco")]       = QStringLiteral("bankTermYears");
        m[QStringLiteral("plazoCoop")]        = QStringLiteral("coopTermYears");
        m[QStringLiteral("plazoFamiliar")]    = QStringLiteral("familyTermYears");
        m[QStringLiteral("plazoPropiedades")] = QStringLiteral("propertiesTermYears");
        m[QStringLiteral("pctFinFarmacia")]      = QStringLiteral("pharmacyFinancingPct");
        m[QStringLiteral("pctFinLocal")]         = QStringLiteral("premisesFinancingPct");
        m[QStringLiteral("pctAperturaHipoteca")] = QStringLiteral("mortgageOpeningPct");
        m[QStringLiteral("pctFinPropiedades")]   = QStringLiteral("propertiesFinancingPct");
        m[QStringLiteral("liquidezAportada")]    = QStringLiteral("contributedCash");
        m[QStringLiteral("aportacionFamiliar")]  = QStringLiteral("familyContribution");
        m[QStringLiteral("finPropiedades")]      = QStringLiteral("propertiesFinancing");
        m[QStringLiteral("excesoAportacion")]    = QStringLiteral("contributionExcess");
        m[QStringLiteral("pedidoInicial")]       = QStringLiteral("initialOrder");
        m[QStringLiteral("salFarmaceutico")]  = QStringLiteral("pharmacistSalary");
        m[QStringLiteral("jornFarmaceutico")] = QStringLiteral("pharmacistFte");
        m[QStringLiteral("pctSS")]            = QStringLiteral("socialSecurityPct");
        m[QStringLiteral("subidaPct")]        = QStringLiteral("raisePct");
        m[QStringLiteral("salAuxiliar")]      = QStringLiteral("assistantSalary");
        m[QStringLiteral("jornAuxiliar")]     = QStringLiteral("assistantFte");
        m[QStringLiteral("salTecnico")]       = QStringLiteral("technicianSalary");
        m[QStringLiteral("jornTecnico")]      = QStringLiteral("technicianFte");
        for (int k = 0; k < 4; ++k) {
            m[QStringLiteral("plJornada%1").arg(k)]  = QStringLiteral("staffFte%1").arg(k);
            m[QStringLiteral("plPersonas%1").arg(k)] = QStringLiteral("staffCount%1").arg(k);
        }
        for (int k = 0; k < 3; ++k) {
            m[QStringLiteral("factorVenta%1").arg(k)]    = QStringLiteral("saleFactor%1").arg(k);
            m[QStringLiteral("impuestosVenta%1").arg(k)] = QStringLiteral("saleTaxes%1").arg(k);
        }
        m[QStringLiteral("fdcInicialSim")]    = QStringLiteral("fdcInitialSim");
        m[QStringLiteral("pctMaxFdC")]        = QStringLiteral("fdcMaxPct");
        m[QStringLiteral("pctAmortLocal")]    = QStringLiteral("investmentPremisesDeprPct");
        m[QStringLiteral("pctExistencias10")] = QStringLiteral("inventoryPctYear10");
        m[QStringLiteral("impAmortLocalPct")] = QStringLiteral("taxPremisesDeprPct");
        m[QStringLiteral("impAmortMinPct")]   = QStringLiteral("taxMinGoodwillDeprPct");
        m[QStringLiteral("impAmortMaxPct")]   = QStringLiteral("taxMaxGoodwillDeprPct");
        m[QStringLiteral("minimoPersonal")]   = QStringLiteral("personalAllowance");
        for (int k = 0; k < 6; ++k) {
            m[QStringLiteral("irpfDesde%1").arg(k)] = QStringLiteral("irpfFrom%1").arg(k);
            m[QStringLiteral("irpfHasta%1").arg(k)] = QStringLiteral("irpfTo%1").arg(k);
            m[QStringLiteral("irpfTipo%1").arg(k)]  = QStringLiteral("irpfRate%1").arg(k);
        }
        for (int k = 0; k < 9; ++k)
            m[QStringLiteral("rdDesde%1").arg(k)] = QStringLiteral("rdFrom%1").arg(k);
        for (int k = 0; k < 15; ++k) {
            m[QStringLiteral("retaDesde%1").arg(k)] = QStringLiteral("retaFrom%1").arg(k);
            m[QStringLiteral("retaCuota%1").arg(k)] = QStringLiteral("retaQuota%1").arg(k);
        }
        m[QStringLiteral("tarifaPlanaMensual")] = QStringLiteral("retaFlatMonthlyFee");
        for (int k = 0; k < 10; ++k) {
            m[QStringLiteral("ipcHistorico%1").arg(k)]       = QStringLiteral("ipcHistorical%1").arg(k);
            m[QStringLiteral("margenComercialSim%1").arg(k)] = QStringLiteral("realisticMarginSeries%1").arg(k);
        }
        // Financing-comparison sub-keys (not otherwise reachable from the flat map above).
        m[QStringLiteral("finBancariaFarmacia")] = QStringLiteral("pharmacyBankFinancing");
        m[QStringLiteral("finBancariaLocal")]    = QStringLiteral("premisesBankFinancing");
        // Top-level scenarios array.
        m[QStringLiteral("escenariosComparacion")] = QStringLiteral("comparisonScenarios");
        return m;
    }();
    return map;
}

// Translates a loaded JSON object's keys from the pre-rename Spanish scheme
// to the current English one. Keys already in English pass through
// unchanged, so this is safe to call unconditionally on any saved session.
static QJsonObject translateLegacyObject(const QJsonObject& in)
{
    static const QHash<QString, QString> scenarioKeyMap{
        { QStringLiteral("nombre"),       QStringLiteral("name") },
        { QStringLiteral("proyeccion"),   QStringLiteral("projection") },
        { QStringLiteral("financiacion"), QStringLiteral("financing") },
    };
    const auto& map = legacyKeyMap();
    QJsonObject out;
    for (auto it = in.constBegin(); it != in.constEnd(); ++it) {
        const QString newKey = map.value(it.key(), it.key());
        if (newKey == QStringLiteral("comparisonScenarios") && it.value().isArray()) {
            QJsonArray scenarios;
            for (const QJsonValue& sv : it.value().toArray()) {
                if (!sv.isObject()) { scenarios.append(sv); continue; }
                QJsonObject scenario;
                const QJsonObject so = sv.toObject();
                for (auto sit = so.constBegin(); sit != so.constEnd(); ++sit) {
                    const QString sKey = scenarioKeyMap.value(sit.key(), sit.key());
                    if (sKey == QStringLiteral("financing") && sit.value().isObject()) {
                        QJsonObject fin;
                        const QJsonObject fo = sit.value().toObject();
                        for (auto fit = fo.constBegin(); fit != fo.constEnd(); ++fit)
                            fin[map.value(fit.key(), fit.key())] = fit.value();
                        scenario[sKey] = fin;
                    } else {
                        scenario[sKey] = sit.value();
                    }
                }
                scenarios.append(scenario);
            }
            out[newKey] = scenarios;
        } else {
            out[newKey] = it.value();
        }
    }
    return out;
}

// Migrates pre-per-employee "staffFteN" (one % applied to the whole
// headcount of role N) into the new "staffFteN_J" per-employee keys, so
// sessions saved before this change keep the same total jornada until the
// user starts customizing individual employees.
static QJsonObject expandLegacyStaffFte(QJsonObject o)
{
    for (int k = 0; k < 4; ++k) {
        const QString oldKey = QStringLiteral("staffFte%1").arg(k);
        const auto it = o.constFind(oldKey);
        if (it == o.constEnd() || !it->isDouble())
            continue;
        const double v = it->toDouble();
        o.remove(oldKey);
        for (int j = 0; j < sim::kMaxStaffPerRole; ++j)
            o[QStringLiteral("staffFte%1_%2").arg(k).arg(j)] = v;
    }
    return o;
}

// Migrates the pre-per-role "raisePct" (one % shared by every headcount
// role) into "raisePct1"/"raisePct2"/"raisePct3" (role 0, the Owner, never
// carried the raise, so it's left at its default).
static QJsonObject expandLegacyRaisePct(QJsonObject o)
{
    const auto it = o.constFind(QStringLiteral("raisePct"));
    if (it != o.constEnd() && it->isDouble()) {
        const double v = it->toDouble();
        o.remove(QStringLiteral("raisePct"));
        for (int k = 1; k < 4; ++k)
            o[QStringLiteral("raisePct%1").arg(k)] = v;
    }
    return o;
}

void Engine::loadFromDisk()
{
    QByteArray data;
#ifdef Q_OS_WASM
    emscripten::val v = emscripten::val::global("localStorage")
        .call<emscripten::val>("getItem", std::string("farmaciasim_data"));
    if (v.isNull() || v.isUndefined()) {
        // Fall back to the pre-rename localStorage key (Spanish field names).
        v = emscripten::val::global("localStorage")
            .call<emscripten::val>("getItem", std::string("farmaciasim_datos"));
        if (v.isNull() || v.isUndefined())
            return;                            // first run: defaults
    }
    data = QByteArray::fromStdString(v.as<std::string>());
#else
    QFile f(m_dataPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        // Fall back to the pre-rename file name (Spanish field names).
        const QString legacyPath = QFileInfo(m_dataPath).absolutePath()
                                  + QStringLiteral("/farmaciasim_datos.json");
        QFile legacy(legacyPath);
        if (!legacy.exists() || !legacy.open(QIODevice::ReadOnly))
            return;                            // first run: defaults
        data = legacy.readAll();
    } else {
        data = f.readAll();
    }
#endif
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;
    const QJsonObject o = expandLegacyRaisePct(expandLegacyStaffFte(translateLegacyObject(doc.object())));
    for (auto it = o.constBegin(); it != o.constEnd(); ++it) {
        if (!it.value().isDouble())
            continue;
        if (auto d = m_dbl.find(it.key()); d != m_dbl.end())
            **d = it.value().toDouble();
        else if (auto n = m_int.find(it.key()); n != m_int.end()) {
            const int v = int(it.value().toDouble());
            if (v > 0) **n = v;
        }
    }
    m_in.staffCount[0] = 1; // Owner pharmacist: always 1 person, not editable

    if (const auto it = o.constFind(QStringLiteral("comparisonScenarios"));
        it != o.constEnd() && it->isArray()) {
        m_comparisonScenarios = it->toArray().toVariantList();
    }

    if (const auto it = o.constFind(QStringLiteral("pdfSaveDir"));
        it != o.constEnd() && it->isString()) {
        m_pdfSaveDir = it->toString();
    }
}

// Binds every input key to its field in 'i'. Shared by registerInputs()
// (binds to the live m_in) and resetKeysToDefaults() (binds to a throwaway
// default-constructed Inputs, to read the factory value of each key).
static void bindInputMaps(sim::Inputs& i, QHash<QString, double*>& dbl, QHash<QString, int*>& intMap)
{
    // ---- Datos Base
    dbl["prescriptionSales"] = &i.prescriptionSales;
    dbl["otcSales"]          = &i.otcSales;
    dbl["marginPct"]         = &i.marginPct;
    dbl["premisesRent"]      = &i.premisesRent;
    dbl["utilities"]         = &i.utilities;
    dbl["advisoryFees"]      = &i.advisoryFees;
    dbl["maintenance"]       = &i.maintenance;
    dbl["robot"]             = &i.robot;
    dbl["insurance"]         = &i.insurance;
    dbl["otherExpenses"]     = &i.otherExpenses;
    // ---- Financiación: growth scenario
    dbl["growthScenario"]       = &i.growthScenario;
    dbl["ipcOptimistic"]        = &i.ipcOptimistic;
    dbl["salaryRaisePct"]       = &i.salaryRaisePct;
    dbl["optimisticMarginYear1"] = &i.optimisticMarginYear1;
    dbl["optimisticMarginYear2"] = &i.optimisticMarginYear2;
    dbl["optimisticMarginYear3"] = &i.optimisticMarginYear3;
    // ---- Financiación: investment, rates, contributions
    dbl["goodwillMultiple"]  = &i.goodwillMultiple;
    dbl["premisesPrice"]     = &i.premisesPrice;
    dbl["inventory"]         = &i.inventory;
    dbl["notaryFees"]        = &i.notaryFees;
    dbl["registryFees"]      = &i.registryFees;
    dbl["miscExpenses"]      = &i.miscExpenses;
    dbl["feesPct"]           = &i.feesPct;
    dbl["ivaPct"]            = &i.ivaPct;
    dbl["itpPct"]            = &i.itpPct;
    dbl["ajdPct"]            = &i.ajdPct;
    dbl["bankRate"]          = &i.bankRate;
    dbl["coopRate"]          = &i.coopRate;
    dbl["familyRate"]        = &i.familyRate;
    dbl["propertiesRate"]    = &i.propertiesRate;
    intMap["bankTermYears"]       = &i.bankTermYears;
    intMap["coopTermYears"]       = &i.coopTermYears;
    intMap["familyTermYears"]     = &i.familyTermYears;
    intMap["propertiesTermYears"] = &i.propertiesTermYears;
    dbl["familyGraceMonths"]   = &i.familyGraceMonths;
    dbl["pharmacyFinancingPct"]  = &i.pharmacyFinancingPct;
    dbl["premisesFinancingPct"]  = &i.premisesFinancingPct;
    dbl["mortgageOpeningPct"]    = &i.mortgageOpeningPct;
    dbl["propertiesFinancingPct"] = &i.propertiesFinancingPct;
    dbl["contributedCash"]     = &i.contributedCash;
    dbl["familyContribution"]  = &i.familyContribution;
    dbl["propertiesFinancing"] = &i.propertiesFinancing;
    dbl["contributionExcess"]  = &i.contributionExcess;
    dbl["initialOrder"]        = &i.initialOrder;
    // ---- Personal
    dbl["pharmacistSalary"]  = &i.pharmacistSalary;
    dbl["pharmacistFte"]     = &i.pharmacistFte;
    dbl["socialSecurityPct"] = &i.socialSecurityPct;
    dbl["assistantSalary"]   = &i.assistantSalary;
    dbl["assistantFte"]      = &i.assistantFte;
    dbl["technicianSalary"]  = &i.technicianSalary;
    dbl["technicianFte"]     = &i.technicianFte;
    for (int k = 0; k < 4; ++k) {
        dbl[QStringLiteral("staffCount%1").arg(k)] = &i.staffCount[k];
        dbl[QStringLiteral("raisePct%1").arg(k)]   = &i.raisePct[k];
        for (int j = 0; j < sim::kMaxStaffPerRole; ++j) {
            dbl[QStringLiteral("staffFte%1_%2").arg(k).arg(j)] = &i.staffFteEach[k][j];
            intMap[QStringLiteral("staffHireYear%1_%2").arg(k).arg(j)] = &i.staffHireYearEach[k][j];
        }
    }
    for (int k = 0; k < 3; ++k) {
        dbl[QStringLiteral("vacationStaffCount%1").arg(k)] = &i.vacationStaffCount[k];
        dbl[QStringLiteral("vacationRaisePct%1").arg(k)]   = &i.vacationRaisePct[k];
        for (int j = 0; j < sim::kMaxStaffPerRole; ++j) {
            dbl[QStringLiteral("vacationStaffFte%1_%2").arg(k).arg(j)]    = &i.vacationStaffFteEach[k][j];
            dbl[QStringLiteral("vacationStaffMonths%1_%2").arg(k).arg(j)] = &i.vacationStaffMonthsEach[k][j];
        }
    }
    for (int d = 0; d < 7; ++d) {
        dbl[QStringLiteral("scheduleOpen%1").arg(d)]  = &i.schedule[d].openHour;
        dbl[QStringLiteral("scheduleClose%1").arg(d)] = &i.schedule[d].closeHour;
    }
    // ---- Análisis
    for (int k = 0; k < 3; ++k) {
        dbl[QStringLiteral("saleFactor%1").arg(k)] = &i.saleFactor[k];
        dbl[QStringLiteral("saleTaxes%1").arg(k)]  = &i.saleTaxes[k];
    }
    dbl["fdcInitialSim"]     = &i.fdcInitialSim;
    dbl["fdcMaxPct"]         = &i.fdcMaxPct;
    dbl["investmentPremisesDeprPct"] = &i.investmentPremisesDeprPct;
    dbl["inventoryPctYear10"]  = &i.inventoryPctYear10;
    // ---- Hoja Impuestos (v2)
    dbl["taxPremisesDeprPct"]    = &i.taxPremisesDeprPct;
    dbl["taxMinGoodwillDeprPct"] = &i.taxMinGoodwillDeprPct;
    dbl["taxMaxGoodwillDeprPct"] = &i.taxMaxGoodwillDeprPct;
    dbl["personalAllowance"]     = &i.personalAllowance;
    // ---- Configuración: official scales and series (editable)
    for (int k = 0; k < 6; ++k) {
        dbl[QStringLiteral("irpfFrom%1").arg(k)] = &i.irpfBrackets[k].from;
        dbl[QStringLiteral("irpfTo%1").arg(k)]   = &i.irpfBrackets[k].to;
        dbl[QStringLiteral("irpfRate%1").arg(k)] = &i.irpfBrackets[k].rate;
    }
    for (int k = 0; k < 9; ++k) {
        dbl[QStringLiteral("rdFrom%1").arg(k)] = &i.rdBrackets[k].from;
        dbl[QStringLiteral("rdBase%1").arg(k)] = &i.rdBrackets[k].base;
        dbl[QStringLiteral("rdPct%1").arg(k)]  = &i.rdBrackets[k].pct;
    }
    for (int k = 0; k < 15; ++k) {
        dbl[QStringLiteral("retaFrom%1").arg(k)]  = &i.retaBrackets[k].from;
        dbl[QStringLiteral("retaQuota%1").arg(k)] = &i.retaBrackets[k].monthlyQuota;
    }
    dbl["retaFlatMonthlyFee"] = &i.retaFlatMonthlyFee;
    for (int k = 0; k < 10; ++k) {
        dbl[QStringLiteral("ipcHistorical%1").arg(k)]        = &i.ipcHistorical[k];
        dbl[QStringLiteral("realisticMarginSeries%1").arg(k)] = &i.realisticMarginSeries[k];
    }
    // ---- Hoja Simulación
    dbl["simulationRevenueDeltaEur"] = &i.simulationRevenueDeltaEur;
    dbl["simulationTermDeltaYears"]  = &i.simulationTermDeltaYears;
    dbl["simulationRateDeltaPct"]    = &i.simulationRateDeltaPct;
    dbl["simulationCashDeltaEur"]    = &i.simulationCashDeltaEur;
}

void Engine::registerInputs()
{
    bindInputMaps(m_in, m_dbl, m_int);
}

void Engine::set(const QString& key, double value)
{
    if (key == QStringLiteral("staffCount0"))
        return; // Owner pharmacist: always 1 person, not editable

    if (auto it = m_dbl.find(key); it != m_dbl.end()) {
        if (**it == value) return;
        **it = value;
    } else if (auto ii = m_int.find(key); ii != m_int.end()) {
        const int v = int(value);
        if (**ii == v || v <= 0) return;
        **ii = v;
    } else {
        qWarning("Engine::set: clave desconocida '%s'", qPrintable(key));
        return;
    }
    recalc();
}

void Engine::resetToDefaults()
{
    m_in = sim::Inputs{};
    recalc();
}

// Restores just the given keys to their factory value, leaving every other
// input untouched (unlike resetToDefaults(), which resets everything). Used
// by the per-group "Restaurar" buttons in Configuración.
void Engine::resetKeysToDefaults(const QStringList& keys)
{
    sim::Inputs def{};
    QHash<QString, double*> defDbl;
    QHash<QString, int*> defInt;
    bindInputMaps(def, defDbl, defInt);

    bool changed = false;
    for (const QString& key : keys) {
        if (auto it = defDbl.find(key); it != defDbl.end()) {
            auto mine = m_dbl.find(key);
            if (mine != m_dbl.end() && **mine != **it) {
                **mine = **it;
                changed = true;
            }
        } else if (auto it = defInt.find(key); it != defInt.end()) {
            auto mine = m_int.find(key);
            if (mine != m_int.end() && **mine != **it) {
                **mine = **it;
                changed = true;
            }
        }
    }
    if (changed) recalc();
}

// ---------------------------------------------------------------- export PDF
#ifdef Q_OS_WASM
// Browser download: Blob + <a download>, without needing QtWidgets.
static void downloadInBrowser(const QByteArray& data, const QString& fileName)
{
    using emscripten::val;
    val view{ emscripten::typed_memory_view(
        size_t(data.size()), reinterpret_cast<const uint8_t*>(data.constData())) };
    val arr = val::global("Uint8Array").new_(data.size());
    arr.call<void>("set", view);

    val parts = val::array();
    parts.call<void>("push", arr);
    val options = val::object();
    options.set("type", std::string("application/pdf"));
    val blob = val::global("Blob").new_(parts, options);

    val url = val::global("URL").call<val>("createObjectURL", blob);
    val a = val::global("document").call<val>("createElement", std::string("a"));
    a.set("href", url);
    a.set("download", fileName.toStdString());
    a.call<void>("click");
    val::global("URL").call<void>("revokeObjectURL", url);
}
#endif

// Saves/downloads the bytes of an already-generated PDF. Desktop: saves it to
// 'customDir' (or Documents if empty/not usable) with the given prefix and
// opens it; WASM: the browser downloads it, ignoring 'customDir'. Returns the
// path (or "descargas del navegador" on WASM), empty on failure.
static QString saveOrDownloadPdf(const QByteArray& data, const QString& filenamePrefix, const QString& customDir)
{
    const QString fileName = filenamePrefix + QLatin1Char('_')
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmm")) + QStringLiteral(".pdf");

#ifdef Q_OS_WASM
    Q_UNUSED(customDir);
    downloadInBrowser(data, fileName);
    return QStringLiteral("descargas del navegador");
#else
    QString dir = customDir.trimmed();
    if (!dir.isEmpty())
        QDir().mkpath(dir);      // create it if it doesn't exist yet
    if (dir.isEmpty() || !QFileInfo(dir).isDir()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (dir.isEmpty())
            dir = QDir::homePath();
    }
    const QString path = dir + QLatin1Char('/') + fileName;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("No se pudo escribir el PDF en '%s'", qPrintable(path));
        return {};
    }
    f.write(data);
    f.close();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));   // open it right away
    return path;
#endif
}

QString Engine::pdfSaveDirDefault() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    return dir;
}

void Engine::setPdfSaveDir(const QUrl& dir)
{
    const QString path = dir.isLocalFile() ? dir.toLocalFile() : QString();
    if (path == m_pdfSaveDir)
        return;
    m_pdfSaveDir = path;
    emit pdfSaveDirChanged();
    saveToDisk();
}

QUrl Engine::pdfSaveDirUrl() const
{
    return QUrl::fromLocalFile(m_pdfSaveDir.isEmpty() ? pdfSaveDirDefault() : m_pdfSaveDir);
}

QString Engine::exportPdf()
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    if (!pdf::writeReport(&buf, m_in, m_r))
        return {};
    buf.close();

    return saveOrDownloadPdf(buf.data(), QStringLiteral("FarmaciaSim_Informe"), m_pdfSaveDir);
}

// Same as ComparacionView.qml (tableRows), but always in "full view": the
// "Financiación" group is interleaved after "Venta total" regardless of the
// UI's toggle switch.
static QVariantList fullComparisonRows(const Engine& engine, int year)
{
    const QVariantList rows = engine.comparisonForYear(year);
    const QVariantList financingRows = engine.financingComparison();

    QVariantList group;
    group << QVariantMap{ { "label", QStringLiteral("Financiación") }, { "separator", true } };
    for (int i = 0; i < financingRows.size(); ++i) {
        QVariantMap f = financingRows[i].toMap();
        f[QStringLiteral("group")] = true;
        f[QStringLiteral("groupEnd")] = (i == financingRows.size() - 1);
        group << f;
    }

    int idx = -1;
    for (int i = 0; i < rows.size(); ++i) {
        if (rows[i].toMap().value(QStringLiteral("label")).toString() == QStringLiteral("Venta total")) {
            idx = i;
            break;
        }
    }
    return idx < 0 ? rows + group : rows.mid(0, idx + 1) + group + rows.mid(idx + 1);
}

QString Engine::exportComparisonPdf(int year)
{
    if (m_comparisonScenarios.isEmpty())
        return {};

    QStringList names;
    for (const QVariant& ev : m_comparisonScenarios)
        names << ev.toMap().value(QStringLiteral("name")).toString();

    QVariantList pages;
    if (year < 0) {
        for (int a = 0; a < 10; ++a)
            pages << QVariantMap{ { "year", a + 1 }, { "rows", fullComparisonRows(*this, a) } };
    } else {
        pages << QVariantMap{ { "year", year + 1 }, { "rows", fullComparisonRows(*this, year) } };
    }

    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    if (!pdf::writeComparison(&buf, pages, names))
        return {};
    buf.close();

    return saveOrDownloadPdf(buf.data(), QStringLiteral("FarmaciaSim_Comparacion"), m_pdfSaveDir);
}

QString Engine::exportSimulationPdf(const QVariantList& hiddenColumnsPerGroup)
{
    // Índices de columna (combinación) ocultos ("ojo"), por grupo (mismo
    // orden que devuelve simulationForYear(): un elemento por Facturación
    // Total). Si se reciben menos grupos de los que hay, el resto se trata
    // como "nada oculto" en ese grupo.
    QVector<QSet<int>> hiddenPerGroup;
    for (const QVariant& gv : hiddenColumnsPerGroup) {
        QSet<int> hidden;
        for (const QVariant& v : gv.toList())
            hidden.insert(v.toInt());
        hiddenPerGroup.push_back(hidden);
    }

    const QVariantList sample = simulationForYear(0);
    while (hiddenPerGroup.size() < sample.size())
        hiddenPerGroup.push_back({});

    // Nº total de combinaciones (columnas), igual en todo grupo/año: se lee
    // de las filas del primer grupo del año 1.
    int totalCombinaciones = 0;
    if (!sample.isEmpty()) {
        const QVariantList rows = sample.first().toMap().value(QStringLiteral("rows")).toList();
        if (!rows.isEmpty())
            totalCombinaciones = rows.first().toMap().value(QStringLiteral("values")).toList().size();
    }

    // Columnas de la tabla fusionada: cada combinación agrupa, una junto a
    // otra, las columnas de los grupos (Facturación Total) donde esa
    // combinación no está oculta — mismo plazo/interés/aportación en todas
    // las columnas de un mismo grupo de combinación, ver la fila
    // "Facturación" añadida más abajo para distinguirlas.
    struct ColumnRef { int combo; int group; };
    QVector<ColumnRef> columns;
    for (int c = 0; c < totalCombinaciones; ++c)
        for (int g = 0; g < sample.size(); ++g)
            if (!hiddenPerGroup[g].contains(c))
                columns.push_back({ c, g });

    QStringList combinacionLabels;
    for (const ColumnRef& col : columns)
        combinacionLabels << QStringLiteral("Combinación %1").arg(col.combo + 1);

    QVariantList years;
    for (int a = 0; a < 10; ++a) {
        const QVariantList groups = simulationForYear(a);

        QVariantList facturacionValues;
        for (const ColumnRef& col : columns)
            facturacionValues << groups.value(col.group).toMap().value(QStringLiteral("facturacion"));

        QVariantList mergedRows{
            QVariantMap{ { "label", QStringLiteral("Facturación") },
                         { "values", facturacionValues }, { "fmt", QStringLiteral("eur") }, { "bold", false } },
        };

        const QVariantList templateRows = groups.isEmpty() ? QVariantList()
                                            : groups.first().toMap().value(QStringLiteral("rows")).toList();
        for (const QVariant& trv : templateRows) {
            const QVariantMap templateRow = trv.toMap();
            QVariantList values;
            for (const ColumnRef& col : columns) {
                const QVariantList groupRows = groups.value(col.group).toMap()
                                                    .value(QStringLiteral("rows")).toList();
                // Las filas están en el mismo orden en simulationForYear(): se
                // busca por etiqueta en vez de por índice para no depender de
                // ese orden coincidiendo entre grupos.
                QVariantList rowValues;
                for (const QVariant& rv : groupRows) {
                    const QVariantMap row = rv.toMap();
                    if (row.value(QStringLiteral("label")) == templateRow.value(QStringLiteral("label"))) {
                        rowValues = row.value(QStringLiteral("values")).toList();
                        break;
                    }
                }
                values << rowValues.value(col.combo);
            }
            mergedRows << QVariantMap{
                { "label", templateRow.value(QStringLiteral("label")) },
                { "values", values },
                { "fmt", templateRow.value(QStringLiteral("fmt")) },
                { "bold", templateRow.value(QStringLiteral("bold")) },
                { "merged", templateRow.value(QStringLiteral("merged")) },
            };
        }

        years << QVariantMap{ { "year", a + 1 }, { "rows", mergedRows } };
    }

    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    if (!pdf::writeSimulation(&buf, years, combinacionLabels))
        return {};
    buf.close();

    return saveOrDownloadPdf(buf.data(), QStringLiteral("FarmaciaSim_Simulacion"), m_pdfSaveDir);
}

void Engine::recalc()
{
    m_r = sim::compute(m_in);
    m_bank->setResult(m_r.bankAmort);
    m_coop ->setResult(m_r.coopAmort);
    m_family ->setResult(m_r.familyAmort);
    m_properties ->setResult(m_r.propertiesAmort);
    buildMaps();
    saveToDisk();
    emit recalculated();
}

// ---------------------------------------------------------------- helpers

// Label of an IRPF bracket (e.g. "12.450 – 20.200 € (24%)" or, for the last
// bracket, "> 300.000 € (47%)"). Generated from the actual bracket
// (in.irpfBrackets), which is editable from the Configuración sheet: a fixed
// string would go stale as soon as the user edits it.
static QString irpfBracketLabel(const sim::IrpfBracket& t, bool isLast)
{
    static const QLocale loc(QLocale::Spanish, QLocale::Spain);
    const double pct100 = t.rate * 100.0;
    const int decPct = (std::abs(pct100 - std::round(pct100)) < 1e-6) ? 0 : 1;
    const QString pctStr = loc.toString(pct100, 'f', decPct) + QStringLiteral("%");
    if (isLast)
        return QStringLiteral("> ") + loc.toString(t.from, 'f', 0)
             + QStringLiteral(" € (") + pctStr + QStringLiteral(")");
    return loc.toString(t.from, 'f', 0) + QStringLiteral(" – ") + loc.toString(t.to, 'f', 0)
         + QStringLiteral(" € (") + pctStr + QStringLiteral(")");
}

static QVariantList toList10(const std::array<double,10>& a)
{
    QVariantList l;
    for (double v : a) l << v;
    return l;
}

static QVariantList toList3(const std::array<double,3>& a)
{
    QVariantList l;
    for (double v : a) l << v;
    return l;
}

static QVariantList toList7(const std::array<double,7>& a)
{
    QVariantList l;
    for (double v : a) l << v;
    return l;
}

static QVariantMap projectionRow(const QString& label, const std::array<double,10>& vals,
                           const QString& fmt = QStringLiteral("eur"), bool bold = false)
{
    return { { "label", label }, { "values", toList10(vals) },
             { "fmt", fmt }, { "bold", bold } };
}

void Engine::buildMaps()
{
    // ---- inputs
    m_inputs.clear();
    for (auto it = m_dbl.cbegin(); it != m_dbl.cend(); ++it)
        m_inputs[it.key()] = *it.value();
    for (auto it = m_int.cbegin(); it != m_int.cend(); ++it)
        m_inputs[it.key()] = *it.value();

    // ---- Datos Base
    const auto& D = m_r.baseData;
    m_baseData = QVariantMap{
        { "totalSales",          D.totalSales },
        { "costOfGoods",         D.costOfGoods },
        { "grossMargin",         D.grossMargin },
        { "marginAfterRd",       D.marginAfterRd },
        { "staffCost",           D.staffCost },
        { "socialSecurity",      D.socialSecurity },
        { "selfEmployedQuota",   m_r.projection.selfEmployedQuota[0] },
        { "totalStaffCost",      D.totalStaffCost },
        { "totalOtherExpenses",  D.totalOtherExpenses },
        { "profitBeforeTax",     D.profitBeforeTax },
    };

    // ---- Financiación
    const auto& F = m_r.financing;
    m_financing = QVariantMap{
        { "goodwill",              F.goodwill },
        { "fees",                  F.fees },
        { "iva",                   F.iva },
        { "itpTax",                F.itpTax },
        { "ajd",                   F.ajd },
        { "taxes",                 F.taxes },
        { "mortgageOpeningCost",   F.mortgageOpeningCost },
        { "totalInvestment",       F.totalInvestment },
        { "pharmacyBankFinancing", F.pharmacyBankFinancing },
        { "premisesBankFinancing", F.premisesBankFinancing },
        { "totalFinancing",        F.totalFinancing },
        { "minimumCash",           F.minimumCash },
        { "cashBelowMinimum",      F.cashBelowMinimum },
    };

    // ---- Personal
    const auto& P = m_r.staff;
    static const char* dataRoleLabels[3] = { "Farmacéutico", "Auxiliar de farmacia", "Técnico" };
    static const char* headcountRoleLabels[4] = { "Propietario farmacéutico", "Farmacéutico empleado",
                                         "Auxiliar de farmacia", "Técnico especialista" };
    QVariantList byRole, headcountPlan;
    for (int k = 0; k < 3; ++k) {
        const auto& r = P.byRole[k];
        byRole << QVariantMap{
            { "role", QString::fromUtf8(dataRoleLabels[k]) },
            { "grossFte", r.grossFte }, { "fte", r.fte },
            { "socialSecurityPct", r.socialSecurityPct }, { "socialSecurityCost", r.socialSecurityCost },
            { "actualSalary", r.actualSalary }, { "totalCost", r.totalCost } };
    }
    for (int k = 0; k < 4; ++k) {
        const auto& r = P.headcountPlan[k];
        headcountPlan << QVariantMap{
            { "role", QString::fromUtf8(headcountRoleLabels[k]) },
            { "fte", r.fte }, { "headcount", r.headcount },
            { "grossFte", r.grossFte }, { "actualGross", r.actualGross },
            { "socialSecurityCost", r.socialSecurityCost }, { "costPerPerson", r.costPerPerson },
            { "totalCost", r.totalCost }};
    }
    static const char* vacationRoleLabels[3] = { "Farmacéutico sustituto", "Auxiliar sustituto", "Técnico sustituto" };
    QVariantList vacationStaffPlan;
    for (int k = 0; k < 3; ++k) {
        const auto& r = P.vacationStaffPlan[k];
        vacationStaffPlan << QVariantMap{
            { "role", QString::fromUtf8(vacationRoleLabels[k]) },
            { "fte", r.fte }, { "headcount", r.headcount },
            { "grossFte", r.grossFte }, { "actualGross", r.actualGross },
            { "socialSecurityCost", r.socialSecurityCost },
            { "totalCost", r.totalCost }, { "avgMonths", r.avgMonths } };
    }
    m_staff = QVariantMap{
        { "byRole", byRole },
        { "totalSocialSecurityCost", P.totalSocialSecurityCost }, { "totalActualSalary", P.totalActualSalary },
        { "totalCost", P.totalCost },
        { "headcountPlan", headcountPlan },
        { "totalHeadcount", P.totalHeadcount }, { "totalActualGross", P.totalActualGross },
        { "totalSocialSecurity", P.totalSocialSecurity }, { "totalHeadcountCost", P.totalHeadcountCost },
        { "netMonthlySalaryYear1", P.netMonthlySalaryYear1 },
        { "vacationStaffPlan", vacationStaffPlan },
        { "totalVacationHeadcount", P.totalVacationHeadcount }, { "totalVacationActualGross", P.totalVacationActualGross },
        { "totalVacationSocialSecurity", P.totalVacationSocialSecurity }, { "totalVacationCost", P.totalVacationCost },
    };

    // ---- Horario (cobertura semanal)
    const auto& S = m_r.schedule;
    m_schedule = QVariantMap{
        { "dailyHours",             toList7(S.dailyHours) },
        { "weeklyOpenHours",        S.weeklyOpenHours },
        { "pharmacistWeeklyHours",  S.pharmacistWeeklyHours },
        { "supportWeeklyHours",     S.supportWeeklyHours },
        { "totalStaffWeeklyHours",  S.totalStaffWeeklyHours },
        { "pharmacistHoursGap",     S.pharmacistHoursGap },
        { "totalHoursGap",          S.totalHoursGap },
    };

    // ---- Proyección (rows as in the sheet)
    const auto& Y = m_r.projection;
    m_projection = QVariantList{
        projectionRow("Venta receta",                  Y.prescriptionSales),
        projectionRow("Venta libre",                   Y.otcSales),
        projectionRow("Venta total",                   Y.totalSales, "eur", true),
        projectionRow("IPC aplicado",                  Y.ipcApplied, "pct1"),
        projectionRow("Coste mercancía",               Y.costOfGoods),
        projectionRow("M. comercial %",                Y.commercialMarginPct, "pct1"),
        projectionRow("M. comercial bruto",            Y.grossMargin),
        projectionRow("Reales decretos",                Y.rdDeduction),
        projectionRow("M. comercial después de RDs",   Y.marginAfterRd, "eur", true),
        projectionRow("Alquiler local",                Y.rent),
        projectionRow("Gastos personal + SS",           Y.staffCost),
        projectionRow("% Gastos de personal",           Y.staffCostPct, "pct1"),
        projectionRow("Cuota autónomos",                Y.selfEmployedQuota),
        projectionRow("Otros gastos",                   Y.otherExpenses),
        projectionRow("Intereses de deudas",            Y.interest),
        projectionRow("Beneficio farmacia",             Y.profit, "eur", true),
        projectionRow("Pago impuestos",                 Y.taxPayment),
        projectionRow("Liquidez después de imp.",       Y.cashAfterTax, "eur", true),
        projectionRow("Devolución banco",               Y.bankPrincipalRepayment),
        projectionRow("Devolución cooperativa",         Y.coopPrincipalRepayment),
        projectionRow("Salario neto anual titular",     Y.netAnnualSalary, "eur", true),
        projectionRow("Salario neto mensual titular",   Y.netMonthlySalary, "eur", true),
    };

    // ---- Impuestos (IRPF, v2)
    const auto& I = m_r.taxes;
    QVariantList bracketsList;
    for (int t = 0; t < 6; ++t)
        bracketsList << QVariantMap{
            { "label", irpfBracketLabel(m_in.irpfBrackets[t], t == 5) },
            { "values", toList10(I.brackets[t]) } };
    m_taxes = QVariantMap{
        { "fdc",               I.fdc },
        { "fees",               I.fees },
        { "ajd",               I.ajd },
        { "depreciableBase",   I.depreciableBase },
        { "premisesCost",      I.premisesCost },
        { "minimumDeduction",  I.minimumDeduction },
        { "profit",            toList10(I.profit) },
        { "premisesDepreciation", toList10(I.premisesDepreciation) },
        { "adjustedPct",       toList10(I.adjustedPct) },
        { "fdcDepreciation",   toList10(I.fdcDepreciation) },
        { "taxableBase",       toList10(I.taxableBase) },
        { "bracketQuota",      toList10(I.bracketQuota) },
        { "payment",           toList10(I.payment) },
        { "brackets",          bracketsList },
    };

    // ---- Análisis
    const auto& A = m_r.analysis;
    m_analysis = QVariantMap{
        { "initialInvestment", toList3(A.initialInvestment) },
        { "fdcSaleValue",      toList3(A.fdcSaleValue) },
        { "premisesSaleValue", toList3(A.premisesSaleValue) },
        { "inventoryYear10",   toList3(A.inventoryYear10) },
        { "fdcOutstanding",    toList3(A.fdcOutstanding) },
        { "debt",              toList3(A.debt) },
        { "grossEquity",       toList3(A.grossEquity) },
        { "netEquity",         toList3(A.netEquity) },
        { "cagr",              toList3(A.cagr) },
        { "irr",               toList3(A.irr) },
        { "monthlyCashFlow",        toList3(A.monthlyCashFlow) },
        { "monthlyPrincipalRepayment", toList3(A.monthlyPrincipalRepayment) },
        { "monthlyInterest",   toList3(A.monthlyInterest) },
        { "ownerNetIncome",    toList3(A.ownerNetIncome) },
        { "annualPremisesDepreciation", A.annualPremisesDepreciation },
        { "pharmacyProfit",    toList10(A.pharmacyProfit) },
        { "fdcDepreciationPct",toList10(A.fdcDepreciationPct) },
        { "fdcDepreciation",   toList10(A.fdcDepreciation) },
        { "premisesDepreciation", toList10(A.premisesDepreciation) },
        { "taxableBase",       toList10(A.taxableBase) },
        { "fdcOutstandingSim", toList10(A.fdcOutstandingSim) },
    };
}

// ---------------------------------------------------------------- comparación de escenarios

// Full snapshot of every currently registered input (same keys as
// registerInputs()/saveToDisk()), saved alongside each frozen scenario so it
// can later be restored wholesale by applyComparisonScenario().
QVariantMap Engine::inputsSnapshot() const
{
    QVariantMap snap;
    for (auto it = m_dbl.cbegin(); it != m_dbl.cend(); ++it)
        snap[it.key()] = *it.value();
    for (auto it = m_int.cbegin(); it != m_int.cend(); ++it)
        snap[it.key()] = *it.value();
    return snap;
}

void Engine::addComparisonScenario()
{
    // Values from the Financiación sheet for the "full view" group: they
    // don't vary by year, so they're saved as a fixed value per scenario.
    const QVariantMap fin{
        { "goodwill",              m_financing.value(QStringLiteral("goodwill")) },
        { "contributedCash",       m_inputs.value(QStringLiteral("contributedCash")) },
        { "pharmacyBankFinancing", m_financing.value(QStringLiteral("pharmacyBankFinancing")) },
        { "premisesBankFinancing", m_financing.value(QStringLiteral("premisesBankFinancing")) },
        { "initialOrder",          m_inputs.value(QStringLiteral("initialOrder")) },
        { "propertiesFinancing",   m_inputs.value(QStringLiteral("propertiesFinancing")) },
        { "propertiesTermYears",   m_inputs.value(QStringLiteral("propertiesTermYears")) },
    };
    const QVariantMap esc{
        { "id",         QDateTime::currentMSecsSinceEpoch() },
        { "name",       QStringLiteral("Escenario %1").arg(m_comparisonScenarios.size() + 1) },
        { "projection", m_projection },
        { "financing",  fin },
        { "inputs",     inputsSnapshot() },
    };
    m_comparisonScenarios.append(esc);
    saveToDisk();
    emit comparisonScenariosChanged();
}

void Engine::removeComparisonScenario(int index)
{
    if (index < 0 || index >= m_comparisonScenarios.size())
        return;
    m_comparisonScenarios.removeAt(index);
    saveToDisk();
    emit comparisonScenariosChanged();
}

bool Engine::applyComparisonScenario(int index)
{
    if (index < 0 || index >= m_comparisonScenarios.size())
        return false;
    const QVariantMap snap = m_comparisonScenarios[index].toMap()
                                .value(QStringLiteral("inputs")).toMap();
    if (snap.isEmpty())
        return false; // scenario frozen before the "inputs" snapshot existed

    for (auto it = snap.cbegin(); it != snap.cend(); ++it) {
        if (auto d = m_dbl.find(it.key()); d != m_dbl.end())
            **d = it.value().toDouble();
        else if (auto n = m_int.find(it.key()); n != m_int.end()) {
            const int v = it.value().toInt();
            if (v > 0) **n = v;
        }
    }
    m_in.staffCount[0] = 1; // Owner pharmacist: always 1 person, not editable
    recalc();
    return true;
}

// Pivots "scenario -> rows(concept, 10 years)" into "rows(concept) -> scenarios",
// keeping the value for the requested year of each scenario.
QVariantList Engine::comparisonForYear(int year) const
{
    QVariantList rows;
    if (m_comparisonScenarios.isEmpty())
        return rows;

    const QVariantList templateRows = m_comparisonScenarios.first().toMap()
                                        .value(QStringLiteral("projection")).toList();
    for (int r = 0; r < templateRows.size(); ++r) {
        const QVariantMap templateRow = templateRows[r].toMap();
        QVariantList values;
        for (const QVariant& ev : m_comparisonScenarios) {
            const QVariantList scenarioRows = ev.toMap().value(QStringLiteral("projection")).toList();
            double v = 0;
            if (r < scenarioRows.size()) {
                const QVariantList rowValues = scenarioRows[r].toMap().value(QStringLiteral("values")).toList();
                if (year >= 0 && year < rowValues.size())
                    v = rowValues[year].toDouble();
            }
            values << v;
        }
        rows << QVariantMap{
            { "label", templateRow.value(QStringLiteral("label")) },
            { "values", values },
            { "fmt", templateRow.value(QStringLiteral("fmt")) },
            { "bold", templateRow.value(QStringLiteral("bold")) },
        };
    }
    return rows;
}

QVariantList Engine::financingComparison() const
{
    static const struct { const char* key; const char* label; const char* fmt; } kRows[] = {
        { "goodwill",              "Valor farmacia total",  "eur" },
        { "contributedCash",       "Liquidez aportada",     "eur" },
        { "pharmacyBankFinancing", "Financiación farmacia", "eur" },
        { "premisesBankFinancing", "Financiación local",    "eur" },
        { "initialOrder",          "Cooperativa",           "eur" },
        { "propertiesFinancing",   "Hipoteca propiedad",    "eur" },
        { "propertiesTermYears",   "Años de hipoteca",      "years" },
    };

    QVariantList rows;
    if (m_comparisonScenarios.isEmpty())
        return rows;

    for (const auto& row : kRows) {
        const QString key = QString::fromUtf8(row.key);
        QVariantList values;
        for (const QVariant& ev : m_comparisonScenarios) {
            const QVariantMap fin = ev.toMap().value(QStringLiteral("financing")).toMap();
            values << fin.value(key, 0.0);
        }
        rows << QVariantMap{
            { "label", QString::fromUtf8(row.label) },
            { "values", values },
            { "fmt", QString::fromUtf8(row.fmt) },
            { "bold", false },
        };
    }
    return rows;
}

// ---------------------------------------------------------------- simulación (todas las combinaciones)

// Un grupo por cada Facturación Total (la actual / + un margen editable en
// la propia hoja Simulación, ver simulationRevenueDeltaEur y los demás
// simulation*Delta* en simcore.h): dentro de cada grupo, las 8 combinaciones
// (2×2×2) de aportación inicial, plazo e interés de hipoteca comparten esa
// misma facturación, así que se muestran juntas y los grupos se apilan uno
// debajo de otro (ver SimulacionView.qml).
QVariantList Engine::simulationForYear(int year) const
{
    // Ejes de variación: cada uno usa el valor actual del dato real
    // correspondiente (no una constante fija) para la primera columna, y ese
    // mismo valor + un margen editable en la hoja Simulación para la
    // segunda. Ninguno puede ser 'static' al depender de m_in.
    //
    // Facturación Total se aplica como factor sobre venta receta+libre
    // (conserva su proporción): el margen en euros (simulationRevenueDeltaEur)
    // se convierte al factor equivalente sobre la facturación actual.
    const double facturacionActual = m_in.prescriptionSales + m_in.otcSales;
    const double facFactorAlto = facturacionActual > 0
        ? (facturacionActual + m_in.simulationRevenueDeltaEur) / facturacionActual
        : 1.0;
    const std::array<double,2> kFacturacionFactor { 1.0, facFactorAlto };

    // El plazo y el interés se aplican a la vez a la hipoteca mobiliaria
    // (banco) e inmobiliaria (propiedades): ambas toman siempre el mismo
    // valor dentro de cada combinación (no hay escenarios "mixtos" con tipos
    // distintos), tomado como referencia el de la hipoteca bancaria.
    const std::array<int,2> kPlazoHipoteca {
        m_in.bankTermYears, m_in.bankTermYears + int(std::lround(m_in.simulationTermDeltaYears))
    };
    const std::array<double,2> kTipoHipoteca { m_in.bankRate, m_in.bankRate + m_in.simulationRateDeltaPct };

    // La aportación inicial actual (Financiación: "Liquidez aportada") y esa
    // misma cifra + el margen editable en la hoja Simulación.
    const std::array<double,2> kAportacionInicial {
        m_in.contributedCash, m_in.contributedCash + m_in.simulationCashDeltaEur
    };

    QVariantList groups;

    for (double facFactor : kFacturacionFactor) {
        QVariantList aniosMobiliaria, tipoMobiliaria,
                     aniosInmobiliaria, tipoInmobiliaria, aportacion,
                     costeTotal, interesesTotales, beneficio;
        double facturacionTotal = 0;

        // Aportación es el eje más externo (tras Facturación) para que, dentro
        // de cada grupo, todas las columnas con la aportación actual queden
        // antes que las de esa cifra + el margen configurado.
        for (double cash : kAportacionInicial)
        for (int plazoYears : kPlazoHipoteca)
        for (double tipo : kTipoHipoteca) {
            sim::Inputs in = m_in;
            in.prescriptionSales   = m_in.prescriptionSales * facFactor;
            in.otcSales             = m_in.otcSales * facFactor;
            in.bankTermYears        = plazoYears;
            in.bankRate              = tipo;
            in.propertiesTermYears  = plazoYears;
            in.propertiesRate        = tipo;
            in.contributedCash      = cash;

            const sim::Results r = sim::compute(in);
            facturacionTotal = r.baseData.totalSales; // igual para toda combinación de este grupo
            const double netMonthly = (year >= 0 && year < 10) ? r.projection.netMonthlySalary[year] : 0.0;

            aniosMobiliaria    << plazoYears;
            tipoMobiliaria      << tipo;
            aniosInmobiliaria  << plazoYears;
            tipoInmobiliaria    << tipo;
            aportacion          << cash;
            costeTotal          << r.financing.totalInvestment;
            // Solo las hipotecas mobiliaria e inmobiliaria varían con plazo/
            // interés en esta simulación (cooperativa y préstamo familiar
            // quedan fijos), así que son las únicas que se suman aquí.
            interesesTotales    << (r.bankAmort.totalInterest + r.propertiesAmort.totalInterest);
            beneficio            << netMonthly;
        }

        const QVariantList rows{
            QVariantMap{ { "label", QStringLiteral("Años hipoteca mobiliaria") },
                         { "values", aniosMobiliaria }, { "fmt", QStringLiteral("years") }, { "bold", false } },
            QVariantMap{ { "label", QStringLiteral("Interés hipoteca mobiliaria") },
                         { "values", tipoMobiliaria }, { "fmt", QStringLiteral("pct1") }, { "bold", false } },
            QVariantMap{ { "label", QStringLiteral("Años hipoteca inmobiliaria") },
                         { "values", aniosInmobiliaria }, { "fmt", QStringLiteral("years") }, { "bold", false } },
            QVariantMap{ { "label", QStringLiteral("Interés hipoteca inmobiliaria") },
                         { "values", tipoInmobiliaria }, { "fmt", QStringLiteral("pct1") }, { "bold", false } },
            QVariantMap{ { "label", QStringLiteral("Aportación inicial") },
                         { "values", aportacion }, { "fmt", QStringLiteral("eur") }, { "bold", false } },
            // "merged": no depende del plazo/interés de la hipoteca, solo de
            // la aportación inicial (y, dentro de ese margen, apenas varía:
            // solo por la comisión de apertura sobre la financiación
            // bancaria) — repetirlo en las 8 columnas es ruido, así que se
            // muestra una sola vez, centrado, con el valor de la primera
            // combinación (ver ConceptTable.qml y Table::dataRow en
            // pdfreport.cpp).
            QVariantMap{ { "label", QStringLiteral("Coste total farmacia") },
                         { "values", costeTotal }, { "fmt", QStringLiteral("eur") }, { "bold", false },
                         { "merged", true } },
            QVariantMap{ { "label", QStringLiteral("Intereses totales pagados") },
                         { "values", interesesTotales }, { "fmt", QStringLiteral("eur") }, { "bold", false } },
            QVariantMap{ { "label", QStringLiteral("Beneficio neto mensual") },
                         { "values", beneficio }, { "fmt", QStringLiteral("eur") }, { "bold", true } },
        };

        groups << QVariantMap{
            { "facturacion", facturacionTotal },
            { "rows",        rows },
        };
    }

    return groups;
}
