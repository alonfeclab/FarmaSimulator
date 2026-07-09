#include "simcore.h"
#include <cmath>

namespace sim {

double pmt(double r, int n, double p)
{
    if (n <= 0) return 0;
    if (r == 0.0) return -p / n;
    // exp(-n*log1p(r)) en lugar de pow(): coincide bit a bit con el PMT de
    // Excel, lo que importa en el filo del último pago del cuadro.
    return -p * r / (1.0 - std::exp(-double(n) * std::log1p(r)));
}

// NPV para IRR
static double npv(double rate, const std::vector<double>& cf)
{
    double v = 0;
    for (size_t i = 0; i < cf.size(); ++i)
        v += cf[i] / std::pow(1.0 + rate, double(i));
    return v;
}

double irr(const std::vector<double>& cf, double guess)
{
    // Newton-Raphson con derivada numérica y bisección de respaldo
    double r = guess;
    for (int it = 0; it < 100; ++it) {
        const double f  = npv(r, cf);
        const double h  = 1e-7;
        const double df = (npv(r + h, cf) - f) / h;
        if (std::fabs(df) < 1e-12) break;
        const double r2 = r - f / df;
        if (std::fabs(r2 - r) < 1e-10) return r2;
        r = (r2 <= -1.0) ? (r - 1.0) / 2.0 : r2; // no cruzar -100%
    }
    // Bisección
    double lo = -0.9999, hi = 10.0;
    double flo = npv(lo, cf), fhi = npv(hi, cf);
    if (flo * fhi > 0) return r; // sin cambio de signo: devolver Newton
    for (int it = 0; it < 200; ++it) {
        const double mid = (lo + hi) / 2.0;
        const double fm = npv(mid, cf);
        if (std::fabs(fm) < 1e-9) return mid;
        if (flo * fm < 0) { hi = mid; fhi = fm; } else { lo = mid; flo = fm; }
    }
    return (lo + hi) / 2.0;
}

// Cuadro de amortización: 300 filas SIEMPRE (como el Excel, filas 18-317).
// Tras liquidarse el préstamo la cuota pasa a 0 pero el interés se sigue
// calculando sobre el saldo (ligeramente negativo) — réplica exacta.
static AmortResult amortizar(double principal, double tasaAnual, int plazoAnios,
                             int anio0, int mes0)
{
    AmortResult a;
    a.principal  = principal;
    a.tasaAnual  = tasaAnual;
    a.plazoAnios = plazoAnios;
    a.numPagos   = plazoAnios * 12;

    // Réplica exacta del Excel: la cuota usa F3/12, pero el interés de cada
    // fila usa G3=F3*100/12 y luego /100 (difieren en el último bit, y eso
    // determina el comportamiento del cuadro tras el último pago).
    const double g3 = tasaAnual * 100.0 / 12.0;                        // G3
    const double cuotaFija = pmt(tasaAnual / 12.0, a.numPagos, principal); // F18

    a.pagoMensual = cuotaFija;
    a.pagoAnual   = cuotaFija * 12.0;

    a.rows.reserve(300);
    double saldo = principal;
    int anio = anio0, mes = mes0;
    for (int m = 1; m <= 300; ++m) {
        AmortRow r;
        r.num      = m;
        r.anio     = anio;
        r.mes      = mes;
        r.saldoIni = saldo;
        r.cuota    = (m == 1) ? cuotaFija : (saldo <= 0.0 ? 0.0 : cuotaFija); // F19: IF(E<=0;0;$F$18)
        r.interes  = (-saldo * g3) / 100.0; // H: -E*$G$3/100
        r.capital  = r.cuota - r.interes;  // G: F-H
        r.saldoFin = saldo + r.capital;    // I: E+G
        a.totalIntereses += r.interes;
        a.rows.push_back(r);
        saldo = r.saldoFin;
        if (++mes > 12) { mes = 1; ++anio; }
    }
    a.costeTotal = principal - a.totalIntereses; // F10
    return a;
}

// Escala general IRPF 2026 (hoja Impuestos, filas 26-31)
const std::array<TramoIRPF,6> kTramosIRPF {{
    {      0,     12450, 0.19 },
    {  12450,     20200, 0.24 },
    {  20200,     35200, 0.30 },
    {  35200,     60000, 0.37 },
    {  60000,    300000, 0.45 },
    { 300000, 999999999, 0.47 },
}};

// Índice de crecimiento: año 1 -> col D, año 2 -> col E, años 3..10 -> col F
static inline int gidx(int i) { return i == 0 ? 0 : (i == 1 ? 1 : 2); }

Results compute(const Inputs& in)
{
    Results R;

    // ================================================= Personal (sección 1)
    {
        auto& P = R.personal;
        const double b[3] = { in.salFarmaceutico, in.salAuxiliar, in.salTecnico };
        const double c[3] = { in.jornFarmaceutico, in.jornAuxiliar, in.jornTecnico };
        for (int i = 0; i < 3; ++i) {
            auto& r = P.datos[i];
            r.brutoFT   = b[i];
            r.jornada   = c[i];
            r.pctSS     = in.pctSS;                 // D7=D8=$D$6
            r.costeSS   = b[i] * c[i] * in.pctSS;   // E = B*C*D
            r.salReal   = b[i] * c[i];              // F = B*C
            r.costeTotal= r.costeSS + r.salReal;    // G = E+F
            P.totCosteSS += r.costeSS;
            P.totSalReal += r.salReal;
            P.totCoste   += r.costeTotal;
        }

        // Plantilla recomendada (filas 13-16)
        // D13=B6 ; D14=B6*(1+H6) ; D15=B7*(1+H6) ; D16=B8*(1+H6)
        const double d[4] = { in.salFarmaceutico,
                              in.salFarmaceutico * (1.0 + in.subidaPct),
                              in.salAuxiliar     * (1.0 + in.subidaPct),
                              in.salTecnico      * (1.0 + in.subidaPct) };
        for (int i = 0; i < 4; ++i) {
            auto& r = P.plantilla[i];
            r.jornada  = in.plJornada[i];
            r.personas = in.plPersonas[i];
            r.brutoFT  = d[i];
            if (i == 0) { // Propietario: E13..H13 = 0 (se retribuye vía beneficio)
                r.brutoReal = r.costeSS = r.costePersona = r.costeTotal = 0;
            } else {
                r.brutoReal    = d[i] * r.jornada * r.personas;   // E = D*B*C
                r.costeSS      = r.brutoReal * in.pctSS;          // F = E*$D$6
                r.costePersona = r.brutoReal * (1.0 + in.pctSS);  // G = E*(1+$D$6)
                r.costeTotal   = r.costePersona;                  // H = G
            }
            P.totPersonas  += r.personas;    // C17
            P.totBrutoReal += r.brutoReal;   // E17 = SUM(E14:E16), E13=0
            P.totSS        += r.costeSS;     // F17
            P.totPlantilla += r.costeTotal;  // H17
        }
    }

    // ================================================= Datos Base
    {
        auto& D = R.datosBase;
        D.ventaTotal     = in.ventaReceta + in.ventaLibre;       // D12
        D.mComBruto      = D.ventaTotal * in.margenPct;          // D14
        D.costeMercancia = D.ventaTotal - D.mComBruto;           // D13
        D.mComDespuesRD  = D.mComBruto - in.realesDecretos;      // D17
        D.gastosPersonal  = R.personal.totBrutoReal;             // D18 = 'Personal '!E17
        D.seguridadSocial = R.personal.totSS;                    // D19 = 'Personal '!F17
        D.totalGastosPersonal = D.gastosPersonal + D.seguridadSocial + in.cuotaAutonomos; // D21
        D.totalOtrosGastos = in.alquilerLocal + in.suministros + in.asesoria
                           + in.mantenimiento + in.robot + in.seguros + in.otrosGastos; // D29
        D.beneficioAntesImp = D.mComDespuesRD - D.totalGastosPersonal - D.totalOtrosGastos; // D30
    }

    // ================================================= Financiación (v2)
    {
        auto& F = R.financiacion;
        F.fondoComercio = (in.ventaReceta + in.ventaLibre) * in.coeficiente; // D14
        F.honorarios    = (F.fondoComercio + in.localComercial) * 0.05;     // D18
        F.iva           = F.honorarios * 0.21;                              // D19
        F.impuestoITP   = 0.08 * in.localComercial;                         // D20
        F.ajd           = 0.015 * (F.fondoComercio + in.existencias);       // D21
        F.impuestos     = F.impuestoITP + F.ajd;                           // D23
        F.totalInversion = F.fondoComercio + in.localComercial + in.existencias
                         + F.honorarios + F.iva
                         + in.gastosVarios + F.impuestos;                   // D24
        F.finBancariaLocal = in.localComercial * in.pctFinLocal;            // D46 (Excel: D15*0,7 literal)
        F.finBancariaFarmacia = F.totalInversion - in.liquidezAportada - in.pedidoInicial
                              - F.finBancariaLocal - in.finPropiedades * in.pctFinPropiedades; // D45
        F.totalFinanciacion = in.liquidezAportada + in.aportacionFamiliar
                            + F.finBancariaFarmacia + F.finBancariaLocal
                            + in.finPropiedades * in.pctFinPropiedades
                            + in.excesoAportacion + in.pedidoInicial;       // D50
    }

    // ================================================= Amortizaciones
    R.amortBanco = amortizar(R.financiacion.finBancariaFarmacia + R.financiacion.finBancariaLocal,
                             in.tipoBanco, in.plazoBanco, in.inicioAnio, in.inicioMes);
    R.amortCoop  = amortizar(in.pedidoInicial, in.tipoCoop, in.plazoCoop,
                             in.inicioAnio, in.inicioMes);
    R.amortProp  = amortizar(in.finPropiedades * in.pctFinPropiedades,
                             in.tipoPropiedades, in.plazoPropiedades,
                             in.inicioAnio, in.inicioMes);

    // Sumas anuales de los cuadros (año i: meses 12i+1 .. 12i+12)
    auto sumaAnual = [](const AmortResult& a, int anio, bool interes) {
        double s = 0;
        for (int m = anio * 12; m < anio * 12 + 12; ++m)
            s += interes ? a.rows[m].interes : a.rows[m].capital;
        return s;
    };

    // ================================================= Proyección 10 años
    {
        auto& P = R.proyeccion;
        for (int i = 0; i < 10; ++i) {
            const int g = gidx(i);
            // Ventas (filas 6-7)
            P.ventaReceta[i] = (i == 0 ? in.ventaReceta : P.ventaReceta[i-1]) * (1.0 + in.crecSeguro[g]);
            P.ventaLibre[i]  = (i == 0 ? in.ventaLibre  : P.ventaLibre[i-1])  * (1.0 + in.crecLibre[g]);
            P.ventaTotal[i]  = P.ventaReceta[i] + P.ventaLibre[i];             // fila 8
            P.costeMercancia[i] = P.ventaTotal[i] * (1.0 - in.margen[g]);      // fila 9
            P.mComBruto[i]      = P.ventaTotal[i] - P.costeMercancia[i];       // fila 10
            P.realesDecretos[i] = (i == 0 ? in.realesDecretos : P.realesDecretos[i-1]) * (1.0 + in.ipc[g]); // fila 11
            P.mComDespuesRD[i]  = P.mComBruto[i] - P.realesDecretos[i];        // fila 12
            P.alquiler[i] = 0;                                                 // fila 13 (constante 0)
            P.gastosPersonal[i] = (i == 0)
                ? R.datosBase.gastosPersonal + R.datosBase.seguridadSocial     // B14 = SUM(D18:E19)
                : P.gastosPersonal[i-1] * (1.0 + in.ipc[g]);                   // fila 14
            P.otrosGastos[i] = (i == 0)
                ? R.datosBase.totalOtrosGastos                                 // B15 = D29
                : P.otrosGastos[i-1] * (1.0 + in.crecSeguro[g]);               // fila 15 (crece con venta seguro)
            P.intereses[i] = sumaAnual(R.amortBanco, i, true)
                           + sumaAnual(R.amortProp,  i, true);                 // fila 16 (negativo)
            P.beneficio[i] = P.mComDespuesRD[i] - P.alquiler[i]
                           - P.gastosPersonal[i] - P.otrosGastos[i] + P.intereses[i]; // fila 17
        }

        // ------------------------- Hoja Impuestos (IRPF, v2) — entre filas 17 y 18
        {
            auto& I = R.impuestos;
            const auto& F = R.financiacion;
            I.fdc             = F.fondoComercio;                               // B6
            I.honorarios      = F.honorarios;                                  // B7
            I.ajd             = 0.015 * (F.fondoComercio + in.existencias);    // B8
            I.baseAmortizable = I.fdc + I.honorarios + I.ajd;                  // B9
            I.costeLocal      = in.localComercial;                             // B10
            I.deduccionMinimo = in.minimoPersonal * kTramosIRPF[0].tipo;       // B33 = B14*O26
            for (int i = 0; i < 10; ++i) {
                I.beneficio[i]  = P.beneficio[i];                              // fila 18
                I.amortLocal[i] = I.costeLocal * in.impAmortLocalPct;          // fila 19
                I.pctAjustado[i] = std::min(in.impAmortMaxPct,
                    std::max(in.impAmortMinPct,
                             (I.beneficio[i] - I.amortLocal[i]) / I.baseAmortizable)); // fila 20
                I.amortFdC[i] = I.pctAjustado[i] * I.baseAmortizable;          // fila 21
                I.baseImponible[i] = std::max(0.0,
                    I.beneficio[i] - I.amortLocal[i] - I.amortFdC[i]);         // fila 22
                I.cuotaEscala[i] = 0;
                for (int t = 0; t < 6; ++t) {                                  // filas 26-31
                    const auto& tr = kTramosIRPF[t];
                    I.tramos[t][i] = std::max(0.0,
                        std::min(I.baseImponible[i], tr.hasta) - tr.desde) * tr.tipo;
                    I.cuotaEscala[i] += I.tramos[t][i];                        // fila 32
                }
                I.pago[i] = std::max(0.0, I.cuotaEscala[i] - I.deduccionMinimo); // fila 34
            }
        }

        // ------------------------- Proyección filas 18-25 (necesitan Impuestos)
        for (int i = 0; i < 10; ++i) {
            P.pagoImpuestos[i] = R.impuestos.pago[i];                          // fila 18 = Impuestos!34
            P.liquidez[i] = P.beneficio[i] - P.pagoImpuestos[i];               // fila 19
            P.devCapitalBanco[i] = sumaAnual(R.amortBanco, i, false)
                                 + sumaAnual(R.amortProp,  i, false);          // fila 20 (negativo)
            P.devCooperativa[i]  = sumaAnual(R.amortCoop,  i, false);          // fila 21
            P.salarioNetoAnual[i]   = P.liquidez[i] + P.devCapitalBanco[i] + P.devCooperativa[i]; // fila 23
            P.salarioNetoMensual[i] = P.salarioNetoAnual[i] / 12.0;            // fila 24
            P.pctGastoPersonal[i] = (P.ventaTotal[i] != 0.0)
                ? P.gastosPersonal[i] / P.ventaTotal[i] : 0.0;                 // fila 25 (IFERROR)
        }
        R.personal.salarioNetoMensualAnio1 = P.salarioNetoMensual[0];          // Personal!B19
    }

    // ================================================= Análisis Inversión
    {
        auto& A = R.analisis;
        const auto& P = R.proyeccion;
        const double saldoBanco120 = R.amortBanco.rows[119].saldoFin; // I137
        const double saldoCoop120  = R.amortCoop .rows[119].saldoFin;
        const double saldoProp120  = R.amortProp .rows[119].saldoFin;

        for (int s = 0; s < 3; ++s) {
            A.inversionInicial[s] = in.liquidezAportada;                        // fila 7
            A.valorVentaFdC[s]    = P.ventaTotal[9] * in.factorVenta[s];        // fila 9
            A.valorVentaLocal[s]  = in.localComercial * std::pow(1.0 + in.ipc[2], 9); // fila 10
            A.existencias10[s]    = P.ventaTotal[9] * 0.1;                      // fila 11
            A.fdcPendiente[s]     = saldoBanco120;                              // fila 12
            A.deuda[s]            = -(saldoBanco120 + saldoCoop120 + saldoProp120); // fila 14
            A.patrimonioBruto[s]  = A.valorVentaFdC[s] + A.valorVentaLocal[s]
                                  + A.existencias10[s] + A.deuda[s];            // fila 15
            A.patrimonioNeto[s]   = A.patrimonioBruto[s] + in.impuestosVenta[s];// fila 16
            A.cagr[s] = std::pow(A.patrimonioNeto[s] / A.inversionInicial[s], 0.1) - 1.0; // fila 17

            std::vector<double> cf;                                             // fila 18 (TIR)
            cf.push_back(-A.inversionInicial[s]);
            for (int i = 0; i < 9; ++i) cf.push_back(P.salarioNetoAnual[i]);
            cf.push_back(P.salarioNetoAnual[9] + A.patrimonioNeto[s]);
            A.tir[s] = irr(cf);
        }

        // Liquidez mensual: años 1, 5, 10 (índices 0, 4, 9)
        const int yrs[3] = {0, 4, 9};
        for (int k = 0; k < 3; ++k) {
            const int y = yrs[k];
            A.liqMensual[k]        = P.liquidez[y] / 12.0;                              // fila 22
            A.devCapitalMensual[k] = -(P.devCapitalBanco[y] + P.devCooperativa[y]) / 12.0; // fila 23
            A.interesesMensual[k]  = -P.intereses[y] / 12.0;                            // fila 24
            A.netoTitular[k]       = P.salarioNetoMensual[y];                           // fila 25
        }

        // Simulación amortización fondo de comercio (filas 28-40)
        A.amortLocalAnual = in.localComercial * in.pctAmortLocal;               // B32 (B30=Financiación!D15)
        double pend = 0;
        for (int i = 0; i < 10; ++i) {
            A.benFarmacia[i] = P.beneficio[i];                                  // fila 35
            A.pctAmortFdC[i] = std::min(in.pctMaxFdC,
                std::max(0.0, A.benFarmacia[i] - A.amortLocalAnual) / in.fdcInicialSim); // fila 36
            const double tope = (i == 0) ? in.fdcInicialSim : std::max(0.0, pend);
            A.amortFdC[i]   = std::min(A.pctAmortFdC[i] * in.fdcInicialSim, tope); // fila 37
            A.amortLocal[i] = A.amortLocalAnual;                                 // fila 38
            A.baseImponible[i] = A.benFarmacia[i] - A.amortFdC[i] - A.amortLocal[i]; // fila 39
            pend = (i == 0) ? std::max(0.0, in.fdcInicialSim - A.amortFdC[i])
                            : std::max(0.0, pend - A.amortFdC[i]);               // fila 40
            A.fdcPendienteSim[i] = pend;
        }
    }

    return R;
}

} // namespace sim
