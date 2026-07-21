import QtQuick
import QtTest
import FarmaciaSim

// Usa el singleton Engine real (recálculo real). El ejecutable de tests corre
// en su propio directorio de salida (ver tests/CMakeLists.txt), así que el
// farmaciasim_data.json que Engine guarda aquí no es el de la app real.
TestCase {
    id: testCase
    name: "MoneyField"
    width: 300
    height: 200
    visible: true
    when: windowShown

    Component {
        id: moneyFieldComponent
        MoneyField {}
    }

    // Vuelve Engine a los valores por defecto antes y después de cada test,
    // para que no se contaminen entre sí ni entre ejecuciones sucesivas.
    function init() { Engine.resetToDefaults() }
    function cleanup() { Engine.resetToDefaults() }

    function test_displayShowsCurrentEngineValue() {
        const field = createTemporaryObject(moneyFieldComponent, testCase, { k: "otcSales" })
        verify(field !== null)

        compare(field.value, Engine.inputs["otcSales"])
        compare(field.text, Fmt.num(Engine.inputs["otcSales"], 2) + " €")
    }

    function test_editingFinishedUpdatesEngineAndRedisplays() {
        const field = createTemporaryObject(moneyFieldComponent, testCase, { k: "otcSales" })
        verify(field !== null)

        field.text = "999.999,50"
        field.editingFinished()

        compare(Engine.inputs["otcSales"], 999999.50)
        compare(field.text, Fmt.num(999999.50, 2) + " €")
    }

    function test_invalidTextIsIgnoredAndKeepsPreviousValue() {
        const field = createTemporaryObject(moneyFieldComponent, testCase, { k: "otcSales" })
        verify(field !== null)
        const previo = Engine.inputs["otcSales"]

        field.text = "no es un número"
        field.editingFinished()

        compare(Engine.inputs["otcSales"], previo)
        compare(field.text, Fmt.num(previo, 2) + " €")
    }
}
