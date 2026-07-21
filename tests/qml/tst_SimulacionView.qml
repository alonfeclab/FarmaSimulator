import QtQuick
import QtTest
import FarmaciaSim

// Usa el singleton Engine real (recálculo real). Verifica que colapsar o
// expandir una columna "Combinación N" en una de las 3 tablas (una por
// Facturación Total) se propaga a las otras dos, igual que el scroll
// horizontal sincronizado (ver page.collapsedColumns en SimulacionView.qml).
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
    function cleanup() { Engine.resetToDefaults() }

    function test_collapsingColumnSyncsAcrossTheThreeTables() {
        const view = createTemporaryObject(simulacionComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)

        const tabla0 = findChild(view, "tabla0")
        const tabla1 = findChild(view, "tabla1")
        const tabla2 = findChild(view, "tabla2")
        verify(tabla0 !== null)
        verify(tabla1 !== null)
        verify(tabla2 !== null)

        compare(tabla0.collapsedColumns.length, 0)
        compare(tabla1.collapsedColumns.length, 0)
        compare(tabla2.collapsedColumns.length, 0)

        // Simula el clic en el "ojo" de la columna 0 de la primera tabla
        // (el MouseArea del botón solo llama a root.toggleColumn(index)).
        tabla0.toggleColumn(0)

        compare(tabla0.collapsedColumns.indexOf(0) !== -1, true)
        compare(tabla1.collapsedColumns.indexOf(0) !== -1, true)
        compare(tabla2.collapsedColumns.indexOf(0) !== -1, true)

        // El ancho de la columna debe colapsarse en las 3, no solo en la primera.
        compare(tabla0.columnWidth(0), tabla0.collapsedWidth)
        compare(tabla1.columnWidth(0), tabla1.collapsedWidth)
        compare(tabla2.columnWidth(0), tabla2.collapsedWidth)

        // Expandir desde OTRA tabla (la del medio) también debe propagarse a las 3.
        tabla1.toggleColumn(0)

        compare(tabla0.collapsedColumns.indexOf(0), -1)
        compare(tabla1.collapsedColumns.indexOf(0), -1)
        compare(tabla2.collapsedColumns.indexOf(0), -1)
        compare(tabla0.columnWidth(0), tabla0.wCell)
        compare(tabla1.columnWidth(0), tabla1.wCell)
        compare(tabla2.columnWidth(0), tabla2.wCell)
    }
}
