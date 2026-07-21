// simcore.h — Pure calculation engine (no Qt).
// Replicates exactly the formulas of "Simulación Farmacia_qt.xlsx".
#pragma once

#include <array>
#include <vector>

namespace sim {

// Max number of individually-editable employees per headcount-plan role
// (rows 13-16 of "Plantilla"). Headcount beyond this reuses the last slot's
// jornada % — plenty of room for any realistic pharmacy staff.
inline constexpr int kMaxStaffPerRole = 20;

// Official scale brackets (IRPF, Reales Decretos, RETA): editable defaults
// from the "Configuración" sheet (part of Inputs).
struct IrpfBracket { double from, to, rate; };
struct RdBracket   { double from, base, pct; };
struct RetaBracket { double from, monthlyQuota; };

// One day of the pharmacy's opening schedule, in 24h decimal hours (e.g. 9.5
// = 9:30). A closed day is represented as closeHour <= openHour (0 hours).
struct ScheduleDay { double openHour, closeHour; };

// ---------------------------------------------------------------- Inputs
struct Inputs {
    // ---- Datos Base (editable cells)
    double prescriptionSales = 800000;   // D10
    double otcSales          = 200000;   // D11
    double marginPct         = 0.34;     // D15
    double premisesRent      = 0;        // D22
    double utilities         = 2500;     // D23
    double advisoryFees      = 5000;     // D24
    double maintenance       = 3000;     // D25
    double robot             = 0;        // D26
    double insurance         = 4000;     // D27
    double otherExpenses     = 15000;    // D28

    // ---- Financiación: growth scenario
    // 0 = Realistic (uses the ipcHistorical series, editable in Configuración)
    // 1 = Optimistic (constant IPC entered by the user)
    double growthScenario = 0;
    double ipcOptimistic   = 0.025;

    // ---- Optimistic scenario: commercial margin editable per year
    // (year 1, year 2, year 3 onward — constant from year 3).
    double optimisticMarginYear1 = 0.330;
    double optimisticMarginYear2 = 0.340;
    double optimisticMarginYear3 = 0.350;

    // ---- Financiación: investment
    double goodwillMultiple = 2;         // D13
    double premisesPrice    = 200000;    // D15
    double inventory        = 100000;    // D16
    double notaryFees       = 4000;      // D20a
    double registryFees     = 2500;      // D20b
    double miscExpenses     = 3955.28;   // D20c (remainder)

    // ---- Financiación: fixed percentages of the sale/purchase (editable from
    // Configuración): agency/notary fees on FdC+premises, VAT (IVA) on those
    // fees, ITP on the commercial premises and AJD on FdC+inventory (the
    // latter is also used in the Impuestos sheet).
    double feesPct = 0.05;        // D18
    double ivaPct  = 0.21;        // D19
    double itpPct  = 0.08;        // D20
    double ajdPct  = 0.015;       // D21 y B8 (Impuestos)

    // ---- Financiación: rates and terms
    double bankRate         = 0.03;     // D27
    double coopRate         = 0.0;      // D28
    double familyRate       = 0.0;      // D29
    int    bankTermYears    = 20;       // D31 (years)
    int    coopTermYears    = 8;        // D32
    int    familyTermYears  = 10;       // D33
    // Periodo de carencia del préstamo familiar, en meses: durante estos
    // primeros meses solo se paga interés (el capital no se amortiza).
    // double (no int) para poder ponerlo a 0 desde la UI (ver Engine::set).
    double familyGraceMonths = 0;
    double pharmacyFinancingPct  = 0.8; // D34
    double premisesFinancingPct  = 0.7; // D35 (the Excel uses literal 0,7 in D44)
    double mortgageOpeningPct = 0.01;  // opening-fee % on bank financing (pharmacy+premises+properties mortgage)
    double propertiesFinancingPct = 0.7; // D36
    double propertiesRate         = 0.03; // D37
    int    propertiesTermYears    = 25;   // D38

    // ---- Financiación: contributions (v2 rows)
    double contributedCash    = 400000; // D43
    double familyContribution = 0;      // D44
    double propertiesFinancing = 0;      // D47
    double contributionExcess  = 0;      // D48
    double initialOrder        = 0;      // D49 (cooperative; empty in the v2 Excel)

    // ---- Hoja Impuestos (IRPF, new in v2)
    double taxPremisesDeprPct    = 0.04;     // B11 (fixed premises depreciation %)
    double taxMinGoodwillDeprPct = 0.05;     // B12 (min goodwill depreciation %)
    double taxMaxGoodwillDeprPct = 0.075;    // B13 (max goodwill depreciation %)
    double personalAllowance     = 5550;     // B14 (IRPF-exempt personal minimum)

    // ---- Personal: section 1 (salary data)
    double pharmacistSalary  = 32000;    // B6
    double pharmacistFte     = 1;        // C6
    double socialSecurityPct = 0.3;      // D6 (same for everyone)
    double assistantSalary   = 19189;    // B7
    double assistantFte      = 1;        // C7
    double technicianSalary  = 21102;    // B8
    double technicianFte     = 0.5;      // C8

    // ---- Personal: recommended headcount (rows 13-16)
    // 0=Owner, 1=Employed pharmacist, 2=Assistant, 3=Technician
    // Per-employee jornada %: staffFteEach[role][j] is employee #j+1 of that
    // role (was a single B13:B16 value shared by the whole headcount).
    std::array<std::array<double, kMaxStaffPerRole>, 4> staffFteEach = []{
        std::array<std::array<double, kMaxStaffPerRole>, 4> a{};
        a[0].fill(1);
        a[1].fill(1);
        a[2].fill(1);
        a[3].fill(0.5);
        return a;
    }();
    std::array<double,4> staffCount {1, 2, 0, 1};   // C13:C16
    // Annual salary raise %, per headcount-plan role (was a single H6 value
    // shared by every role). Index 0 (Owner) is unused: the owner's
    // reference salary (D13) never carries the raise.
    std::array<double,4> raisePct {0, 0.1, 0.1, 0.1};
    // Year each employee starts working (Plantilla only; the owner and the
    // vacation-cover reinforcements aren't staggered). Costs for years before
    // an employee's start year are excluded from the 10-year Proyección.
    // Index 0 (Owner) is unused: the owner is always active from year 1.
    std::array<std::array<int, kMaxStaffPerRole>, 4> staffHireYearEach = []{
        std::array<std::array<int, kMaxStaffPerRole>, 4> a{};
        for (auto& role : a) role.fill(2027);
        return a;
    }();

    // ---- Personal: vacation-cover headcount (temp staff hired to cover the
    // regular plantilla's vacations). Same 3 roles as byRole (0=Farmacéutico,
    // 1=Auxiliar, 2=Técnico); reuses each role's base salary. Per-employee
    // jornada % and months worked per year (they typically don't work the
    // full year).
    std::array<std::array<double, kMaxStaffPerRole>, 3> vacationStaffFteEach = []{
        std::array<std::array<double, kMaxStaffPerRole>, 3> a{};
        a[0].fill(1);
        a[1].fill(1);
        a[2].fill(0.5);
        return a;
    }();
    std::array<std::array<double, kMaxStaffPerRole>, 3> vacationStaffMonthsEach = []{
        std::array<std::array<double, kMaxStaffPerRole>, 3> a{};
        for (auto& role : a) role.fill(1.0);
        return a;
    }();
    std::array<double,3> vacationStaffCount {0, 0, 0};
    std::array<double,3> vacationRaisePct   {0, 0, 0};

    // ---- Personal: horario de apertura, usado para estimar las horas
    // semanales de cobertura necesarias frente a las horas contratadas.
    // Índice 0=Lunes ... 6=Domingo.
    std::array<ScheduleDay,7> schedule = []{
        std::array<ScheduleDay,7> a{};
        for (int d = 0; d < 5; ++d) a[d] = { 9, 21 };   // Lunes-Viernes 9-21
        a[5] = { 10, 14 };                               // Sábado 10-14
        a[6] = { 0, 0 };                                 // Domingo cerrado
        return a;
    }();

    // ---- Análisis Inversión
    std::array<double,3> saleFactor {1.8, 2.0, 2.2};                     // B8:D8
    std::array<double,3> saleTaxes  {-530474.16, -607279.23, -684084.30}; // B13:D13
    double fdcInitialSim       = 2000000;     // B28
    double fdcMaxPct           = 0.075;       // B29
    double investmentPremisesDeprPct = 0.04;  // B31
    double inventoryPctYear10  = 0.1;         // row 11: estimated 10-year inventory, % of year-10 total sales

    // Loan start date (F5): 01/2027
    int startYear  = 2027;
    int startMonth = 1;

    // ---- Configuración: official scales and series (editable by the user
    // from the "Configuración" sheet; these are the values currently in force).

    // General IRPF scale 2026 (Impuestos sheet, rows 26-31)
    std::array<IrpfBracket,6> irpfBrackets {{
        {      0,     12450, 0.19 },
        {  12450,     20200, 0.24 },
        {  20200,     35200, 0.30 },
        {  35200,     60000, 0.37 },
        {  60000,    300000, 0.45 },
        { 300000, 999999999, 0.47 },
    }};

    // "Reales Decretos" deduction scale (RD 823/2008, art. 2.5): brackets on
    // MONTHLY prescription sales billed to the SNS (PVP+IVA).
    std::array<RdBracket,9> rdBrackets {{
        {      0.00,     0.00, 0.000 },
        {  37500.00,     0.00, 0.078 },
        {  45000.00,   585.00, 0.091 },
        {  58345.61,  1799.45, 0.114 },
        { 120206.01,  8851.53, 0.136 },
        { 208075.90, 20801.83, 0.157 },
        { 295242.83, 34487.04, 0.172 },
        { 382409.76, 49479.75, 0.182 },
        { 600000.00, 89081.17, 0.200 },
    }};

    // RETA scale (self-employed) 2026 = 2025, frozen (Orden PJC/297/2026, BOE
    // 31-mar-2026): 15 brackets by monthly net income, minimum monthly quota
    // of each bracket (minimum contribution base + 0.9% MEI).
    std::array<RetaBracket,15> retaBrackets {{
        {      0.00, 205.88 },
        {   670.00, 226.47 },
        {   900.00, 267.64 },
        {  1166.70, 299.64 },
        {  1300.00, 302.64 },
        {  1500.00, 302.64 },
        {  1700.00, 319.12 },
        {  1850.00, 324.26 },
        {  2030.00, 329.41 },
        {  2330.00, 349.98 },
        {  2760.00, 380.85 },
        {  3190.00, 401.43 },
        {  3620.00, 432.29 },
        {  4050.00, 514.61 },
        {  6000.00, 607.31 },
    }};

    // Flat rate for the initial RETA sign-up (in force since 2023, 2026 =
    // 2025): 80 €/month fixed + MEI, regardless of the income bracket,
    // during the first 12 months of activity.
    double retaFlatMonthlyFee = 88.64;

    // Spain's historical CPI (IPC) (INE, average annual change), last 10
    // available years — "Realistic" scenario.
    std::array<double,10> ipcHistorical {
        -0.005, -0.002, 0.020, 0.017, 0.007, -0.003, 0.031, 0.084, 0.035, 0.028
    };

    // Simulated commercial margin evolution, "Realistic" scenario.
    std::array<double,10> realisticMarginSeries {
        0.330, 0.336, 0.333, 0.341, 0.338, 0.347, 0.344, 0.353, 0.358, 0.365
    };
};

// ---------------------------------------------------------------- Outputs
struct AmortRow {
    int    paymentNum;      // payment # (1..300)
    int    year, month;     // date
    double startingBalance, payment, principalPaid, interest, endingBalance;
};

struct AmortResult {
    double principal      = 0;  // F2
    double annualRate     = 0;  // F3
    int    termYears      = 0;  // F4
    double monthlyPayment = 0;  // F7
    double annualPayment  = 0;  // I7
    int    numPayments    = 0;  // F8
    double totalInterest  = 0;  // F9  (sum H18:H317)
    double totalCost      = 0;  // F10 (F2-F9)
    std::vector<AmortRow> rows; // 300 rows (months 1..300)
};

struct StaffRow { double grossFte, fte, socialSecurityPct, socialSecurityCost, actualSalary, totalCost; };
// fte here is the SUM of every employee's jornada % for that role (not the
// average): e.g. two full-time employees (100% each) add up to fte=2.0.
struct HeadcountRow { double fte, headcount, grossFte, actualGross, socialSecurityCost, costPerPerson, totalCost; };
// Vacation-cover headcount row: like HeadcountRow, but the cost is prorated
// by each employee's months worked per year (avgMonths is informational,
// weighted by headcount).
struct VacationStaffRow { double fte, headcount, grossFte, actualGross, socialSecurityCost, totalCost, avgMonths; };

struct StaffResult {
    std::array<StaffRow,3> byRole;     // rows 6-8
    double totalSocialSecurityCost=0, totalActualSalary=0, totalCost=0;          // row 9
    std::array<HeadcountRow,4> headcountPlan;// rows 13-16
    double totalHeadcount=0, totalActualGross=0, totalSocialSecurity=0, totalHeadcountCost=0; // row 17
    double netMonthlySalaryYear1=0;    // B19
    std::array<VacationStaffRow,3> vacationStaffPlan; // refuerzos de vacaciones
    double totalVacationHeadcount=0, totalVacationActualGross=0,
           totalVacationSocialSecurity=0, totalVacationCost=0;
};

// Weekly coverage: hours the pharmacy is open vs. hours contracted, derived
// from the schedule and the headcount plan (kWeeklyFullTimeHours = 8h/día x
// 5 días). Purely informational — no pass/fail semantics: the titular
// pharmacist always covers any gap in legally-required presence.
struct ScheduleResult {
    std::array<double,7> dailyHours{};     // horas que abre cada día (0 = cerrado)
    double weeklyOpenHours=0;              // total horas/semana abierta
    double pharmacistWeeklyHours=0;        // horas/semana: propietario + farmacéutico empleado
    double supportWeeklyHours=0;           // horas/semana: auxiliar + técnico
    double totalStaffWeeklyHours=0;        // pharmacistWeeklyHours + supportWeeklyHours
    double pharmacistHoursGap=0;           // weeklyOpenHours - pharmacistWeeklyHours (puede ser negativo)
    double totalHoursGap=0;                // weeklyOpenHours - totalStaffWeeklyHours
};

struct BaseDataResult {
    double totalSales=0, costOfGoods=0, grossMargin=0, rdDeduction=0, marginAfterRd=0;
    double staffCost=0, socialSecurity=0, totalStaffCost=0;
    double totalOtherExpenses=0, profitBeforeTax=0;
};

struct FinancingResult {
    double goodwill=0, fees=0, iva=0, totalInvestment=0;
    double itpTax=0;         // D20 (v2): 8% of the premises
    double ajd=0;            // D21 (v2): 1.5% of FdC + inventory
    double taxes=0;          // D23 (v2): ITP + AJD
    double mortgageOpeningCost=0; // % on bank financing (pharmacy+premises+properties mortgage)
    double pharmacyBankFinancing=0, premisesBankFinancing=0, totalFinancing=0;
    double minimumCash=0;    // Recommended minimum contribution = max(0, (totalInvestment-premisesPrice)*(1-pharmacyFinancingPct) - propertiesFinancing*propertiesFinancingPct - initialOrder) + premisesPrice*(1-premisesFinancingPct)
    bool   cashBelowMinimum=false; // contributedCash < minimumCash
};

struct TaxResult {
    // Step 1: depreciable base
    double fdc=0;             // B6
    double fees=0;            // B7
    double ajd=0;              // B8
    double depreciableBase=0;  // B9
    double premisesCost=0;     // B10
    double minimumDeduction=0; // B33 = personalAllowance × 19%
    // Step 2 and 3 (10 years)
    std::array<double,10> profit{},          // row 18
        premisesDepreciation{},              // row 19
        adjustedPct{},                       // row 20
        fdcDepreciation{},                   // row 21
        taxableBase{},                       // row 22
        bracketQuota{},                      // row 32
        payment{};                           // row 34 (TAX PAYMENT)
    std::array<std::array<double,10>,6> brackets{}; // rows 26-31
};

struct ProjectionResult {           // 10 values = years 1..10
    std::array<double,10> prescriptionSales{}, otcSales{}, totalSales{},
        costOfGoods{}, grossMargin{}, rdDeduction{}, marginAfterRd{},
        rent{}, staffCost{}, selfEmployedQuota{}, otherExpenses{}, interest{},
        profit{}, taxPayment{}, cashAfterTax{}, bankPrincipalRepayment{},
        coopPrincipalRepayment{}, netAnnualSalary{}, netMonthlySalary{},
        staffCostPct{};
    // Series applied by the growth scenario (informational only).
    std::array<double,10> ipcApplied{}, commercialMarginPct{};
};

struct AnalysisResult {
    // Scenarios 0=pessimistic, 1=neutral, 2=optimistic
    std::array<double,3> initialInvestment{}, fdcSaleValue{}, premisesSaleValue{},
        inventoryYear10{}, fdcOutstanding{}, debt{}, grossEquity{},
        netEquity{}, cagr{}, irr{};
    // Monthly cash flow: years 1, 5, 10
    std::array<double,3> monthlyCashFlow{}, monthlyPrincipalRepayment{}, monthlyInterest{}, ownerNetIncome{};
    // FdC depreciation simulation (10 years)
    double annualPremisesDepreciation=0;   // B32
    std::array<double,10> pharmacyProfit{}, fdcDepreciationPct{}, fdcDepreciation{},
        premisesDepreciation{}, taxableBase{}, fdcOutstandingSim{};
};

struct Results {
    StaffResult      staff;
    ScheduleResult   schedule;
    BaseDataResult   baseData;
    FinancingResult  financing;
    AmortResult      bankAmort, coopAmort, familyAmort, propertiesAmort;
    ProjectionResult projection;
    TaxResult        taxes;
    AnalysisResult   analysis;
};

// "Reales Decretos" deduction (RD 823/2008 art. 2.5): progressive bracket
// scale on MONTHLY prescription sales (PVP+IVA) billed to the SNS. Applied
// to the monthly average of annual prescription sales and annualized (x12).
double calculateRdDeduction(double annualPrescriptionSales, const std::array<RdBracket,9>& table);

// RETA (self-employed) quota, annual, per the real 15-bracket scale on net
// monthly income (Social Security, 2026 table = 2025, frozen). annualProfit
// is the pharmacy's profit before this quota; it is averaged monthly to
// locate the bracket, and the minimum monthly quota of that bracket is
// applied (includes the 0.9% MEI).
double calculateSelfEmployedQuota(double annualProfit, const std::array<RetaBracket,15>& table);

// Excel's PMT: constant (negative) payment of a loan.
double pmt(double monthlyRate, int numPayments, double principal);

// IRR (Excel's IRR) of a series of cash flows.
double irr(const std::vector<double>& cashflows, double guess = 0.1);

Results compute(const Inputs& in);

} // namespace sim
