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
    void compute_familyAmortGracePeriod();
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
    // Tabla de tramos por defecto (Inputs::rdBrackets, editable desde
    // Configuración; RD 823/2008 art. 2.5), sobre la media MENSUAL de venta
    // de receta. deduccionMensual = base + (mensual-desde)*pct.
    const Inputs in;
    const auto& tabla = in.rdBrackets;
    QCOMPARE(calculateRdDeduction(0.0, tabla), 0.0);

    // mensual = 40000 -> tramo [37500, 45000) base=0 pct=0,078
    {
        const double anual = 40000.0 * 12.0;
        const double expected = (0.0 + (40000.0 - 37500.0) * 0.078) * 12.0;
        QVERIFY(std::fabs(calculateRdDeduction(anual, tabla) - expected) < 1e-6);
    }

    // Límite exacto del tramo: mensual = 45000 -> cae en el tramo [45000, 58345.61)
    {
        const double anual = 45000.0 * 12.0;
        const double expected = 585.00 * 12.0; // (mensual-desde)=0
        QVERIFY(std::fabs(calculateRdDeduction(anual, tabla) - expected) < 1e-6);
    }

    // Dentro del tramo [45000, 58345.61) con offset no nulo, para ejercitar
    // también el pct del tramo (no solo su base).
    {
        const double anual = 50000.0 * 12.0;
        const double expected = (585.00 + (50000.0 - 45000.0) * 0.091) * 12.0;
        QVERIFY(std::fabs(calculateRdDeduction(anual, tabla) - expected) < 1e-6);
    }

    // Tramo más alto (>=600000 mensual)
    {
        const double anual = 650000.0 * 12.0;
        const double expected = (89081.17 + (650000.0 - 600000.0) * 0.200) * 12.0;
        QVERIFY(std::fabs(calculateRdDeduction(anual, tabla) - expected) < 1e-6);
    }
}

void TestSimCore::cuotaAutonomos_matchesTramoTable()
{
    // Tabla de tramos por defecto (Inputs::retaBrackets, editable desde Configuración).
    const Inputs in;
    const auto& tabla = in.retaBrackets;

    // Beneficio <= 0 -> tramo más bajo (200/205,88 €/mes con MEI).
    QCOMPARE(calculateSelfEmployedQuota(0.0, tabla), 205.88 * 12.0);
    QCOMPARE(calculateSelfEmployedQuota(-50000.0, tabla), 205.88 * 12.0);

    // 1500 €/mes de beneficio -> tramo [1300, 1500) -> 302,64 €/mes.
    QVERIFY(std::fabs(calculateSelfEmployedQuota(1500.0 * 12.0, tabla) - 302.64 * 12.0) < 1e-6);

    // Límite exacto del tramo [1500, 1700): 1700 €/mes de beneficio.
    QVERIFY(std::fabs(calculateSelfEmployedQuota(1700.0 * 12.0, tabla) - 319.12 * 12.0) < 1e-6);

    // Tramo más alto (>= 6000 €/mes de beneficio).
    QVERIFY(std::fabs(calculateSelfEmployedQuota(650000.0, tabla) - 607.31 * 12.0) < 1e-6);
}

void TestSimCore::compute_datosBaseInvariants()
{
    const Inputs in; // valores por defecto = valores del Excel
    const Results r = compute(in);
    const auto& D = r.baseData;

    QVERIFY(std::fabs(D.totalSales - (in.prescriptionSales + in.otcSales)) < 1e-6);
    QVERIFY(std::fabs(D.grossMargin - D.totalSales * in.marginPct) < 1e-6);
    QVERIFY(std::fabs((D.costOfGoods + D.grossMargin) - D.totalSales) < 1e-6);
    QVERIFY(std::fabs(D.totalStaffCost
                       - (D.staffCost + D.socialSecurity + r.projection.selfEmployedQuota[0])) < 1e-6);
}

void TestSimCore::compute_amortBancoFullyAmortizes()
{
    const Inputs in;
    const Results r = compute(in);
    const auto& a = r.bankAmort;

    QCOMPARE(int(a.rows.size()), 300);
    QCOMPARE(a.numPayments, in.bankTermYears * 12);

    // el capital de cada fila es negativo (reduce el saldo pendiente); su suma
    // en valor absoluto debe cubrir el principal cuando el préstamo se liquida.
    double capitalPagado = 0;
    for (int i = 0; i < a.numPayments; ++i)
        capitalPagado += a.rows[i].principalPaid;

    QVERIFY(std::fabs(-capitalPagado - a.principal) < a.principal * 1e-6);
    QVERIFY(std::fabs(a.rows[a.numPayments - 1].endingBalance) < a.principal * 1e-6);
}

void TestSimCore::compute_familyAmortGracePeriod()
{
    Inputs in;
    in.familyContribution = 100000.0;
    in.familyRate         = 0.03;
    in.familyTermYears    = 10;
    in.familyGraceMonths  = 12;

    const Results r = compute(in);
    const auto& a = r.familyAmort;

    QCOMPARE(int(a.rows.size()), 300);
    QCOMPARE(a.numPayments, in.familyTermYears * 12);

    // Durante la carencia (primeros 12 meses): ni capital ni interés se pagan.
    for (int i = 0; i < 12; ++i) {
        QVERIFY(std::fabs(a.rows[i].principalPaid) < 1e-9);
        QVERIFY(std::fabs(a.rows[i].interest) < 1e-9);
        QVERIFY(std::fabs(a.rows[i].payment) < 1e-9);
        QVERIFY(std::fabs(a.rows[i].endingBalance - a.principal) < 1e-6);
    }

    // Tras la carencia, el préstamo se sigue amortizando por completo dentro del plazo.
    double capitalPagado = 0;
    for (int i = 0; i < a.numPayments; ++i)
        capitalPagado += a.rows[i].principalPaid;
    QVERIFY(std::fabs(-capitalPagado - a.principal) < a.principal * 1e-6);
    QVERIFY(std::fabs(a.rows[a.numPayments - 1].endingBalance) < a.principal * 1e-6);
}

void TestSimCore::compute_personalTotalsSumRows()
{
    const Inputs in;
    const Results r = compute(in);
    const auto& P = r.staff;

    double sumaSS = 0, sumaReal = 0, sumaCoste = 0;
    for (const auto& row : P.byRole) {
        sumaSS    += row.socialSecurityCost;
        sumaReal  += row.actualSalary;
        sumaCoste += row.totalCost;
    }
    QVERIFY(std::fabs(sumaSS - P.totalSocialSecurityCost) < 1e-6);
    QVERIFY(std::fabs(sumaReal - P.totalActualSalary) < 1e-6);
    QVERIFY(std::fabs(sumaCoste - P.totalCost) < 1e-6);

    double personas = 0, brutoReal = 0, ss = 0, plantilla = 0;
    for (const auto& row : P.headcountPlan) {
        personas  += row.headcount;
        brutoReal += row.actualGross;
        ss        += row.socialSecurityCost;
        plantilla += row.totalCost;
    }
    QVERIFY(std::fabs(personas - P.totalHeadcount) < 1e-9);
    QVERIFY(std::fabs(brutoReal - P.totalActualGross) < 1e-6);
    QVERIFY(std::fabs(ss - P.totalSocialSecurity) < 1e-6);
    QVERIFY(std::fabs(plantilla - P.totalHeadcountCost) < 1e-6);
}

void TestSimCore::compute_goldenValues()
{
    // Foto de referencia de compute(Inputs{}) tomada al escribir este test.
    // Si se cambia una fórmula a propósito, hay que regenerar estos valores
    // (no re-teclearlos a ciegas para que el test "pase").
    const Inputs in;
    const Results r = compute(in);

    QVERIFY(std::fabs(r.baseData.profitBeforeTax - 169851.78448) < 1e-3);
    QVERIFY(std::fabs(r.financing.totalInvestment - 2512177.0505050505) < 1e-2);
    QVERIFY(std::fabs(r.bankAmort.monthlyPayment - (-10937.6465475271)) < 1e-6);
    QVERIFY(std::fabs(r.projection.profit[9] - 197557.796236802) < 1e-4);
    QVERIFY(std::fabs(r.taxes.payment[0] - 0.0) < 1e-6);
    QVERIFY(std::fabs(r.analysis.irr[1] - 0.1610866688) < 1e-6);
}

QTEST_APPLESS_MAIN(TestSimCore)
#include "test_simcore.moc"
