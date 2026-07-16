#include "simcore.h"
#include <cmath>

namespace sim {

double pmt(double r, int n, double p)
{
    if (n <= 0) return 0;
    if (r == 0.0) return -p / n;
    // exp(-n*log1p(r)) instead of pow(): matches Excel's PMT bit-for-bit,
    // which matters at the edge of the schedule's last payment.
    return -p * r / (1.0 - std::exp(-double(n) * std::log1p(r)));
}

// NPV for IRR
static double npv(double rate, const std::vector<double>& cf)
{
    double v = 0;
    for (size_t i = 0; i < cf.size(); ++i)
        v += cf[i] / std::pow(1.0 + rate, double(i));
    return v;
}

double irr(const std::vector<double>& cf, double guess)
{
    // Newton-Raphson with numerical derivative and bisection fallback
    double r = guess;
    for (int it = 0; it < 100; ++it) {
        const double f  = npv(r, cf);
        const double h  = 1e-7;
        const double df = (npv(r + h, cf) - f) / h;
        if (std::fabs(df) < 1e-12) break;
        const double r2 = r - f / df;
        if (std::fabs(r2 - r) < 1e-10) return r2;
        r = (r2 <= -1.0) ? (r - 1.0) / 2.0 : r2; // never cross -100%
    }
    // Bisection
    double lo = -0.9999, hi = 10.0;
    double flo = npv(lo, cf), fhi = npv(hi, cf);
    if (flo * fhi > 0) return r; // no sign change: return Newton's result
    for (int it = 0; it < 200; ++it) {
        const double mid = (lo + hi) / 2.0;
        const double fm = npv(mid, cf);
        if (std::fabs(fm) < 1e-9) return mid;
        if (flo * fm < 0) { hi = mid; fhi = fm; } else { lo = mid; flo = fm; }
    }
    return (lo + hi) / 2.0;
}

// Amortization schedule: ALWAYS 300 rows (like the Excel, rows 18-317).
// Once the loan is paid off the payment drops to 0 but interest keeps being
// computed on the (slightly negative) balance — exact replica.
static AmortResult amortize(double principal, double annualRate, int termYears,
                             int startYear, int startMonth)
{
    AmortResult a;
    a.principal   = principal;
    a.annualRate  = annualRate;
    a.termYears   = termYears;
    a.numPayments = termYears * 12;

    // Exact replica of the Excel: the payment uses F3/12, but the interest of
    // each row uses G3=F3*100/12 and then /100 (they differ in the last bit,
    // and that determines the schedule's behavior after the last payment).
    const double g3 = annualRate * 100.0 / 12.0;                              // G3
    const double fixedPayment = pmt(annualRate / 12.0, a.numPayments, principal); // F18

    a.monthlyPayment = fixedPayment;
    a.annualPayment  = fixedPayment * 12.0;

    a.rows.reserve(300);
    double balance = principal;
    int year = startYear, month = startMonth;
    for (int m = 1; m <= 300; ++m) {
        AmortRow r;
        r.paymentNum      = m;
        r.year            = year;
        r.month           = month;
        r.startingBalance = balance;
        r.payment  = (m == 1) ? fixedPayment : (balance <= 0.0 ? 0.0 : fixedPayment); // F19: IF(E<=0;0;$F$18)
        r.interest = (-balance * g3) / 100.0; // H: -E*$G$3/100
        r.principalPaid = r.payment - r.interest;  // G: F-H
        r.endingBalance = balance + r.principalPaid; // I: E+G
        a.totalInterest += r.interest;
        a.rows.push_back(r);
        balance = r.endingBalance;
        if (++month > 12) { month = 1; ++year; }
    }
    a.totalCost = principal - a.totalInterest; // F10
    return a;
}

double calculateRdDeduction(double annualPrescriptionSales, const std::array<RdBracket,9>& table)
{
    const double monthly = annualPrescriptionSales / 12.0;
    const RdBracket* bracket = &table[0];
    for (const auto& t : table) {
        if (monthly < t.from) break;
        bracket = &t;
    }
    const double monthlyDeduction = bracket->base + (monthly - bracket->from) * bracket->pct;
    return monthlyDeduction * 12.0;
}

double calculateSelfEmployedQuota(double annualProfit, const std::array<RetaBracket,15>& table)
{
    const double monthly = std::max(0.0, annualProfit) / 12.0;
    const RetaBracket* bracket = &table[0];
    for (const auto& t : table) {
        if (monthly < t.from) break;
        bracket = &t;
    }
    return bracket->monthlyQuota * 12.0;
}

// Commercial margin, "Optimistic" scenario: editable by the user (year 1,
// year 2, year 3 onward — constant from year 3).
static std::array<double,10> optimisticMarginSeries(const Inputs& in)
{
    std::array<double,10> a;
    a.fill(in.optimisticMarginYear3);
    a[0] = in.optimisticMarginYear1;
    a[1] = in.optimisticMarginYear2;
    return a;
}

Results compute(const Inputs& in)
{
    Results R;

    // ================================================= Personal (section 1)
    {
        auto& P = R.staff;
        const double b[3] = { in.pharmacistSalary, in.assistantSalary, in.technicianSalary };
        const double c[3] = { in.pharmacistFte, in.assistantFte, in.technicianFte };
        for (int i = 0; i < 3; ++i) {
            auto& r = P.byRole[i];
            r.grossFte  = b[i];
            r.fte       = c[i];
            r.socialSecurityPct  = in.socialSecurityPct;    // D7=D8=$D$6
            r.socialSecurityCost = b[i] * c[i] * in.socialSecurityPct; // E = B*C*D
            r.actualSalary        = b[i] * c[i];             // F = B*C
            r.totalCost = r.socialSecurityCost + r.actualSalary; // G = E+F
            P.totalSocialSecurityCost += r.socialSecurityCost;
            P.totalActualSalary       += r.actualSalary;
            P.totalCost                += r.totalCost;
        }

        // Recommended headcount (rows 13-16)
        // D13=B6 ; D14=B6*(1+H6) ; D15=B7*(1+H6) ; D16=B8*(1+H6)
        const double d[4] = { in.pharmacistSalary,
                              in.pharmacistSalary * (1.0 + in.raisePct),
                              in.assistantSalary  * (1.0 + in.raisePct),
                              in.technicianSalary * (1.0 + in.raisePct) };
        for (int i = 0; i < 4; ++i) {
            auto& r = P.headcountPlan[i];
            r.fte       = in.staffFte[i];
            r.headcount = (i == 0) ? 1.0 : in.staffCount[i]; // Owner: always 1 person
            r.grossFte  = d[i];
            if (i == 0) { // Owner: E13..H13 = 0 (compensated via profit)
                r.actualGross = r.socialSecurityCost = r.costPerPerson = r.totalCost = 0;
            } else {
                r.actualGross      = d[i] * r.fte * r.headcount;      // E = D*B*C
                r.socialSecurityCost = r.actualGross * in.socialSecurityPct; // F = E*$D$6
                r.costPerPerson     = r.actualGross * (1.0 + in.socialSecurityPct); // G = E*(1+$D$6)
                r.totalCost         = r.costPerPerson;               // H = G
            }
            P.totalHeadcount    += r.headcount;    // C17
            P.totalActualGross  += r.actualGross;  // E17 = SUM(E14:E16), E13=0
            P.totalSocialSecurity += r.socialSecurityCost; // F17
            P.totalHeadcountCost  += r.totalCost;  // H17
        }
    }

    // ================================================= Datos Base
    {
        auto& D = R.baseData;
        D.totalSales   = in.prescriptionSales + in.otcSales;         // D12
        D.grossMargin  = D.totalSales * in.marginPct;                // D14
        D.costOfGoods  = D.totalSales - D.grossMargin;                // D13
        D.rdDeduction  = calculateRdDeduction(in.prescriptionSales, in.rdBrackets); // D16
        D.marginAfterRd = D.grossMargin - D.rdDeduction;              // D17
        D.staffCost      = R.staff.totalActualGross;                  // D18 = 'Personal '!E17
        D.socialSecurity = R.staff.totalSocialSecurity;                // D19 = 'Personal '!F17
        D.totalOtherExpenses = in.premisesRent + in.utilities + in.advisoryFees
                           + in.maintenance + in.robot + in.insurance + in.otherExpenses; // D29
        // D20/D21 (self-employed quota and total staff cost) are completed
        // further below, after computing the year-1 RETA quota in Projection.
    }

    // ================================================= Financiación (v2)
    {
        auto& F = R.financing;
        F.goodwill = (in.prescriptionSales + in.otcSales) * in.goodwillMultiple; // D14
        F.fees     = (F.goodwill + in.premisesPrice) * in.feesPct;   // D18
        F.iva      = F.fees * in.ivaPct;                              // D19
        F.itpTax   = in.itpPct * in.premisesPrice;                   // D20
        F.ajd      = in.ajdPct * (F.goodwill + in.inventory);        // D21
        F.taxes    = F.itpTax + F.ajd;                                // D23
        const double totalInvestmentBeforeOpeningCost = F.goodwill + in.premisesPrice + in.inventory
                         + F.fees + F.iva
                         + in.notaryFees + in.registryFees + in.miscExpenses + F.taxes; // D24 (before opening fees)
        F.premisesBankFinancing = in.premisesPrice * in.premisesFinancingPct; // D46 (Excel: D15*0,7 literal)
        const double pharmacyBankFinancingBeforeOpeningCost = totalInvestmentBeforeOpeningCost - in.contributedCash
                              - in.initialOrder - F.premisesBankFinancing
                              - in.propertiesFinancing * in.propertiesFinancingPct; // D45 (before opening fees)
        // The mortgage opening fee is a % of the bank financing (pharmacy+
        // premises) plus the properties mortgage, but that same bank
        // financing covers those fees: solved algebraically instead of
        // iterating.
        //   X = pharmacyBankFinancingBeforeOpeningCost + p·(X + premisesBankFinancing + propertiesMortgageFinancing)
        //   X·(1-p) = pharmacyBankFinancingBeforeOpeningCost + p·(premisesBankFinancing + propertiesMortgageFinancing)
        const double p = in.mortgageOpeningPct;
        const double propertiesMortgageFinancing = in.propertiesFinancing * in.propertiesFinancingPct;
        F.pharmacyBankFinancing = (pharmacyBankFinancingBeforeOpeningCost + p * (F.premisesBankFinancing + propertiesMortgageFinancing)) / (1 - p);
        F.mortgageOpeningCost = p * (F.pharmacyBankFinancing + F.premisesBankFinancing + propertiesMortgageFinancing);
        F.totalInvestment = totalInvestmentBeforeOpeningCost + F.mortgageOpeningCost; // D24
        F.totalFinancing = in.contributedCash + in.familyContribution
                            + F.pharmacyBankFinancing + F.premisesBankFinancing
                            + in.propertiesFinancing * in.propertiesFinancingPct
                            + in.contributionExcess + in.initialOrder;       // D50
        F.minimumCash = (F.totalInvestment
                          - in.propertiesFinancing * in.propertiesFinancingPct
                            - in.initialOrder - in.premisesPrice)* (1-in.pharmacyFinancingPct) + in.premisesPrice * (1-in.premisesFinancingPct);
        F.cashBelowMinimum = in.contributedCash < F.minimumCash;
    }

    // ================================================= Amortizaciones
    R.bankAmort = amortize(R.financing.pharmacyBankFinancing + R.financing.premisesBankFinancing,
                             in.bankRate, in.bankTermYears, in.startYear, in.startMonth);
    R.coopAmort  = amortize(in.initialOrder, in.coopRate, in.coopTermYears,
                             in.startYear, in.startMonth);
    R.propertiesAmort  = amortize(in.propertiesFinancing * in.propertiesFinancingPct,
                             in.propertiesRate, in.propertiesTermYears,
                             in.startYear, in.startMonth);

    // Annual sums of the schedules (year i: months 12i+1 .. 12i+12)
    auto annualSum = [](const AmortResult& a, int year, bool interest) {
        double s = 0;
        for (int m = year * 12; m < year * 12 + 12; ++m)
            s += interest ? a.rows[m].interest : a.rows[m].principalPaid;
        return s;
    };

    // ================================================= Proyección 10 years
    {
        auto& P = R.projection;

        // Growth scenario: Realistic uses the last 10 years' historical IPC;
        // Optimistic uses the constant IPC set by the user.
        const std::array<double,10> annualIpc = (in.growthScenario >= 0.5)
            ? [&]{ std::array<double,10> a; a.fill(in.ipcOptimistic); return a; }()
            : in.ipcHistorical;
        const std::array<double,10> annualCommercialMargin = (in.growthScenario >= 0.5)
            ? optimisticMarginSeries(in)
            : in.realisticMarginSeries;

        for (int i = 0; i < 10; ++i) {
            // Year 1 starts from the initial values as-is (IPC not applied);
            // growth starts counting from year 2.
            // Negative IPC is treated as 0% (no deflation applied in the projection).
            const double ipc = (i == 0) ? 0.0 : std::max(0.0, annualIpc[i]);
            P.ipcApplied[i]         = ipc;
            P.commercialMarginPct[i] = annualCommercialMargin[i];

            // Sales (rows 6-7)
            P.prescriptionSales[i] = (i == 0 ? in.prescriptionSales : P.prescriptionSales[i-1]) * (1.0 + ipc);
            P.otcSales[i]  = (i == 0 ? in.otcSales  : P.otcSales[i-1])  * (1.0 + ipc);
            P.totalSales[i]  = P.prescriptionSales[i] + P.otcSales[i];             // row 8
            P.costOfGoods[i] = P.totalSales[i] * (1.0 - annualCommercialMargin[i]); // row 9
            P.grossMargin[i]      = P.totalSales[i] - P.costOfGoods[i];       // row 10
            P.rdDeduction[i] = (i == 0 ? R.baseData.rdDeduction : P.rdDeduction[i-1]) * (1.0 + ipc); // row 11
            P.marginAfterRd[i]  = P.grossMargin[i] - P.rdDeduction[i];        // row 12
            P.rent[i] = 0;                                                 // row 13 (constant 0)
            P.staffCost[i] = (i == 0)
                ? R.baseData.staffCost + R.baseData.socialSecurity     // B14 = SUM(D18:E19)
                : P.staffCost[i-1] * (1.0 + ipc);                         // row 14
            P.otherExpenses[i] = (i == 0)
                ? R.baseData.totalOtherExpenses                                 // B15 = D29
                : P.otherExpenses[i-1] * (1.0 + ipc);                            // row 15
            P.interest[i] = annualSum(R.bankAmort, i, true)
                           + annualSum(R.propertiesAmort,  i, true);                 // row 16 (negative)
            const double profitBeforeQuota = P.marginAfterRd[i] - P.rent[i]
                           - P.staffCost[i] - P.otherExpenses[i] + P.interest[i];
            // Self-employed (RETA) quota for the year: year 1 uses the flat
            // sign-up rate (fixed, regardless of bracket); from year 2 onward
            // the bracket is located from the previous year's projected
            // profit (before the quota itself).
            P.selfEmployedQuota[i] = (i == 0)
                ? in.retaFlatMonthlyFee * 12.0
                : calculateSelfEmployedQuota(profitBeforeQuota, in.retaBrackets);
            P.profit[i] = profitBeforeQuota - P.selfEmployedQuota[i];          // row 17
        }

        // Datos Base (D20/D21): use the RETA quota computed for year 1.
        R.baseData.totalStaffCost = R.baseData.staffCost
            + R.baseData.socialSecurity + P.selfEmployedQuota[0];               // D21
        R.baseData.profitBeforeTax = R.baseData.marginAfterRd
            - R.baseData.totalStaffCost - R.baseData.totalOtherExpenses;  // D30

        // ------------------------- Hoja Impuestos (IRPF, v2) — between rows 17 and 18
        {
            auto& I = R.taxes;
            const auto& F = R.financing;
            I.fdc             = F.goodwill;                               // B6
            I.fees             = F.fees;                                  // B7
            I.ajd             = in.ajdPct * (F.goodwill + in.inventory); // B8
            I.depreciableBase = I.fdc + I.fees + I.ajd;                  // B9
            I.premisesCost      = in.premisesPrice;                             // B10
            I.minimumDeduction = in.personalAllowance * in.irpfBrackets[0].rate;     // B33 = B14*O26
            for (int i = 0; i < 10; ++i) {
                I.profit[i]  = P.profit[i];                              // row 18
                I.premisesDepreciation[i] = I.premisesCost * in.taxPremisesDeprPct;          // row 19
                I.adjustedPct[i] = std::min(in.taxMaxGoodwillDeprPct,
                    std::max(in.taxMinGoodwillDeprPct,
                             (I.profit[i] - I.premisesDepreciation[i]) / I.depreciableBase)); // row 20
                I.fdcDepreciation[i] = I.adjustedPct[i] * I.depreciableBase;          // row 21
                I.taxableBase[i] = std::max(0.0,
                    I.profit[i] - I.premisesDepreciation[i] - I.fdcDepreciation[i]);         // row 22
                I.bracketQuota[i] = 0;
                for (int t = 0; t < 6; ++t) {                                  // rows 26-31
                    const auto& tr = in.irpfBrackets[t];
                    I.brackets[t][i] = std::max(0.0,
                        std::min(I.taxableBase[i], tr.to) - tr.from) * tr.rate;
                    I.bracketQuota[i] += I.brackets[t][i];                        // row 32
                }
                I.payment[i] = std::max(0.0, I.bracketQuota[i] - I.minimumDeduction); // row 34
            }
        }

        // ------------------------- Proyección rows 18-25 (need Impuestos)
        for (int i = 0; i < 10; ++i) {
            P.taxPayment[i] = R.taxes.payment[i];                          // row 18 = Impuestos!34
            P.cashAfterTax[i] = P.profit[i] - P.taxPayment[i];               // row 19
            P.bankPrincipalRepayment[i] = annualSum(R.bankAmort, i, false)
                                 + annualSum(R.propertiesAmort,  i, false);          // row 20 (negative)
            P.coopPrincipalRepayment[i]  = annualSum(R.coopAmort,  i, false);          // row 21
            P.netAnnualSalary[i]   = P.cashAfterTax[i] + P.bankPrincipalRepayment[i] + P.coopPrincipalRepayment[i]; // row 23
            P.netMonthlySalary[i] = P.netAnnualSalary[i] / 12.0;            // row 24
            P.staffCostPct[i] = (P.totalSales[i] != 0.0)
                ? P.staffCost[i] / P.totalSales[i] : 0.0;                 // row 25 (IFERROR)
        }
        R.staff.netMonthlySalaryYear1 = P.netMonthlySalary[0];          // Personal!B19
    }

    // ================================================= Análisis Inversión
    {
        auto& A = R.analysis;
        const auto& P = R.projection;
        const double bankBalanceAt120 = R.bankAmort.rows[119].endingBalance; // I137
        const double coopBalanceAt120  = R.coopAmort .rows[119].endingBalance;
        const double propertiesBalanceAt120  = R.propertiesAmort .rows[119].endingBalance;

        // Revaluation of the premises over 10 years with the IPC actually applied each year.
        double ipcFactor10 = 1.0;
        for (int y = 1; y < 10; ++y) ipcFactor10 *= (1.0 + P.ipcApplied[y]);

        // Goodwill already depreciated over the 10 years (Impuestos sheet).
        double fdcDepreciationAccum10 = 0.0;
        for (int i = 0; i < 10; ++i) fdcDepreciationAccum10 += R.taxes.fdcDepreciation[i];

        for (int s = 0; s < 3; ++s) {
            A.initialInvestment[s] = in.contributedCash;                        // row 7
            A.fdcSaleValue[s]    = P.totalSales[9] * in.saleFactor[s];        // row 9
            A.premisesSaleValue[s]  = in.premisesPrice * ipcFactor10;            // row 10
            A.inventoryYear10[s]    = P.totalSales[9] * in.inventoryPctYear10;      // row 11
            A.fdcOutstanding[s]     = R.taxes.fdc - fdcDepreciationAccum10;          // row 12
            A.debt[s]            = -(bankBalanceAt120 + coopBalanceAt120 + propertiesBalanceAt120); // row 14
            A.grossEquity[s]  = A.fdcSaleValue[s] + A.premisesSaleValue[s]
                                  + A.inventoryYear10[s] + A.debt[s];            // row 15
            A.netEquity[s]   = A.grossEquity[s] + in.saleTaxes[s];// row 16
            A.cagr[s] = std::pow(A.netEquity[s] / A.initialInvestment[s], 0.1) - 1.0; // row 17

            std::vector<double> cf;                                             // row 18 (IRR)
            cf.push_back(-A.initialInvestment[s]);
            for (int i = 0; i < 9; ++i) cf.push_back(P.netAnnualSalary[i]);
            cf.push_back(P.netAnnualSalary[9] + A.netEquity[s]);
            A.irr[s] = irr(cf);
        }

        // Monthly cash flow: years 1, 5, 10 (indices 0, 4, 9)
        const int yrs[3] = {0, 4, 9};
        for (int k = 0; k < 3; ++k) {
            const int y = yrs[k];
            A.monthlyCashFlow[k]        = P.cashAfterTax[y] / 12.0;                              // row 22
            A.monthlyPrincipalRepayment[k] = -(P.bankPrincipalRepayment[y] + P.coopPrincipalRepayment[y]) / 12.0; // row 23
            A.monthlyInterest[k]  = -P.interest[y] / 12.0;                            // row 24
            A.ownerNetIncome[k]       = P.netMonthlySalary[y];                           // row 25
        }

        // Goodwill depreciation simulation (rows 28-40)
        A.annualPremisesDepreciation = in.premisesPrice * in.investmentPremisesDeprPct;               // B32 (B30=Financiación!D15)
        double remaining = 0;
        for (int i = 0; i < 10; ++i) {
            A.pharmacyProfit[i] = P.profit[i];                                  // row 35
            A.fdcDepreciationPct[i] = std::min(in.fdcMaxPct,
                std::max(0.0, A.pharmacyProfit[i] - A.annualPremisesDepreciation) / in.fdcInitialSim); // row 36
            const double cap = (i == 0) ? in.fdcInitialSim : std::max(0.0, remaining);
            A.fdcDepreciation[i]   = std::min(A.fdcDepreciationPct[i] * in.fdcInitialSim, cap); // row 37
            A.premisesDepreciation[i] = A.annualPremisesDepreciation;                 // row 38
            A.taxableBase[i] = A.pharmacyProfit[i] - A.fdcDepreciation[i] - A.premisesDepreciation[i]; // row 39
            remaining = (i == 0) ? std::max(0.0, in.fdcInitialSim - A.fdcDepreciation[i])
                            : std::max(0.0, remaining - A.fdcDepreciation[i]);               // row 40
            A.fdcOutstandingSim[i] = remaining;
        }
    }

    return R;
}

} // namespace sim
