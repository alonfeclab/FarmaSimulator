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
    const QJsonObject o = translateLegacyObject(doc.object());
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
}

void Engine::registerInputs()
{
    auto& i = m_in;
    // ---- Datos Base
    m_dbl["prescriptionSales"] = &i.prescriptionSales;
    m_dbl["otcSales"]          = &i.otcSales;
    m_dbl["marginPct"]         = &i.marginPct;
    m_dbl["premisesRent"]      = &i.premisesRent;
    m_dbl["utilities"]         = &i.utilities;
    m_dbl["advisoryFees"]      = &i.advisoryFees;
    m_dbl["maintenance"]       = &i.maintenance;
    m_dbl["robot"]             = &i.robot;
    m_dbl["insurance"]         = &i.insurance;
    m_dbl["otherExpenses"]     = &i.otherExpenses;
    // ---- Financiación: growth scenario
    m_dbl["growthScenario"]       = &i.growthScenario;
    m_dbl["ipcOptimistic"]        = &i.ipcOptimistic;
    m_dbl["optimisticMarginYear1"] = &i.optimisticMarginYear1;
    m_dbl["optimisticMarginYear2"] = &i.optimisticMarginYear2;
    m_dbl["optimisticMarginYear3"] = &i.optimisticMarginYear3;
    // ---- Financiación: investment, rates, contributions
    m_dbl["goodwillMultiple"]  = &i.goodwillMultiple;
    m_dbl["premisesPrice"]     = &i.premisesPrice;
    m_dbl["inventory"]         = &i.inventory;
    m_dbl["notaryFees"]        = &i.notaryFees;
    m_dbl["registryFees"]      = &i.registryFees;
    m_dbl["miscExpenses"]      = &i.miscExpenses;
    m_dbl["feesPct"]           = &i.feesPct;
    m_dbl["ivaPct"]            = &i.ivaPct;
    m_dbl["itpPct"]            = &i.itpPct;
    m_dbl["ajdPct"]            = &i.ajdPct;
    m_dbl["bankRate"]          = &i.bankRate;
    m_dbl["coopRate"]          = &i.coopRate;
    m_dbl["familyRate"]        = &i.familyRate;
    m_dbl["propertiesRate"]    = &i.propertiesRate;
    m_int["bankTermYears"]       = &i.bankTermYears;
    m_int["coopTermYears"]       = &i.coopTermYears;
    m_int["familyTermYears"]     = &i.familyTermYears;
    m_int["propertiesTermYears"] = &i.propertiesTermYears;
    m_dbl["pharmacyFinancingPct"]  = &i.pharmacyFinancingPct;
    m_dbl["premisesFinancingPct"]  = &i.premisesFinancingPct;
    m_dbl["mortgageOpeningPct"]    = &i.mortgageOpeningPct;
    m_dbl["propertiesFinancingPct"] = &i.propertiesFinancingPct;
    m_dbl["contributedCash"]     = &i.contributedCash;
    m_dbl["familyContribution"]  = &i.familyContribution;
    m_dbl["propertiesFinancing"] = &i.propertiesFinancing;
    m_dbl["contributionExcess"]  = &i.contributionExcess;
    m_dbl["initialOrder"]        = &i.initialOrder;
    // ---- Personal
    m_dbl["pharmacistSalary"]  = &i.pharmacistSalary;
    m_dbl["pharmacistFte"]     = &i.pharmacistFte;
    m_dbl["socialSecurityPct"] = &i.socialSecurityPct;
    m_dbl["raisePct"]          = &i.raisePct;
    m_dbl["assistantSalary"]   = &i.assistantSalary;
    m_dbl["assistantFte"]      = &i.assistantFte;
    m_dbl["technicianSalary"]  = &i.technicianSalary;
    m_dbl["technicianFte"]     = &i.technicianFte;
    for (int k = 0; k < 4; ++k) {
        m_dbl[QStringLiteral("staffFte%1").arg(k)]   = &i.staffFte[k];
        m_dbl[QStringLiteral("staffCount%1").arg(k)] = &i.staffCount[k];
    }
    // ---- Análisis
    for (int k = 0; k < 3; ++k) {
        m_dbl[QStringLiteral("saleFactor%1").arg(k)] = &i.saleFactor[k];
        m_dbl[QStringLiteral("saleTaxes%1").arg(k)]  = &i.saleTaxes[k];
    }
    m_dbl["fdcInitialSim"]     = &i.fdcInitialSim;
    m_dbl["fdcMaxPct"]         = &i.fdcMaxPct;
    m_dbl["investmentPremisesDeprPct"] = &i.investmentPremisesDeprPct;
    m_dbl["inventoryPctYear10"]  = &i.inventoryPctYear10;
    // ---- Hoja Impuestos (v2)
    m_dbl["taxPremisesDeprPct"]    = &i.taxPremisesDeprPct;
    m_dbl["taxMinGoodwillDeprPct"] = &i.taxMinGoodwillDeprPct;
    m_dbl["taxMaxGoodwillDeprPct"] = &i.taxMaxGoodwillDeprPct;
    m_dbl["personalAllowance"]     = &i.personalAllowance;
    // ---- Configuración: official scales and series (editable)
    for (int k = 0; k < 6; ++k) {
        m_dbl[QStringLiteral("irpfFrom%1").arg(k)] = &i.irpfBrackets[k].from;
        m_dbl[QStringLiteral("irpfTo%1").arg(k)]   = &i.irpfBrackets[k].to;
        m_dbl[QStringLiteral("irpfRate%1").arg(k)] = &i.irpfBrackets[k].rate;
    }
    for (int k = 0; k < 9; ++k) {
        m_dbl[QStringLiteral("rdFrom%1").arg(k)] = &i.rdBrackets[k].from;
        m_dbl[QStringLiteral("rdBase%1").arg(k)] = &i.rdBrackets[k].base;
        m_dbl[QStringLiteral("rdPct%1").arg(k)]  = &i.rdBrackets[k].pct;
    }
    for (int k = 0; k < 15; ++k) {
        m_dbl[QStringLiteral("retaFrom%1").arg(k)]  = &i.retaBrackets[k].from;
        m_dbl[QStringLiteral("retaQuota%1").arg(k)] = &i.retaBrackets[k].monthlyQuota;
    }
    m_dbl["retaFlatMonthlyFee"] = &i.retaFlatMonthlyFee;
    for (int k = 0; k < 10; ++k) {
        m_dbl[QStringLiteral("ipcHistorical%1").arg(k)]        = &i.ipcHistorical[k];
        m_dbl[QStringLiteral("realisticMarginSeries%1").arg(k)] = &i.realisticMarginSeries[k];
    }
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
// Documents with the given prefix and opens it; WASM: the browser downloads
// it. Returns the path (or "descargas del navegador" on WASM), empty on failure.
static QString saveOrDownloadPdf(const QByteArray& data, const QString& filenamePrefix)
{
    const QString fileName = filenamePrefix + QLatin1Char('_')
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmm")) + QStringLiteral(".pdf");

#ifdef Q_OS_WASM
    downloadInBrowser(data, fileName);
    return QStringLiteral("descargas del navegador");
#else
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
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

QString Engine::exportPdf()
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    if (!pdf::writeReport(&buf, m_in, m_r))
        return {};
    buf.close();

    return saveOrDownloadPdf(buf.data(), QStringLiteral("FarmaciaSim_Informe"));
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

    return saveOrDownloadPdf(buf.data(), QStringLiteral("FarmaciaSim_Comparacion"));
}

void Engine::recalc()
{
    m_r = sim::compute(m_in);
    m_bank->setResult(m_r.bankAmort);
    m_coop ->setResult(m_r.coopAmort);
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
    m_staff = QVariantMap{
        { "byRole", byRole },
        { "totalSocialSecurityCost", P.totalSocialSecurityCost }, { "totalActualSalary", P.totalActualSalary },
        { "totalCost", P.totalCost },
        { "headcountPlan", headcountPlan },
        { "totalHeadcount", P.totalHeadcount }, { "totalActualGross", P.totalActualGross },
        { "totalSocialSecurity", P.totalSocialSecurity }, { "totalHeadcountCost", P.totalHeadcountCost },
        { "netMonthlySalaryYear1", P.netMonthlySalaryYear1 },
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
void Engine::addComparisonScenario()
{
    // Values from the Financiación sheet for the "full view" group: they
    // don't vary by year, so they're saved as a fixed value per scenario.
    const QVariantMap fin{
        { "contributedCash",       m_inputs.value(QStringLiteral("contributedCash")) },
        { "pharmacyBankFinancing", m_financing.value(QStringLiteral("pharmacyBankFinancing")) },
        { "premisesBankFinancing", m_financing.value(QStringLiteral("premisesBankFinancing")) },
        { "initialOrder",          m_inputs.value(QStringLiteral("initialOrder")) },
        { "propertiesFinancing",   m_inputs.value(QStringLiteral("propertiesFinancing")) },
    };
    const QVariantMap esc{
        { "id",         QDateTime::currentMSecsSinceEpoch() },
        { "name",       QStringLiteral("Escenario %1").arg(m_comparisonScenarios.size() + 1) },
        { "projection", m_projection },
        { "financing",  fin },
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
    static const struct { const char* key; const char* label; } kRows[] = {
        { "contributedCash",       "Liquidez aportada" },
        { "pharmacyBankFinancing", "Financiación farmacia" },
        { "premisesBankFinancing", "Financiación local" },
        { "initialOrder",          "Cooperativa" },
        { "propertiesFinancing",   "Hipoteca propiedad" },
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
            { "fmt", QStringLiteral("eur") },
            { "bold", false },
        };
    }
    return rows;
}

// ---------------------------------------------------------------- simulación aislada
// Computes a "quick" projection with some inputs substituted, from a copy of
// the current inputs. Doesn't touch m_in nor trigger recalc()/saveToDisk():
// this way changes on the "Simulación simple" sheet don't propagate to the
// other sheets nor get saved.
QVariantMap Engine::simulateQuick(const QVariantMap& overrides) const
{
    sim::Inputs workingCopy = m_in;
    if (overrides.contains(QStringLiteral("prescriptionSales")))
        workingCopy.prescriptionSales = overrides[QStringLiteral("prescriptionSales")].toDouble();
    if (overrides.contains(QStringLiteral("otcSales")))
        workingCopy.otcSales = overrides[QStringLiteral("otcSales")].toDouble();
    if (overrides.contains(QStringLiteral("inventory")))
        workingCopy.inventory = overrides[QStringLiteral("inventory")].toDouble();
    if (overrides.contains(QStringLiteral("contributedCash")))
        workingCopy.contributedCash = overrides[QStringLiteral("contributedCash")].toDouble();
    if (overrides.contains(QStringLiteral("propertiesFinancing")))
        workingCopy.propertiesFinancing = overrides[QStringLiteral("propertiesFinancing")].toDouble();
    if (overrides.contains(QStringLiteral("initialOrder")))
        workingCopy.initialOrder = overrides[QStringLiteral("initialOrder")].toDouble();
    if (overrides.contains(QStringLiteral("premisesRent")))
        workingCopy.premisesRent = overrides[QStringLiteral("premisesRent")].toDouble();

    const sim::Results r = sim::compute(workingCopy);
    const auto& Y = r.projection;
    const QVariantList projection{
        projectionRow("Venta total",                   Y.totalSales, "eur"),
        projectionRow("M. Comercial después de RDs",   Y.marginAfterRd, "eur"),
        projectionRow("Beneficio farmacia",            Y.profit, "eur"),
        projectionRow("Liquidez después de imp.",      Y.cashAfterTax, "eur"),
        projectionRow("Salario neto anual titular",    Y.netAnnualSalary, "eur", true),
        projectionRow("Salario neto mensual titular",  Y.netMonthlySalary, "eur", true),
    };
    return QVariantMap{
        { "totalSales", r.baseData.totalSales },
        { "projection", projection },
        { "minimumCash", r.financing.minimumCash },
        { "cashBelowMinimum", r.financing.cashBelowMinimum },
    };
}
