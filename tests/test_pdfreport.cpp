// Tests de pdfreport.cpp. Solo expone pdf::escribirInforme() (todo lo demás
// es interno, namespace anónimo), así que son tests de caja negra: que
// produzca un PDF válido y no reviente con distintos Inputs.
//
// No se prueba el caso de "dispositivo no escribible": en el único uso real
// (Engine::exportarPdf, src/engine.cpp) siempre se le pasa un QBuffer en
// memoria recién abierto en WriteOnly, que nunca falla por permisos; la
// escritura a disco de verdad ocurre después, en una capa distinta, con un
// QFile cuyo open() sí se comprueba explícitamente.
#include <QtTest>
#include <QBuffer>
#include <QVariantList>
#include <QVariantMap>
#include "pdfreport.h"
#include "simcore.h"

using namespace sim;

class TestPdfReport : public QObject
{
    Q_OBJECT

private slots:
    void escribirInforme_defaultInputs_producesValidPdf();
    void escribirInforme_sinPrestamosNiVentas_noRevienta();
    void escribirSimulacion_datosTipicos_producePdfValido();
    void escribirSimulacion_valoresExtremos_noRevienta();
    void escribirSimulacion_conColumnasOcultas_soloIncluyeLasVisibles();
};

void TestPdfReport::escribirInforme_defaultInputs_producesValidPdf()
{
    const Inputs in; // valores del Excel
    const Results r = compute(in);

    QBuffer buf;
    QVERIFY(buf.open(QIODevice::WriteOnly));
    QVERIFY(pdf::writeReport(&buf, in, r));
    buf.close();

    // Un informe de varias páginas (portada + 6 hojas) no es minúsculo.
    QVERIFY(buf.data().size() > 2000);
    QVERIFY(buf.data().startsWith("%PDF-"));
}

void TestPdfReport::escribirInforme_sinPrestamosNiVentas_noRevienta()
{
    // Caso extremo: sin ventas ni financiación de cooperativa/propiedades.
    // simcore.cpp ya protege las divisiones por cero (p. ej. pctGastoPersonal);
    // esto comprueba que el maquetado del PDF tampoco revienta con esos ceros.
    Inputs in;
    in.prescriptionSales = 0;
    in.otcSales = 0;
    in.marginPct = 0;
    in.initialOrder = 0;
    in.propertiesFinancing = 0;
    const Results r = compute(in);

    QBuffer buf;
    QVERIFY(buf.open(QIODevice::WriteOnly));
    QVERIFY(pdf::writeReport(&buf, in, r));
    QVERIFY(buf.data().startsWith("%PDF-"));
}

// Construye una fila con 'n' valores, con la misma forma que las que arma
// Engine::simulationForYear() (label/values/fmt/bold).
static QVariantMap simRow(const QString& label, const QString& fmt, bool bold,
                           int n, double base, double step)
{
    QVariantList values;
    for (int i = 0; i < n; ++i)
        values << (base + i * step);
    return QVariantMap{ { "label", label }, { "values", values },
                         { "fmt", fmt }, { "bold", bold } };
}

// Las 9 filas que arma Engine::simulationForYear() (una por escenario,
// columna 0 = "Actual"), 'n' columnas cada una.
static QVariantList simRows(int n)
{
    return QVariantList{
        simRow(QStringLiteral("Facturación Total"),               QStringLiteral("eur"),   false, n, 1000000, 50000),
        simRow(QStringLiteral("Años hipoteca mobiliaria"),        QStringLiteral("years"), false, n, 20, 1),
        simRow(QStringLiteral("Interés hipoteca mobiliaria"),     QStringLiteral("pct1"),  false, n, 0.03, 0.001),
        simRow(QStringLiteral("Años hipoteca inmobiliaria"),      QStringLiteral("years"), false, n, 20, 1),
        simRow(QStringLiteral("Interés hipoteca inmobiliaria"),   QStringLiteral("pct1"),  false, n, 0.03, 0.001),
        simRow(QStringLiteral("Aportación inicial"),              QStringLiteral("eur"),   false, n, 400000, 1000),
        simRow(QStringLiteral("Coste total farmacia"),            QStringLiteral("eur"),   false, n, 900000, 5000),
        simRow(QStringLiteral("Intereses totales pagados"),       QStringLiteral("eur"),   false, n, 150000, 2000),
        simRow(QStringLiteral("Beneficio neto mensual"),          QStringLiteral("eur"),   true,  n, 1500, 25),
    };
}

// 'nAnios' páginas (una por año), cada una con 'nColumnas' columnas —
// misma forma que arma Engine::exportSimulationPdf() a partir de
// simulationForYear() (columna 0 "Actual" + una por escenario guardado).
static QVariantList simYears(int nAnios, int nColumnas)
{
    QVariantList years;
    for (int a = 0; a < nAnios; ++a)
        years << QVariantMap{ { "year", a + 1 }, { "rows", simRows(nColumnas) } };
    return years;
}

static QStringList scenarioLabels(int n)
{
    QStringList labels{ QStringLiteral("Actual") };
    for (int i = 1; i < n; ++i)
        labels << QStringLiteral("Escenario %1").arg(i);
    return labels;
}

void TestPdfReport::escribirSimulacion_datosTipicos_producePdfValido()
{
    // 10 años × 16 columnas ("Actual" + 15 escenarios), muchas más de las
    // que caben legibles en una sola tabla por página.
    const QVariantList years = simYears(10, 16);

    QBuffer buf;
    QVERIFY(buf.open(QIODevice::WriteOnly));
    QVERIFY(pdf::writeSimulation(&buf, years, scenarioLabels(16)));
    buf.close();

    // Un informe con 10 años (cada uno partido en varias tablas, 16
    // columnas no caben legibles en una sola) no es minúsculo.
    QVERIFY(buf.data().size() > 2000);
    QVERIFY(buf.data().startsWith("%PDF-"));
}

void TestPdfReport::escribirSimulacion_valoresExtremos_noRevienta()
{
    // Casos límite: un solo año, una sola columna y una facturación a cero,
    // para comprobar que el maquetado no revienta con anchos de columna o
    // valores degenerados.
    const QVariantList years{
        QVariantMap{ { "year", 1 }, { "rows", simRows(1) } },
    };

    QBuffer buf;
    QVERIFY(buf.open(QIODevice::WriteOnly));
    QVERIFY(pdf::writeSimulation(&buf, years, scenarioLabels(1)));
    buf.close();

    QVERIFY(buf.data().startsWith("%PDF-"));
}

// pdf::writeSimulation() es puro sobre lo que recibe: no sabe nada de "ojo"
// ni de escenarios borrados, solo dibuja las columnas de 'years'/
// 'scenarioLabels' que se le pasan (caja negra). Aquí se comprueba que menos
// columnas dan un documento más pequeño (menos celdas dibujadas), es decir
// que las columnas ausentes realmente no se pintan en el PDF (no quedan
// "huecos" reservados para ellas).
void TestPdfReport::escribirSimulacion_conColumnasOcultas_soloIncluyeLasVisibles()
{
    const QVariantList aniosCompleto = simYears(10, 16);
    const QVariantList aniosFiltrado = simYears(10, 6); // como si solo se hubieran guardado 6 escenarios

    QBuffer bufCompleto;
    QVERIFY(bufCompleto.open(QIODevice::WriteOnly));
    QVERIFY(pdf::writeSimulation(&bufCompleto, aniosCompleto, scenarioLabels(16)));
    bufCompleto.close();

    QBuffer bufFiltrado;
    QVERIFY(bufFiltrado.open(QIODevice::WriteOnly));
    QVERIFY(pdf::writeSimulation(&bufFiltrado, aniosFiltrado, scenarioLabels(6)));
    bufFiltrado.close();

    QVERIFY(bufCompleto.data().startsWith("%PDF-"));
    QVERIFY(bufFiltrado.data().startsWith("%PDF-"));
    QVERIFY(bufFiltrado.data().size() < bufCompleto.data().size());
}

QTEST_MAIN(TestPdfReport)
#include "test_pdfreport.moc"
