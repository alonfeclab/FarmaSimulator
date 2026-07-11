// simcore.h — Motor de cálculo puro (sin Qt).
// Replica exactamente las fórmulas de "Simulación Farmacia_qt.xlsx".
#pragma once

#include <array>
#include <vector>

namespace sim {

// ---------------------------------------------------------------- Entradas
struct Inputs {
    // ---- Datos Base (celdas editables)
    double ventaReceta      = 800000;   // D10
    double ventaLibre       = 200000;   // D11
    double margenPct        = 0.34;     // D15
    double cuotaAutonomos   = 1500;     // D20
    double alquilerLocal    = 0;        // D22
    double suministros      = 2500;     // D23
    double asesoria         = 5000;     // D24
    double mantenimiento    = 3000;     // D25
    double robot            = 0;        // D26
    double seguros          = 4000;     // D27
    double otrosGastos      = 15000;    // D28

    // ---- Financiación: escenario de crecimiento
    // 0 = Realista (IPC histórico de los últimos 10 años, no editable)
    // 1 = Optimista (IPC constante introducido por el usuario)
    double escenarioCrecimiento = 0;
    double ipcOptimista         = 0.025;

    // ---- Financiación: inversión
    double coeficiente     = 2;         // D13
    double localComercial  = 200000;    // D15
    double existencias     = 100000;    // D16
    double gastosVarios    = 8696.48;   // D20

    // ---- Financiación: tipos y plazos
    double tipoBanco        = 0.03;     // D27
    double tipoCoop         = 0.0;      // D28
    double tipoFamiliar     = 0.0;      // D29
    int    plazoBanco       = 20;       // D31 (años)
    int    plazoCoop        = 8;        // D32
    int    plazoFamiliar    = 10;       // D33
    double pctFinFarmacia   = 0.8;      // D34
    double pctFinLocal      = 0.7;      // D35 (el Excel usa 0,7 literal en D44)
    double pctFinPropiedades= 0.7;      // D36
    double tipoPropiedades  = 0.03;     // D37
    int    plazoPropiedades = 25;       // D38

    // ---- Financiación: aportaciones (filas v2)
    double liquidezAportada   = 400000; // D43
    double aportacionFamiliar = 0;      // D44
    double finPropiedades     = 1000000;// D47
    double excesoAportacion   = 0;      // D48
    double pedidoInicial      = 0;      // D49 (cooperativa; vacío en el Excel v2)

    // ---- Hoja Impuestos (IRPF, nueva en v2)
    double impAmortLocalPct = 0.04;     // B11 (% amortización local, fijo)
    double impAmortMinPct   = 0.05;     // B12 (% amort. farmacia mínimo)
    double impAmortMaxPct   = 0.075;    // B13 (% amort. farmacia máximo)
    double minimoPersonal   = 5550;     // B14 (mínimo personal exento IRPF)

    // ---- Personal: sección 1 (datos salariales)
    double salFarmaceutico  = 32000;    // B6
    double jornFarmaceutico = 1;        // C6
    double pctSS            = 0.3;      // D6 (igual para todos)
    double subidaPct        = 0.1;      // H6 (% subida s/ salario base)
    double salAuxiliar      = 19189;    // B7
    double jornAuxiliar     = 1;        // C7
    double salTecnico       = 21102;    // B8
    double jornTecnico      = 0.5;      // C8

    // ---- Personal: plantilla recomendada (filas 13-16)
    // 0=Propietario, 1=Farmacéutico empleado, 2=Auxiliar, 3=Técnico
    std::array<double,4> plJornada  {1, 1, 1, 0.5}; // B13:B16
    std::array<double,4> plPersonas {1, 2, 0, 1};   // C13:C16

    // ---- Análisis Inversión
    std::array<double,3> factorVenta   {1.8, 2.0, 2.2};              // B8:D8
    std::array<double,3> impuestosVenta{-530474.16, -607279.23, -684084.30}; // B13:D13
    double fdcInicialSim = 2000000;     // B28
    double pctMaxFdC     = 0.075;       // B29
    double pctAmortLocal = 0.04;        // B31

    // Fecha inicio préstamos (F5): 01/2027
    int inicioAnio = 2027;
    int inicioMes  = 1;
};

// ---------------------------------------------------------------- Salidas
struct AmortRow {
    int    num;        // Nº pago (1..300)
    int    anio, mes;  // fecha
    double saldoIni, cuota, capital, interes, saldoFin;
};

struct AmortResult {
    double principal   = 0;  // F2
    double tasaAnual   = 0;  // F3
    int    plazoAnios  = 0;  // F4
    double pagoMensual = 0;  // F7
    double pagoAnual   = 0;  // I7
    int    numPagos    = 0;  // F8
    double totalIntereses = 0; // F9  (suma H18:H317)
    double costeTotal     = 0; // F10 (F2-F9)
    std::vector<AmortRow> rows; // 300 filas (meses 1..300)
};

struct PersonalRow { double brutoFT, jornada, pctSS, costeSS, salReal, costeTotal; };
struct PlantillaRow { double jornada, personas, brutoFT, brutoReal, costeSS, costePersona, costeTotal; };

struct PersonalResult {
    std::array<PersonalRow,3> datos;     // filas 6-8
    double totCosteSS=0, totSalReal=0, totCoste=0;          // fila 9
    std::array<PlantillaRow,4> plantilla;// filas 13-16
    double totPersonas=0, totBrutoReal=0, totSS=0, totPlantilla=0; // fila 17
    double salarioNetoMensualAnio1=0;    // B19
};

struct DatosBaseResult {
    double ventaTotal=0, costeMercancia=0, mComBruto=0, realesDecretos=0, mComDespuesRD=0;
    double gastosPersonal=0, seguridadSocial=0, totalGastosPersonal=0;
    double totalOtrosGastos=0, beneficioAntesImp=0;
};

struct FinanciacionResult {
    double fondoComercio=0, honorarios=0, iva=0, totalInversion=0;
    double impuestoITP=0;   // D20 (v2): 8% del local
    double ajd=0;           // D21 (v2): 1,5% de FdC + existencias
    double impuestos=0;     // D23 (v2): ITP + AJD
    double finBancariaFarmacia=0, finBancariaLocal=0, totalFinanciacion=0;
    double minimoLiquidez=0;    // Aportación mínima recomendada = totalInversion - (finPropiedades*pctFinPropiedades) - pedidoInicial
    bool   liquidezInvalida=false; // liquidezAportada < minimoLiquidez
};

// Hoja "Impuestos" (IRPF por tramos, escala general 2026) — nueva en v2
struct TramoIRPF { double desde, hasta, tipo; };

struct ImpuestosResult {
    // Paso 1: base amortizable
    double fdc=0;            // B6
    double honorarios=0;     // B7
    double ajd=0;            // B8
    double baseAmortizable=0;// B9
    double costeLocal=0;     // B10
    double deduccionMinimo=0;// B33 = mínimo personal × 19%
    // Paso 2 y 3 (10 años)
    std::array<double,10> beneficio{},      // fila 18
        amortLocal{},                        // fila 19
        pctAjustado{},                       // fila 20
        amortFdC{},                          // fila 21
        baseImponible{},                     // fila 22
        cuotaEscala{},                       // fila 32
        pago{};                              // fila 34 (PAGO IMPUESTOS)
    std::array<std::array<double,10>,6> tramos{}; // filas 26-31
};

// Escala general IRPF 2026 (filas 26-31 de la hoja Impuestos)
extern const std::array<TramoIRPF,6> kTramosIRPF;

struct ProyeccionResult {           // 10 valores = años 1..10
    std::array<double,10> ventaReceta{}, ventaLibre{}, ventaTotal{},
        costeMercancia{}, mComBruto{}, realesDecretos{}, mComDespuesRD{},
        alquiler{}, gastosPersonal{}, otrosGastos{}, intereses{},
        beneficio{}, pagoImpuestos{}, liquidez{}, devCapitalBanco{},
        devCooperativa{}, salarioNetoAnual{}, salarioNetoMensual{},
        pctGastoPersonal{};
    // Series aplicadas por el escenario de crecimiento (solo informativas).
    std::array<double,10> ipcAplicado{}, margenComercial{};
};

struct AnalisisResult {
    // Escenarios 0=pesimista, 1=neutral, 2=optimista
    std::array<double,3> inversionInicial{}, valorVentaFdC{}, valorVentaLocal{},
        existencias10{}, fdcPendiente{}, deuda{}, patrimonioBruto{},
        patrimonioNeto{}, cagr{}, tir{};
    // Liquidez mensual: años 1, 5, 10
    std::array<double,3> liqMensual{}, devCapitalMensual{}, interesesMensual{}, netoTitular{};
    // Simulación amortización FdC (10 años)
    double amortLocalAnual=0;   // B32
    std::array<double,10> benFarmacia{}, pctAmortFdC{}, amortFdC{},
        amortLocal{}, baseImponible{}, fdcPendienteSim{};
};

struct Results {
    PersonalResult     personal;
    DatosBaseResult    datosBase;
    FinanciacionResult financiacion;
    AmortResult        amortBanco, amortCoop, amortProp;
    ProyeccionResult   proyeccion;
    ImpuestosResult    impuestos;
    AnalisisResult     analisis;
};

// Deducción "Reales Decretos" (RD 823/2008 art. 2.5): escala progresiva por
// tramos sobre la facturación MENSUAL de recetas (PVP+IVA) al SNS. Se aplica
// a la media mensual de la venta de receta anual y se anualiza (x12).
double calcularRealesDecretos(double ventaRecetaAnual);

// PMT de Excel: cuota constante (negativa) de un préstamo.
double pmt(double tasaMensual, int numPagos, double principal);

// TIR (IRR de Excel) de una serie de flujos de caja.
double irr(const std::vector<double>& cashflows, double guess = 0.1);

Results compute(const Inputs& in);

} // namespace sim
