import QtQuick
import QtTest
import FarmaciaSim

// Verifica el resumen que se muestra en el tooltip de una columna colapsada
// (ver columnSummary() y el botón "ojo" en ConceptTable.qml).
TestCase {
    id: testCase
    name: "ConceptTable"
    width: 400
    height: 300
    visible: true
    when: windowShown

    Component {
        id: tableComponent
        ConceptTable {}
    }

    function test_columnSummaryIncludesHeaderAndFormattedRowValues() {
        const table = createTemporaryObject(tableComponent, testCase, {
            width: 400,
            height: 300,
            headerLabels: ["Escenario 1", "Escenario 2"],
            model: [
                { label: "Años hipoteca", values: [20, 25], fmt: "years", bold: false },
                { label: "Aportación inicial", values: [400000, 450000], fmt: "eur", bold: false }
            ]
        })
        verify(table !== null)

        const resumen = table.columnSummary(1)
        compare(resumen, "Escenario 2\nAños hipoteca: 25 años\nAportación inicial: 450.000 €")
    }

    function test_columnSummaryFallsBackToDefaultHeaderAndSkipsSeparators() {
        const table = createTemporaryObject(tableComponent, testCase, {
            width: 400,
            height: 300,
            model: [
                { label: "Grupo", separator: true },
                { label: "Beneficio neto mensual", values: [1200], fmt: "eur", bold: true }
            ]
        })
        verify(table !== null)

        // Sin headerLabels, usa el "Año N" habitual.
        const resumen = table.columnSummary(0)
        compare(resumen, "Año 1\nBeneficio neto mensual: 1.200 €")
    }

    // firstClosableColumn permite excluir las primeras columnas del botón
    // "✕" (p.ej. la columna "Actual" en Simulación no debe poder borrarse).
    function test_firstClosableColumnExcludesLeadingColumnsFromClosing() {
        const table = createTemporaryObject(tableComponent, testCase, {
            width: 400,
            height: 300,
            closableColumns: true,
            firstClosableColumn: 1,
            headerLabels: ["Actual", "Escenario 1", "Escenario 2"],
            model: [ { label: "Aportación inicial", values: [400000, 450000, 500000], fmt: "eur", bold: false } ]
        })
        verify(table !== null)

        compare(table.isColumnClosable(0), false)
        compare(table.isColumnClosable(1), true)
        compare(table.isColumnClosable(2), true)
    }
}
