import QtQuick
import QtTest
import FarmaciaSim

// Usa el singleton Engine real (recálculo real). Verifica el contrato de
// "mismas condiciones que la compra" de la hoja Venta (mismo coeficiente,
// mismo precio del local) y que el patrimonio acumulado cuadra con sus
// componentes fila a fila.
TestCase {
    id: testCase
    name: "VentaView"
    width: 1200
    height: 900
    visible: true
    when: windowShown

    Component {
        id: ventaComponent
        VentaView {}
    }

    function init() { Engine.resetToDefaults() }
    function cleanup() { Engine.resetToDefaults() }

    function fila(tabla, etiqueta) {
        return tabla.model.find(f => f.label === etiqueta)
    }

    function test_tableHasTenYearsAndEquityAddsUp() {
        const view = createTemporaryObject(ventaComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        compare(tabla.cols, 10)

        const patrimonio = testCase.fila(tabla, "Patrimonio acumulado")
        const sinSalario = testCase.fila(tabla, "Patrimonio sin contar el salario")
        const salario = testCase.fila(tabla, "Salario neto acumulado del titular")
        verify(patrimonio !== undefined)
        verify(sinSalario !== undefined)
        verify(salario !== undefined)

        for (let i = 0; i < 10; ++i)
            fuzzyCompare(patrimonio.values[i], sinSalario.values[i] + salario.values[i], 1e-6)
    }

    // El valor de venta usa el MISMO coeficiente de la compra (Financiación):
    // no hay un segundo coeficiente que mantener sincronizado.
    function test_goodwillValueFollowsPurchaseMultiple() {
        const view = createTemporaryObject(ventaComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        Engine.set("goodwillMultiple", 2.5)

        const fdc = testCase.fila(tabla, "Fondo de comercio (coef. 2,50)")
        verify(fdc !== undefined)
        const ventaTotal = testCase.fila(tabla, "Venta total del año")
        verify(ventaTotal !== undefined)

        for (let i = 0; i < 10; ++i)
            fuzzyCompare(fdc.values[i], ventaTotal.values[i] * 2.5, 1e-6)
    }

    // El local se vende al MISMO precio al que se compró, igual todos los
    // años (no se revaloriza).
    function test_premisesKeepThePurchasePrice() {
        const view = createTemporaryObject(ventaComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        Engine.set("premisesPrice", 250000)

        const local = testCase.fila(tabla, "Local comercial")
        verify(local !== undefined)
        for (let i = 0; i < 10; ++i)
            compare(local.values[i], 250000)
    }

    // Con todos los tramos de la escala del ahorro a 0 % no hay recorte.
    function test_zeroSaleTaxLeavesProceedsUntouched() {
        const view = createTemporaryObject(ventaComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        for (let k = 0; k < 5; ++k)
            Engine.set("savingsRate" + k, 0)

        const impuestos = testCase.fila(tabla, "Impuestos de la venta (escala del ahorro)")
        verify(impuestos !== undefined)
        const menosDeuda = testCase.fila(tabla, "Valor de venta menos deuda")
        const neta = testCase.fila(tabla, "Liquidez neta de la venta")
        for (let i = 0; i < 10; ++i) {
            compare(impuestos.values[i], 0)
            fuzzyCompare(neta.values[i], menosDeuda.values[i], 1e-6)
        }
    }
}
