#include "engine.h"
#include "amortmodel.h"
#include "pdfreport.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WASM
#include <emscripten/val.h>   // acceso síncrono a localStorage del navegador
#endif

Engine::Engine(QObject* parent)
    : QObject(parent)
{
    m_banco = new AmortModel(QStringLiteral("Amortización préstamo bancario"), this);
    m_coop  = new AmortModel(QStringLiteral("Amortización cooperativa"), this);
    m_prop  = new AmortModel(QStringLiteral("Amortización propiedades"), this);
    registerInputs();
    resolverRutaDatos();
    cargarDeDisco();
    recalc();
}

// ---------------------------------------------------------------- persistencia
void Engine::resolverRutaDatos()
{
#ifdef Q_OS_WASM
    // En el navegador no hay sistema de archivos: se usa localStorage.
    m_rutaDatos = QStringLiteral("almacenamiento del navegador (localStorage)");
#else
    // Preferir un JSON junto al ejecutable (uso portable). Si esa carpeta no
    // es escribible (p. ej. Archivos de programa), usar AppData.
    const QString junto = QCoreApplication::applicationDirPath()
                        + QStringLiteral("/farmaciasim_datos.json");
    QFile probe(junto);
    if (probe.open(QIODevice::ReadWrite)) {   // no trunca; crea si no existe
        probe.close();
        if (probe.size() == 0)
            QFile::remove(junto);             // no dejar un archivo vacío
        m_rutaDatos = junto;
        return;
    }
    const QString appData =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    m_rutaDatos = appData + QStringLiteral("/farmaciasim_datos.json");
#endif
}

void Engine::guardarEnDisco() const
{
    QJsonObject o;
    for (auto it = m_dbl.cbegin(); it != m_dbl.cend(); ++it)
        o[it.key()] = *it.value();
    for (auto it = m_int.cbegin(); it != m_int.cend(); ++it)
        o[it.key()] = *it.value();
    o["escenariosComparacion"] = QJsonArray::fromVariantList(m_escenariosComparacion);

#ifdef Q_OS_WASM
    const QByteArray json = QJsonDocument(o).toJson(QJsonDocument::Compact);
    emscripten::val::global("localStorage")
        .call<void>("setItem", std::string("farmaciasim_datos"),
                    json.toStdString());
#else
    QSaveFile f(m_rutaDatos);                 // escritura atómica
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("No se pudo guardar en '%s'", qPrintable(m_rutaDatos));
        return;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.commit();
#endif
}

void Engine::cargarDeDisco()
{
    QByteArray datos;
#ifdef Q_OS_WASM
    const emscripten::val v = emscripten::val::global("localStorage")
        .call<emscripten::val>("getItem", std::string("farmaciasim_datos"));
    if (v.isNull() || v.isUndefined())
        return;                                // primera ejecución: defaults
    datos = QByteArray::fromStdString(v.as<std::string>());
#else
    QFile f(m_rutaDatos);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;                                // primera ejecución: defaults
    datos = f.readAll();
#endif
    const QJsonDocument doc = QJsonDocument::fromJson(datos);
    if (!doc.isObject())
        return;
    const QJsonObject o = doc.object();
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
    m_in.plPersonas[0] = 1; // Farmacéutico titular: siempre 1 persona, no editable

    if (const auto it = o.constFind(QStringLiteral("escenariosComparacion"));
        it != o.constEnd() && it->isArray()) {
        m_escenariosComparacion = it->toArray().toVariantList();
    }
}

void Engine::registerInputs()
{
    auto& i = m_in;
    // ---- Datos Base
    m_dbl["ventaReceta"]    = &i.ventaReceta;
    m_dbl["ventaLibre"]     = &i.ventaLibre;
    m_dbl["margenPct"]      = &i.margenPct;
    m_dbl["alquilerLocal"]  = &i.alquilerLocal;
    m_dbl["suministros"]    = &i.suministros;
    m_dbl["asesoria"]       = &i.asesoria;
    m_dbl["mantenimiento"]  = &i.mantenimiento;
    m_dbl["robot"]          = &i.robot;
    m_dbl["seguros"]        = &i.seguros;
    m_dbl["otrosGastos"]    = &i.otrosGastos;
    // ---- Financiación: escenario de crecimiento
    m_dbl["escenarioCrecimiento"] = &i.escenarioCrecimiento;
    m_dbl["ipcOptimista"]         = &i.ipcOptimista;
    // ---- Financiación: inversión, tipos, aportaciones
    m_dbl["coeficiente"]       = &i.coeficiente;
    m_dbl["localComercial"]    = &i.localComercial;
    m_dbl["existencias"]       = &i.existencias;
    m_dbl["gastosVarios"]      = &i.gastosVarios;
    m_dbl["tipoBanco"]         = &i.tipoBanco;
    m_dbl["tipoCoop"]          = &i.tipoCoop;
    m_dbl["tipoFamiliar"]      = &i.tipoFamiliar;
    m_dbl["tipoPropiedades"]   = &i.tipoPropiedades;
    m_int["plazoBanco"]        = &i.plazoBanco;
    m_int["plazoCoop"]         = &i.plazoCoop;
    m_int["plazoFamiliar"]     = &i.plazoFamiliar;
    m_int["plazoPropiedades"]  = &i.plazoPropiedades;
    m_dbl["pctFinFarmacia"]    = &i.pctFinFarmacia;
    m_dbl["pctFinLocal"]       = &i.pctFinLocal;
    m_dbl["pctFinPropiedades"] = &i.pctFinPropiedades;
    m_dbl["liquidezAportada"]  = &i.liquidezAportada;
    m_dbl["aportacionFamiliar"]= &i.aportacionFamiliar;
    m_dbl["finPropiedades"]    = &i.finPropiedades;
    m_dbl["excesoAportacion"]  = &i.excesoAportacion;
    m_dbl["pedidoInicial"]     = &i.pedidoInicial;
    // ---- Personal
    m_dbl["salFarmaceutico"]  = &i.salFarmaceutico;
    m_dbl["jornFarmaceutico"] = &i.jornFarmaceutico;
    m_dbl["pctSS"]            = &i.pctSS;
    m_dbl["subidaPct"]        = &i.subidaPct;
    m_dbl["salAuxiliar"]      = &i.salAuxiliar;
    m_dbl["jornAuxiliar"]     = &i.jornAuxiliar;
    m_dbl["salTecnico"]       = &i.salTecnico;
    m_dbl["jornTecnico"]      = &i.jornTecnico;
    for (int k = 0; k < 4; ++k) {
        m_dbl[QStringLiteral("plJornada%1").arg(k)]  = &i.plJornada[k];
        m_dbl[QStringLiteral("plPersonas%1").arg(k)] = &i.plPersonas[k];
    }
    // ---- Análisis
    for (int k = 0; k < 3; ++k) {
        m_dbl[QStringLiteral("factorVenta%1").arg(k)]    = &i.factorVenta[k];
        m_dbl[QStringLiteral("impuestosVenta%1").arg(k)] = &i.impuestosVenta[k];
    }
    m_dbl["fdcInicialSim"] = &i.fdcInicialSim;
    m_dbl["pctMaxFdC"]     = &i.pctMaxFdC;
    m_dbl["pctAmortLocal"] = &i.pctAmortLocal;
    // ---- Hoja Impuestos (v2)
    m_dbl["impAmortLocalPct"] = &i.impAmortLocalPct;
    m_dbl["impAmortMinPct"]   = &i.impAmortMinPct;
    m_dbl["impAmortMaxPct"]   = &i.impAmortMaxPct;
    m_dbl["minimoPersonal"]   = &i.minimoPersonal;
}

void Engine::set(const QString& key, double value)
{
    if (key == QStringLiteral("plPersonas0"))
        return; // Farmacéutico titular: siempre 1 persona, no editable

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

void Engine::restaurarValoresIniciales()
{
    m_in = sim::Inputs{};
    recalc();
}

// ---------------------------------------------------------------- exportar PDF
#ifdef Q_OS_WASM
// Descarga en el navegador: Blob + <a download>, sin necesitar QtWidgets.
static void descargarEnNavegador(const QByteArray& datos, const QString& nombre)
{
    using emscripten::val;
    val vista{ emscripten::typed_memory_view(
        size_t(datos.size()), reinterpret_cast<const uint8_t*>(datos.constData())) };
    val arr = val::global("Uint8Array").new_(datos.size());
    arr.call<void>("set", vista);

    val partes = val::array();
    partes.call<void>("push", arr);
    val opciones = val::object();
    opciones.set("type", std::string("application/pdf"));
    val blob = val::global("Blob").new_(partes, opciones);

    val url = val::global("URL").call<val>("createObjectURL", blob);
    val a = val::global("document").call<val>("createElement", std::string("a"));
    a.set("href", url);
    a.set("download", nombre.toStdString());
    a.call<void>("click");
    val::global("URL").call<void>("revokeObjectURL", url);
}
#endif

// Guarda/descarga los bytes de un PDF ya generado. Escritorio: lo guarda en
// Documentos con el prefijo dado y lo abre; WASM: lo descarga el navegador.
// Devuelve la ruta (o "descargas del navegador" en WASM), vacío si falla.
static QString guardarPdf(const QByteArray& datos, const QString& prefijoNombre)
{
    const QString nombre = prefijoNombre + QLatin1Char('_')
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmm")) + QStringLiteral(".pdf");

#ifdef Q_OS_WASM
    descargarEnNavegador(datos, nombre);
    return QStringLiteral("descargas del navegador");
#else
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    const QString ruta = dir + QLatin1Char('/') + nombre;

    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("No se pudo escribir el PDF en '%s'", qPrintable(ruta));
        return {};
    }
    f.write(datos);
    f.close();
    QDesktopServices::openUrl(QUrl::fromLocalFile(ruta));   // abrirlo al momento
    return ruta;
#endif
}

QString Engine::exportarPdf()
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    if (!pdf::escribirInforme(&buf, m_in, m_r))
        return {};
    buf.close();

    return guardarPdf(buf.data(), QStringLiteral("FarmaciaSim_Informe"));
}

QString Engine::exportarPdfComparacion(int anio)
{
    if (m_escenariosComparacion.isEmpty() || anio < 0)
        return {};

    // Igual que ComparacionView.qml (filasTabla), pero siempre en "vista
    // completa": el grupo "Financiación" se intercala después de "VENTA
    // TOTAL" sin depender del interruptor de la interfaz.
    const QVariantList filas = comparacionAnio(anio);
    const QVariantList filasFin = comparacionFinanciacion();

    QVariantList grupo;
    grupo << QVariantMap{ { "label", QStringLiteral("FINANCIACIÓN") }, { "separator", true } };
    for (int i = 0; i < filasFin.size(); ++i) {
        QVariantMap f = filasFin[i].toMap();
        f[QStringLiteral("grupo")] = true;
        f[QStringLiteral("groupEnd")] = (i == filasFin.size() - 1);
        grupo << f;
    }

    int idx = -1;
    for (int i = 0; i < filas.size(); ++i) {
        if (filas[i].toMap().value(QStringLiteral("label")).toString() == QStringLiteral("VENTA TOTAL")) {
            idx = i;
            break;
        }
    }
    const QVariantList filasTabla = idx < 0
        ? filas + grupo
        : filas.mid(0, idx + 1) + grupo + filas.mid(idx + 1);

    QStringList nombres;
    for (const QVariant& ev : m_escenariosComparacion)
        nombres << ev.toMap().value(QStringLiteral("nombre")).toString();

    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    if (!pdf::escribirComparacion(&buf, filasTabla, nombres, anio + 1))
        return {};
    buf.close();

    return guardarPdf(buf.data(), QStringLiteral("FarmaciaSim_Comparacion"));
}

void Engine::recalc()
{
    m_r = sim::compute(m_in);
    m_banco->setResult(m_r.amortBanco);
    m_coop ->setResult(m_r.amortCoop);
    m_prop ->setResult(m_r.amortProp);
    buildMaps();
    guardarEnDisco();
    emit recalculated();
}

// ---------------------------------------------------------------- helpers
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

static QVariantMap proyRow(const QString& label, const std::array<double,10>& vals,
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
    const auto& D = m_r.datosBase;
    m_datosBase = QVariantMap{
        { "ventaTotal",          D.ventaTotal },
        { "costeMercancia",      D.costeMercancia },
        { "mComBruto",           D.mComBruto },
        { "mComDespuesRD",       D.mComDespuesRD },
        { "gastosPersonal",      D.gastosPersonal },
        { "seguridadSocial",     D.seguridadSocial },
        { "cuotaAutonomos",      m_r.proyeccion.cuotaAutonomos[0] },
        { "totalGastosPersonal", D.totalGastosPersonal },
        { "totalOtrosGastos",    D.totalOtrosGastos },
        { "beneficioAntesImp",   D.beneficioAntesImp },
    };

    // ---- Financiación
    const auto& F = m_r.financiacion;
    m_financiacion = QVariantMap{
        { "fondoComercio",        F.fondoComercio },
        { "honorarios",           F.honorarios },
        { "iva",                  F.iva },
        { "impuestoITP",          F.impuestoITP },
        { "ajd",                  F.ajd },
        { "impuestos",            F.impuestos },
        { "totalInversion",       F.totalInversion },
        { "finBancariaFarmacia",  F.finBancariaFarmacia },
        { "finBancariaLocal",     F.finBancariaLocal },
        { "totalFinanciacion",    F.totalFinanciacion },
        { "minimoLiquidez",       F.minimoLiquidez },
        { "liquidezInvalida",     F.liquidezInvalida },
    };

    // ---- Personal
    const auto& P = m_r.personal;
    static const char* tiposDatos[3] = { "Farmacéutico", "Auxiliar de farmacia", "Técnico" };
    static const char* tiposPlant[4] = { "Propietario farmacéutico", "Farmacéutico empleado",
                                         "Auxiliar de farmacia", "Técnico especialista" };
    QVariantList datos, plantilla;
    for (int k = 0; k < 3; ++k) {
        const auto& r = P.datos[k];
        datos << QVariantMap{
            { "tipo", QString::fromUtf8(tiposDatos[k]) },
            { "brutoFT", r.brutoFT }, { "jornada", r.jornada },
            { "pctSS", r.pctSS }, { "costeSS", r.costeSS },
            { "salReal", r.salReal }, { "costeTotal", r.costeTotal } };
    }
    for (int k = 0; k < 4; ++k) {
        const auto& r = P.plantilla[k];
        plantilla << QVariantMap{
            { "tipo", QString::fromUtf8(tiposPlant[k]) },
            { "jornada", r.jornada }, { "personas", r.personas },
            { "brutoFT", r.brutoFT }, { "brutoReal", r.brutoReal },
            { "costeSS", r.costeSS }, { "costePersona", r.costePersona },
            { "costeTotal", r.costeTotal }};
    }
    m_personal = QVariantMap{
        { "datos", datos },
        { "totCosteSS", P.totCosteSS }, { "totSalReal", P.totSalReal },
        { "totCoste", P.totCoste },
        { "plantilla", plantilla },
        { "totPersonas", P.totPersonas }, { "totBrutoReal", P.totBrutoReal },
        { "totSS", P.totSS }, { "totPlantilla", P.totPlantilla },
        { "salarioNetoMensualAnio1", P.salarioNetoMensualAnio1 },
    };

    // ---- Proyección (filas como en la hoja)
    const auto& Y = m_r.proyeccion;
    m_proyeccion = QVariantList{
        proyRow("Venta Receta",                  Y.ventaReceta),
        proyRow("Venta Libre",                   Y.ventaLibre),
        proyRow("VENTA TOTAL",                   Y.ventaTotal, "eur", true),
        proyRow("IPC aplicado",                  Y.ipcAplicado, "pct1"),
        proyRow("Coste Mercancía (Proveedores)", Y.costeMercancia),
        proyRow("M. Comercial %",                Y.margenComercial, "pct1"),
        proyRow("M. Comercial Bruto",            Y.mComBruto),
        proyRow("Reales Decretos",               Y.realesDecretos),
        proyRow("M. Comercial después de RDs",   Y.mComDespuesRD, "eur", true),
        proyRow("Alquiler Local Comercial",      Y.alquiler),
        proyRow("Gastos de Personal + S.S.",     Y.gastosPersonal),
        proyRow("% Gastos de Personal",          Y.pctGastoPersonal, "pct1"),
        proyRow("Cuota Autónomos (RETA)",        Y.cuotaAutonomos),
        proyRow("Otros Gastos",                  Y.otrosGastos),
        proyRow("Intereses de Deudas",           Y.intereses),
        proyRow("BENEFICIO FARMACIA",            Y.beneficio, "eur", true),
        proyRow("Pago Impuestos",                Y.pagoImpuestos),
        proyRow("LIQUIDEZ DESPUÉS DE IMP.",      Y.liquidez, "eur", true),
        proyRow("Devolución Capital al Banco",   Y.devCapitalBanco),
        proyRow("Devolución Cooperativa",        Y.devCooperativa),
        proyRow("SALARIO NETO ANUAL TITULAR",    Y.salarioNetoAnual, "eur", true),
        proyRow("SALARIO NETO MENSUAL TITULAR",  Y.salarioNetoMensual, "eur", true),
    };

    // ---- Impuestos (IRPF, v2)
    const auto& I = m_r.impuestos;
    QVariantList tramosList;
    static const char* tramoLabels[6] = {
        "Tramo 0 – 12.450 € (19%)", "Tramo 12.450 – 20.200 € (24%)",
        "Tramo 20.200 – 35.200 € (30%)", "Tramo 35.200 – 60.000 € (37%)",
        "Tramo 60.000 – 300.000 € (45%)", "Tramo > 300.000 € (47%)" };
    for (int t = 0; t < 6; ++t)
        tramosList << QVariantMap{
            { "label", QString::fromUtf8(tramoLabels[t]) },
            { "values", toList10(I.tramos[t]) } };
    m_impuestos = QVariantMap{
        { "fdc",              I.fdc },
        { "honorarios",       I.honorarios },
        { "ajd",              I.ajd },
        { "baseAmortizable",  I.baseAmortizable },
        { "costeLocal",       I.costeLocal },
        { "deduccionMinimo",  I.deduccionMinimo },
        { "beneficio",        toList10(I.beneficio) },
        { "amortLocal",       toList10(I.amortLocal) },
        { "pctAjustado",      toList10(I.pctAjustado) },
        { "amortFdC",         toList10(I.amortFdC) },
        { "baseImponible",    toList10(I.baseImponible) },
        { "cuotaEscala",      toList10(I.cuotaEscala) },
        { "pago",             toList10(I.pago) },
        { "tramos",           tramosList },
    };

    // ---- Análisis
    const auto& A = m_r.analisis;
    m_analisis = QVariantMap{
        { "inversionInicial",  toList3(A.inversionInicial) },
        { "valorVentaFdC",     toList3(A.valorVentaFdC) },
        { "valorVentaLocal",   toList3(A.valorVentaLocal) },
        { "existencias10",     toList3(A.existencias10) },
        { "fdcPendiente",      toList3(A.fdcPendiente) },
        { "deuda",             toList3(A.deuda) },
        { "patrimonioBruto",   toList3(A.patrimonioBruto) },
        { "patrimonioNeto",    toList3(A.patrimonioNeto) },
        { "cagr",              toList3(A.cagr) },
        { "tir",               toList3(A.tir) },
        { "liqMensual",        toList3(A.liqMensual) },
        { "devCapitalMensual", toList3(A.devCapitalMensual) },
        { "interesesMensual",  toList3(A.interesesMensual) },
        { "netoTitular",       toList3(A.netoTitular) },
        { "amortLocalAnual",   A.amortLocalAnual },
        { "benFarmacia",       toList10(A.benFarmacia) },
        { "pctAmortFdC",       toList10(A.pctAmortFdC) },
        { "amortFdC",          toList10(A.amortFdC) },
        { "amortLocal",        toList10(A.amortLocal) },
        { "baseImponible",     toList10(A.baseImponible) },
        { "fdcPendienteSim",   toList10(A.fdcPendienteSim) },
    };
}

// ---------------------------------------------------------------- comparación de escenarios
void Engine::anadirEscenarioComparacion()
{
    // Valores de la hoja Financiación para el grupo de la "vista completa":
    // no varían por año, así que se guardan como un valor fijo por escenario.
    const QVariantMap fin{
        { "liquidezAportada",    m_inputs.value(QStringLiteral("liquidezAportada")) },
        { "finBancariaFarmacia", m_financiacion.value(QStringLiteral("finBancariaFarmacia")) },
        { "finBancariaLocal",    m_financiacion.value(QStringLiteral("finBancariaLocal")) },
        { "pedidoInicial",       m_inputs.value(QStringLiteral("pedidoInicial")) },
        { "finPropiedades",      m_inputs.value(QStringLiteral("finPropiedades")) },
    };
    const QVariantMap esc{
        { "id",           QDateTime::currentMSecsSinceEpoch() },
        { "nombre",       QStringLiteral("Escenario %1").arg(m_escenariosComparacion.size() + 1) },
        { "proyeccion",   m_proyeccion },
        { "financiacion", fin },
    };
    m_escenariosComparacion.append(esc);
    guardarEnDisco();
    emit escenariosComparacionChanged();
}

void Engine::quitarEscenarioComparacion(int index)
{
    if (index < 0 || index >= m_escenariosComparacion.size())
        return;
    m_escenariosComparacion.removeAt(index);
    guardarEnDisco();
    emit escenariosComparacionChanged();
}

// Pivota "escenario -> filas(concepto, 10 años)" a "filas(concepto) -> escenarios",
// quedándonos con el valor del año pedido de cada escenario.
QVariantList Engine::comparacionAnio(int anio) const
{
    QVariantList filas;
    if (m_escenariosComparacion.isEmpty())
        return filas;

    const QVariantList plantilla = m_escenariosComparacion.first().toMap()
                                        .value(QStringLiteral("proyeccion")).toList();
    for (int r = 0; r < plantilla.size(); ++r) {
        const QVariantMap filaPlantilla = plantilla[r].toMap();
        QVariantList valores;
        for (const QVariant& ev : m_escenariosComparacion) {
            const QVariantList filasEsc = ev.toMap().value(QStringLiteral("proyeccion")).toList();
            double v = 0;
            if (r < filasEsc.size()) {
                const QVariantList vals = filasEsc[r].toMap().value(QStringLiteral("values")).toList();
                if (anio >= 0 && anio < vals.size())
                    v = vals[anio].toDouble();
            }
            valores << v;
        }
        filas << QVariantMap{
            { "label", filaPlantilla.value(QStringLiteral("label")) },
            { "values", valores },
            { "fmt", filaPlantilla.value(QStringLiteral("fmt")) },
            { "bold", filaPlantilla.value(QStringLiteral("bold")) },
        };
    }
    return filas;
}

QVariantList Engine::comparacionFinanciacion() const
{
    static const struct { const char* clave; const char* label; } kFilas[] = {
        { "liquidezAportada",    "Liquidez aportada" },
        { "finBancariaFarmacia", "Financiación farmacia" },
        { "finBancariaLocal",    "Financiación local" },
        { "pedidoInicial",       "Cooperativa" },
        { "finPropiedades",      "Hipoteca propiedad" },
    };

    QVariantList filas;
    if (m_escenariosComparacion.isEmpty())
        return filas;

    for (const auto& f : kFilas) {
        const QString clave = QString::fromUtf8(f.clave);
        QVariantList valores;
        for (const QVariant& ev : m_escenariosComparacion) {
            const QVariantMap fin = ev.toMap().value(QStringLiteral("financiacion")).toMap();
            valores << fin.value(clave, 0.0);
        }
        filas << QVariantMap{
            { "label", QString::fromUtf8(f.label) },
            { "values", valores },
            { "fmt", QStringLiteral("eur") },
            { "bold", false },
        };
    }
    return filas;
}

// ---------------------------------------------------------------- simulación aislada
// Calcula una proyección "de bolsillo" con algunas entradas sustituidas, a
// partir de una copia de las entradas actuales. No toca m_in ni dispara
// recalc()/guardarEnDisco(): así los cambios de la hoja "Simulación simple"
// no se propagan a las demás hojas ni se guardan.
QVariantMap Engine::simularSimple(const QVariantMap& cambios) const
{
    sim::Inputs copia = m_in;
    if (cambios.contains(QStringLiteral("ventaReceta")))
        copia.ventaReceta = cambios[QStringLiteral("ventaReceta")].toDouble();
    if (cambios.contains(QStringLiteral("ventaLibre")))
        copia.ventaLibre = cambios[QStringLiteral("ventaLibre")].toDouble();
    if (cambios.contains(QStringLiteral("existencias")))
        copia.existencias = cambios[QStringLiteral("existencias")].toDouble();
    if (cambios.contains(QStringLiteral("liquidezAportada")))
        copia.liquidezAportada = cambios[QStringLiteral("liquidezAportada")].toDouble();
    if (cambios.contains(QStringLiteral("finPropiedades")))
        copia.finPropiedades = cambios[QStringLiteral("finPropiedades")].toDouble();
    if (cambios.contains(QStringLiteral("pedidoInicial")))
        copia.pedidoInicial = cambios[QStringLiteral("pedidoInicial")].toDouble();
    if (cambios.contains(QStringLiteral("alquilerLocal")))
        copia.alquilerLocal = cambios[QStringLiteral("alquilerLocal")].toDouble();

    const sim::Results r = sim::compute(copia);
    const auto& Y = r.proyeccion;
    const QVariantList proyeccion{
        proyRow("Venta total",                   Y.ventaTotal, "eur"),
        proyRow("M. Comercial después de RDs",   Y.mComDespuesRD, "eur"),
        proyRow("Beneficio farmacia",            Y.beneficio, "eur"),
        proyRow("Liquidez después de imp.",      Y.liquidez, "eur"),
        proyRow("Salario neto anual titular",    Y.salarioNetoAnual, "eur", true),
        proyRow("Salario neto mensual titular",  Y.salarioNetoMensual, "eur", true),
    };
    return QVariantMap{
        { "ventaTotal", r.datosBase.ventaTotal },
        { "proyeccion", proyeccion },
        { "minimoLiquidez", r.financiacion.minimoLiquidez },
        { "liquidezInvalida", r.financiacion.liquidezInvalida },
    };
}
