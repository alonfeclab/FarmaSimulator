// Tests unitarios del motor de cálculo puro (simcore.cpp), corridos con Qt Test / CTest.
#include <QtTest>
#include <cmath>
#include "simcore.h"

using namespace sim;

namespace {
double npv(double rate, const std::vector<double>& cf)
{
    double v = 0;
    for (size_t i = 0; i < cf.size(); ++i)
        v += cf[i] / std::pow(1.0 + rate, double(i));
    return v;
}
}

class TestSimCore : public QObject
{
    Q_OBJECT

private slots:
    void pmt_matchesClosedFormAnnuity();
    void pmt_edgeCases();
    void irr_npvIsZeroAtSolution();
    void realesDecretos_matchesTramoTable();
    void cuotaAutonomos_matchesTramoTable();
    void compute_datosBaseInvariants();
    void compute_amortBancoFullyAmortizes();
    void compute_personalTotalsSumRows();
    void compute_goldenValues();
};

void TestSimCore::pmt_matchesClosedFormAnnuity()
{
    // Préstamo de 100.000 al 6% anual (0,5% mensual) a 10 años (120 pagos).
    const double r = 0.06 / 12.0;
    const int    n = 120;
    const double p = 100000.0;

    const double expected = -p * r / (1.0 - std::pow(1.0 + r, -double(n)));
    const double actual   = pmt(r, n, p);

    QVERIFY(std::fabs(actual - expected) < std::fabs(expected) * 1e-9);
}

void TestSimCore::pmt_edgeCases()
{
    QCOMPARE(pmt(0.03, 0, 100000.0), 0.0);
    QCOMPARE(pmt(0.03, -5, 100000.0), 0.0);
    QCOMPARE(pmt(0.0, 100, 100000.0), -1000.0); // -p/n
}

void TestSimCore::irr_npvIsZeroAtSolution()
{
    const std::vector<std::vector<double>> series = {
        { -1000, 200, 300, 300, 500 },
        { -400000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 450000 },
        { -100, 110 },
    };

    for (const auto& cf : series) {
        const double r = irr(cf);
        QVERIFY(std::fabs(npv(r, cf)) < 1e-6);
    }
}

void TestSimCore::realesDecretos_matchesTramoTable()
{
    // Tabla de tramos de simcore.cpp (RD 823/2008 art. 2.5), sobre la media
    // MENSUAL de venta de receta. deduccionMensual = base + (mensual-desde)*pct.
    QCOMPARE(calcularRealesDecretos(0.0), 0.0);

    // mensual = 40000 -> tramo [37500, 45000) base=0 pct=0,078
    {
        const double anual = 40000.0 * 12.0;
        const double expected = (0.0 + (40000.0 - 37500.0) * 0.078) * 12.0;
        QVERIFY(std::fabs(calcularRealesDecretos(anual) - expected) < 1e-6);
    }

    // Límite exacto del tramo: mensual = 45000 -> cae en el tramo [45000, 58345.61)
    {
        const double anual = 45000.0 * 12.0;
        const double expected = 585.00 * 12.0; // (mensual-desde)=0
        QVERIFY(std::fabs(calcularRealesDecretos(anual) - expected) < 1e-6);
    }

    // Dentro del tramo [45000, 58345.61) con offset no nulo, para ejercitar
    // también el pct del tramo (no solo su base).
    {
        const double anual = 50000.0 * 12.0;
        const double expected = (585.00 + (50000.0 - 45000.0) * 0.091) * 12.0;
        QVERIFY(std::fabs(calcularRealesDecretos(anual) - expected) < 1e-6);
    }

    // Tramo más alto (>=600000 mensual)
    {
        const double anual = 650000.0 * 12.0;
        const double expected = (89081.17 + (650000.0 - 600000.0) * 0.200) * 12.0;
        QVERIFY(std::fabs(calcularRealesDecretos(anual) - expected) < 1e-6);
    }
}

void TestSimCore::cuotaAutonomos_matchesTramoTable()
{
    // Beneficio <= 0 -> tramo más bajo (200/205,88 €/mes con MEI).
    QCOMPARE(calcularCuotaAutonomos(0.0), 205.88 * 12.0);
    QCOMPARE(calcularCuotaAutonomos(-50000.0), 205.88 * 12.0);

    // 1500 €/mes de beneficio -> tramo [1300, 1500) -> 302,64 €/mes.
    QVERIFY(std::fabs(calcularCuotaAutonomos(1500.0 * 12.0) - 302.64 * 12.0) < 1e-6);

    // Límite exacto del tramo [1500, 1700): 1700 €/mes de beneficio.
    QVERIFY(std::fabs(calcularCuotaAutonomos(1700.0 * 12.0) - 319.12 * 12.0) < 1e-6);

    // Tramo más alto (>= 6000 €/mes de beneficio).
    QVERIFY(std::fabs(calcularCuotaAutonomos(650000.0) - 607.31 * 12.0) < 1e-6);
}

void TestSimCore::compute_datosBaseInvariants()
{
    const Inputs in; // valores por defecto = valores del Excel
    const Results r = compute(in);
    const auto& D = r.datosBase;

    QVERIFY(std::fabs(D.ventaTotal - (in.ventaReceta + in.ventaLibre)) < 1e-6);
    QVERIFY(std::fabs(D.mComBruto - D.ventaTotal * in.margenPct) < 1e-6);
    QVERIFY(std::fabs((D.costeMercancia + D.mComBruto) - D.ventaTotal) < 1e-6);
    QVERIFY(std::fabs(D.totalGastosPersonal
                       - (D.gastosPersonal + D.seguridadSocial + r.proyeccion.cuotaAutonomos[0])) < 1e-6);
}

void TestSimCore::compute_amortBancoFullyAmortizes()
{
    const Inputs in;
    const Results r = compute(in);
    const auto& a = r.amortBanco;

    QCOMPARE(int(a.rows.size()), 300);
    QCOMPARE(a.numPagos, in.plazoBanco * 12);

    // capital de cada fila es negativo (reduce el saldo pendiente); su suma en
    // valor absoluto debe cubrir el principal cuando el préstamo se liquida.
    double capitalPagado = 0;
    for (int i = 0; i < a.numPagos; ++i)
        capitalPagado += a.rows[i].capital;

    QVERIFY(std::fabs(-capitalPagado - a.principal) < a.principal * 1e-6);
    QVERIFY(std::fabs(a.rows[a.numPagos - 1].saldoFin) < a.principal * 1e-6);
}

void TestSimCore::compute_personalTotalsSumRows()
{
    const Inputs in;
    const Results r = compute(in);
    const auto& P = r.personal;

    double sumaSS = 0, sumaReal = 0, sumaCoste = 0;
    for (const auto& row : P.datos) {
        sumaSS    += row.costeSS;
        sumaReal  += row.salReal;
        sumaCoste += row.costeTotal;
    }
    QVERIFY(std::fabs(sumaSS - P.totCosteSS) < 1e-6);
    QVERIFY(std::fabs(sumaReal - P.totSalReal) < 1e-6);
    QVERIFY(std::fabs(sumaCoste - P.totCoste) < 1e-6);

    double personas = 0, brutoReal = 0, ss = 0, plantilla = 0;
    for (const auto& row : P.plantilla) {
        personas  += row.personas;
        brutoReal += row.brutoReal;
        ss        += row.costeSS;
        plantilla += row.costeTotal;
    }
    QVERIFY(std::fabs(personas - P.totPersonas) < 1e-9);
    QVERIFY(std::fabs(brutoReal - P.totBrutoReal) < 1e-6);
    QVERIFY(std::fabs(ss - P.totSS) < 1e-6);
    QVERIFY(std::fabs(plantilla - P.totPlantilla) < 1e-6);
}

void TestSimCore::compute_goldenValues()
{
    // Foto de referencia de compute(Inputs{}) tomada al escribir este test.
    // Si se cambia una fórmula a propósito, hay que regenerar estos valores
    // (no re-teclearlos a ciegas para que el test "pase").
    const Inputs in;
    const Results r = compute(in);

    QVERIFY(std::fabs(r.datosBase.beneficioAntesImp - 169415.46448) < 1e-3);
    QVERIFY(std::fabs(r.financiacion.totalInversion - 2489296.48) < 1e-2);
    QVERIFY(std::fabs(r.amortBanco.pagoMensual - (-7705.0049051490)) < 1e-6);
    QVERIFY(std::fabs(r.proyeccion.beneficio[9] - 202272.4569497254) < 1e-4);
    QVERIFY(std::fabs(r.impuestos.pago[0] - 0.0) < 1e-6);
    QVERIFY(std::fabs(r.analisis.tir[1] - 0.1777760444) < 1e-6);
}

QTEST_APPLESS_MAIN(TestSimCore)
#include "test_simcore.moc"
