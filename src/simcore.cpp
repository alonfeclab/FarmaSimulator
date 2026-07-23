#include "simcore.h"
#include <algorithm>
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
//
// graceMonths (optional, only used by the family loan): a leading "carencia"
// period during which neither interest nor capital is paid or amortized.
// The fixed payment is then recomputed on the untouched principal over the
// remaining term, so the loan still fully amortizes within numPayments.
static AmortResult amortize(double principal, double annualRate, int termYears,
                             int startYear, int startMonth, int graceMonths = 0)
{
    AmortResult a;
    a.principal   = principal;
    a.annualRate  = annualRate;
    a.termYears   = termYears;
    a.numPayments = termYears * 12;

    const int grace = std::clamp(graceMonths, 0, std::max(0, a.numPayments - 1));

    // Exact replica of the Excel: the payment uses F3/12, but the interest of
    // each row uses G3=F3*100/12 and then /100 (they differ in the last bit,
    // and that determines the schedule's behavior after the last payment).
    const double g3 = annualRate * 100.0 / 12.0;                              // G3
    const double fixedPayment = pmt(annualRate / 12.0, a.numPayments - grace, principal); // F18

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
        if (m <= grace) {
            // Carencia total: ni capital ni interés se mueven en este periodo.
            r.interest = 0.0;
            r.payment = 0.0;
            r.principalPaid = 0.0;
        } else {
            r.interest = (-balance * g3) / 100.0; // H: -E*$G$3/100
            r.payment  = (m == grace + 1) ? fixedPayment : (balance <= 0.0 ? 0.0 : fixedPayment); // F19: IF(E<=0;0;$F$18)
            r.principalPaid = r.payment - r.interest;  // G: F-H
        }
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
        // H6 is now per-role (in.raisePct[1..3]); role 0 (Owner) has none.
        const double d[4] = { in.pharmacistSalary,
                              in.pharmacistSalary * (1.0 + in.raisePct[1]),
                              in.assistantSalary  * (1.0 + in.raisePct[2]),
                              in.technicianSalary * (1.0 + in.raisePct[3]) };
        for (int i = 0; i < 4; ++i) {
            auto& r = P.headcountPlan[i];
            r.headcount = (i == 0) ? 1.0 : in.staffCount[i]; // Owner: always 1 person
            r.grossFte  = d[i];
            if (i == 0) { // Owner: E13..H13 = 0 (compensated via profit)
                r.fte = in.staffFteEach[i][0];
                r.actualGross = r.socialSecurityCost = r.costPerPerson = r.totalCost = 0;
            } else {
                // Sum each employee's individual jornada % instead of one %
                // shared by the whole headcount; extras beyond
                // kMaxStaffPerRole reuse the last slot's value.
                const int n = std::clamp(int(std::lround(r.headcount)), 0, kMaxStaffPerRole);
                double fteSum = 0;
                for (int j = 0; j < n; ++j)
                    fteSum += in.staffFteEach[i][j];
                if (r.headcount > kMaxStaffPerRole)
                    fteSum += in.staffFteEach[i][kMaxStaffPerRole - 1] * (r.headcount - kMaxStaffPerRole);
                r.fte = fteSum; // sum (not average) of every employee's jornada %, for display
                r.actualGross      = d[i] * fteSum;                    // E = D*sum(fte)
                r.socialSecurityCost = r.actualGross * in.socialSecurityPct; // F = E*$D$6
                r.costPerPerson     = r.actualGross * (1.0 + in.socialSecurityPct); // G = E*(1+$D$6)
                r.totalCost         = r.costPerPerson;               // H = G
            }
            P.totalHeadcount    += r.headcount;    // C17
            P.totalActualGross  += r.actualGross;  // E17 = SUM(E14:E16), E13=0
            P.totalSocialSecurity += r.socialSecurityCost; // F17
            P.totalHeadcountCost  += r.totalCost;  // H17
        }

        // Vacation-cover headcount: temp workers hired to cover the regular
        // plantilla's vacations. Reuses each role's base salary (pharmacist/
        // assistant/technician); cost is prorated by each employee's months
        // worked per year, since they don't work the full year.
        const double dv[3] = { in.pharmacistSalary * (1.0 + in.vacationRaisePct[0]),
                               in.assistantSalary  * (1.0 + in.vacationRaisePct[1]),
                               in.technicianSalary * (1.0 + in.vacationRaisePct[2]) };
        for (int i = 0; i < 3; ++i) {
            auto& r = P.vacationStaffPlan[i];
            r.headcount = in.vacationStaffCount[i];
            r.grossFte  = dv[i];
            const int n = std::clamp(int(std::lround(r.headcount)), 0, kMaxStaffPerRole);
            double fteSum = 0, weightedSum = 0, monthsSum = 0;
            for (int j = 0; j < n; ++j) {
                const double months = std::clamp(in.vacationStaffMonthsEach[i][j], 0.0, 12.0);
                fteSum += in.vacationStaffFteEach[i][j];
                weightedSum += in.vacationStaffFteEach[i][j] * (months / 12.0);
                monthsSum += months;
            }
            if (r.headcount > kMaxStaffPerRole) {
                const double extra = r.headcount - kMaxStaffPerRole;
                const double lastFte    = in.vacationStaffFteEach[i][kMaxStaffPerRole - 1];
                const double lastMonths = std::clamp(in.vacationStaffMonthsEach[i][kMaxStaffPerRole - 1], 0.0, 12.0);
                fteSum      += lastFte * extra;
                weightedSum += lastFte * (lastMonths / 12.0) * extra;
                monthsSum   += lastMonths * extra;
            }
            r.fte       = fteSum; // sum of every employee's jornada %, for display
            r.avgMonths = r.headcount > 0 ? monthsSum / r.headcount : 0.0;
            r.actualGross         = dv[i] * weightedSum; // gross prorated by months worked
            r.socialSecurityCost  = r.actualGross * in.socialSecurityPct;
            r.totalCost            = r.actualGross + r.socialSecurityCost;
            P.totalVacationHeadcount      += r.headcount;
            P.totalVacationActualGross    += r.actualGross;
            P.totalVacationSocialSecurity += r.socialSecurityCost;
            P.totalVacationCost           += r.totalCost;
        }
    }

    // ================================================= Horario (cobertura)
    {
        auto& S = R.schedule;
        constexpr double kWeeklyFullTimeHours = 8.0 * 5.0; // 8h/día x 5 días = jornada completa semanal

        double totalOpen = 0;
        for (int d = 0; d < 7; ++d) {
            const double h = std::max(0.0, in.schedule[d].closeHour - in.schedule[d].openHour);
            S.dailyHours[d] = h;
            totalOpen += h;
        }
        S.weeklyOpenHours = totalOpen;

        const auto& P = R.staff.headcountPlan;
        // r.fte is already the SUM of every employee's jornada fraction for
        // that role (see the headcount-plan loop above), not an average.
        auto roleHours = [&](int idx) { return P[idx].fte * kWeeklyFullTimeHours; };
        S.pharmacistWeeklyHours = roleHours(0) + roleHours(1); // Propietario + farmacéutico empleado
        S.supportWeeklyHours    = roleHours(2) + roleHours(3); // Auxiliar + técnico
        S.totalStaffWeeklyHours = S.pharmacistWeeklyHours + S.supportWeeklyHours;
        S.pharmacistHoursGap = S.weeklyOpenHours - S.pharmacistWeeklyHours;
        S.totalHoursGap      = S.weeklyOpenHours - S.totalStaffWeeklyHours;
    }

    // ================================================= Datos Base
    {
        auto& D = R.baseData;
        D.totalSales   = in.prescriptionSales + in.otcSales;         // D12
        D.grossMargin  = D.totalSales * in.marginPct;                // D14
        D.costOfGoods  = D.totalSales - D.grossMargin;                // D13
        D.rdDeduction  = calculateRdDeduction(in.prescriptionSales, in.rdBrackets); // D16
        D.marginAfterRd = D.grossMargin - D.rdDeduction;              // D17
        D.staffCost      = R.staff.totalActualGross + R.staff.totalVacationActualGross;  // D18 = 'Personal '!E17 + refuerzos vacaciones
        D.socialSecurity = R.staff.totalSocialSecurity + R.staff.totalVacationSocialSecurity; // D19 = 'Personal '!F17 + refuerzos vacaciones
        D.totalOtherExpenses = in.utilities + in.advisoryFees
                           + in.maintenance + in.robot + in.insurance + in.otherExpenses; // D29 (alquiler shown separately, not bundled here)
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
        // Properties/cooperative financing offsets the pharmacy cash requirement
        // euro-for-euro (they are loan sources, not partial collateral), clamped
        // at zero so surplus guarantees don't produce a negative minimum.
        const double pharmacyCashNeed = (F.totalInvestment - in.premisesPrice) * (1 - in.pharmacyFinancingPct)
                                          - in.propertiesFinancing * in.propertiesFinancingPct
                                          - in.initialOrder;
        F.minimumCash = std::max(0.0, pharmacyCashNeed) + in.premisesPrice * (1-in.premisesFinancingPct);
        F.cashBelowMinimum = in.contributedCash < F.minimumCash;
    }

    // ================================================= Amortizaciones
    // El local se amortiza junto con la hipoteca de propiedades, no con el
    // préstamo bancario principal de la farmacia.
    R.bankAmort = amortize(R.financing.pharmacyBankFinancing,
                             in.bankRate, in.bankTermYears, in.startYear, in.startMonth);
    R.coopAmort  = amortize(in.initialOrder, in.coopRate, in.coopTermYears,
                             in.startYear, in.startMonth);
    R.propertiesAmort  = amortize(in.propertiesFinancing * in.propertiesFinancingPct + R.financing.premisesBankFinancing,
                             in.propertiesRate, in.propertiesTermYears,
                             in.startYear, in.startMonth);
    R.familyAmort  = amortize(in.familyContribution, in.familyRate, in.familyTermYears,
                             in.startYear, in.startMonth, int(in.familyGraceMonths));

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

        // Per-role (1=pharmacist, 2=assistant, 3=technician) gross salary for
        // one full-time-equivalent, grown year over year by IPC just like
        // every other projection row. Role 0 (Owner) is always 0-cost.
        std::array<double,4> roleGrossFte = { 0.0,
            in.pharmacistSalary * (1.0 + in.raisePct[1]),
            in.assistantSalary  * (1.0 + in.raisePct[2]),
            in.technicianSalary * (1.0 + in.raisePct[3]) };
        // Vacation-cover cost isn't staggered by hire year (it's temporary,
        // recurring staff), so it keeps growing by IPC from its year-1 total.
        double vacationCost = R.staff.totalVacationActualGross + R.staff.totalVacationSocialSecurity;

        // Sum of jornada % (fte) of the employees of 'role' already hired by
        // 'currentYear' (their staffHireYearEach <= currentYear); mirrors the
        // headcount-plan fteSum logic above, including the >kMaxStaffPerRole
        // extension (reuses the last slot's jornada % and hire year).
        auto activeFteSum = [&](int role, int currentYear) {
            const double headcount = in.staffCount[role];
            const int n = std::clamp(int(std::lround(headcount)), 0, kMaxStaffPerRole);
            double sum = 0;
            for (int j = 0; j < n; ++j)
                if (in.staffHireYearEach[role][j] <= currentYear)
                    sum += in.staffFteEach[role][j];
            if (headcount > kMaxStaffPerRole) {
                const int last = kMaxStaffPerRole - 1;
                if (in.staffHireYearEach[role][last] <= currentYear)
                    sum += in.staffFteEach[role][last] * (headcount - kMaxStaffPerRole);
            }
            return sum;
        };

        for (int i = 0; i < 10; ++i) {
            // IPC is applied from year 1 onward (including year 1 itself).
            // Negative IPC is treated as 0% (no deflation applied in the projection).
            const double ipc = std::max(0.0, annualIpc[i]);
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
            P.rent[i] = (i == 0
                ? in.premisesRent
                : P.rent[i-1]) * (1.0 + ipc);                                 // row 13
            // row 14: Plantilla cost only counts employees already hired by
            // this projection year (in.startYear + i); vacation-cover cost
            // isn't staggered. Both grow year over year by the fixed
            // salaryRaisePct (decoupled from IPC), including year 1 itself
            // (see comment above).
            for (int r = 1; r <= 3; ++r) roleGrossFte[r] *= (1.0 + in.salaryRaisePct);
            vacationCost *= (1.0 + in.salaryRaisePct);
            const int currentYear = in.startYear + i;
            double regularStaffCost = 0;
            for (int r = 1; r <= 3; ++r)
                regularStaffCost += roleGrossFte[r] * activeFteSum(r, currentYear) * (1.0 + in.socialSecurityPct);
            P.staffCost[i] = regularStaffCost + vacationCost;
            P.otherExpenses[i] = (i == 0
                ? R.baseData.totalOtherExpenses                                 // B15 = D29 (rent shown separately in row 13)
                : P.otherExpenses[i-1]) * (1.0 + ipc);                            // row 15
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
            - R.baseData.totalStaffCost - in.premisesRent - R.baseData.totalOtherExpenses;  // D30

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
