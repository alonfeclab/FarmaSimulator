import QtQuick
import QtTest
import FarmaciaSim

// Usa el singleton Engine real (recálculo real). Verifica que:
// - la columna 0 ("Actual") siempre existe y no lleva el botón "✕" de cerrar;
// - Engine.addSimulationScenario()/removeSimulationScenario() añaden y
//   quitan columnas de la única tabla de la hoja.
TestCase {
    id: testCase
    name: "SimulacionView"
    width: 1200
    height: 900
    visible: true
    when: windowShown

    Component {
        id: simulacionComponent
        SimulacionView {}
    }

    function init() { Engine.resetToDefaults() }
    function cleanup() {
        // resetToDefaults() no toca los escenarios guardados (no son un
        // input de Configuración): se limpian aparte para no dejar estado
        // entre tests.
        while (Engine.simulationScenarios.length > 0)
            Engine.removeSimulationScenario(0)
        Engine.resetToDefaults()
    }

    function test_defaultColumnIsPresentAndNotClosable() {
        const view = createTemporaryObject(simulacionComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)

        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        compare(tabla.cols, 1)
        compare(tabla.isColumnClosable(0), false)
    }

    function test_addSimulationScenarioAddsColumn() {
        const view = createTemporaryObject(simulacionComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        Engine.addSimulationScenario({ cashEur: 500000 })

        compare(tabla.cols, 2)
        compare(tabla.isColumnClosable(0), false)
        compare(tabla.isColumnClosable(1), true)

        const filaAportacion = tabla.model.find(f => f.label === "Aportación inicial")
        verify(filaAportacion !== undefined)
        compare(filaAportacion.values[0], Engine.inputs.contributedCash)
        compare(filaAportacion.values[1], 500000)
    }

    function test_removeSimulationScenarioRemovesColumn() {
        const view = createTemporaryObject(simulacionComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        Engine.addSimulationScenario({ marginPct: 0.4 })
        Engine.addSimulationScenario({ marginPct: 0.45 })
        compare(tabla.cols, 3)

        tabla.closeColumn(1) // borra el primer escenario añadido, no "Actual"

        compare(tabla.cols, 2)
        const filaMargen = tabla.model.find(f => f.label === "Margen comercial")
        compare(filaMargen.values[1], 0.45)
    }

    // Un escenario añadido sin editar ningún campo de "Nuevo escenario" no
    // fija ningún override: la columna sigue enteramente al escenario
    // principal, así que su valor cambia si cambia el principal.
    function test_scenarioWithNoOverridesTracksMainScenario() {
        const view = createTemporaryObject(simulacionComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)
        const tabla = findChild(view, "tabla")
        verify(tabla !== null)

        Engine.addSimulationScenario({})
        Engine.set("contributedCash", 600000)

        const filaAportacion = tabla.model.find(f => f.label === "Aportación inicial")
        compare(filaAportacion.values[0], 600000)
        compare(filaAportacion.values[1], 600000)
    }
}
