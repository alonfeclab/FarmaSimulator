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
#include "pdfreport.h"
#include "simcore.h"

using namespace sim;

class TestPdfReport : public QObject
{
    Q_OBJECT

private slots:
    void escribirInforme_defaultInputs_producesValidPdf();
    void escribirInforme_sinPrestamosNiVentas_noRevienta();
};

void TestPdfReport::escribirInforme_defaultInputs_producesValidPdf()
{
    const Inputs in; // valores del Excel
    const Results r = compute(in);

    QBuffer buf;
    QVERIFY(buf.open(QIODevice::WriteOnly));
    QVERIFY(pdf::escribirInforme(&buf, in, r));
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
    in.ventaReceta = 0;
    in.ventaLibre = 0;
    in.margenPct = 0;
    in.pedidoInicial = 0;
    in.finPropiedades = 0;
    const Results r = compute(in);

    QBuffer buf;
    QVERIFY(buf.open(QIODevice::WriteOnly));
    QVERIFY(pdf::escribirInforme(&buf, in, r));
    QVERIFY(buf.data().startsWith("%PDF-"));
}

QTEST_MAIN(TestPdfReport)
#include "test_pdfreport.moc"
