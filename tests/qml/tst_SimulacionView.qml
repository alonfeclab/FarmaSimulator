import QtQuick
import QtTest
import FarmaciaSim

// Usa el singleton Engine real (recálculo real). Verifica que colapsar o
// expandir una columna "Combinación N" en una de las 2 tablas (una por
// Facturación Total: igual / + un % configurable) NO se propaga a la otra tabla — cada
// tabla lleva su propio filtro de columnas (ver grupoCol.collapsedColumns en
// SimulacionView.qml) — mientras que el scroll horizontal sí sigue
// sincronizado entre ambas.
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

    function test_collapsingColumnOnlyAffectsItsOwnTable() {
        const view = createTemporaryObject(simulacionComponent, testCase, { width: 1200, height: 900 })
        verify(view !== null)

        const tabla0 = findChild(view, "tabla0")
        const tabla1 = findChild(view, "tabla1")
        verify(tabla0 !== null)
        verify(tabla1 !== null)

        compare(tabla0.collapsedColumns.length, 0)
        compare(tabla1.collapsedColumns.length, 0)

        // Simula el clic en el "ojo" de la columna 0 de la primera tabla
        // (el MouseArea del botón solo llama a root.toggleColumn(index)).
        tabla0.toggleColumn(0)

        compare(tabla0.collapsedColumns.indexOf(0) !== -1, true)
        compare(tabla1.collapsedColumns.indexOf(0), -1)

        // El ancho de la columna solo debe colapsarse en la tabla donde se
        // pulsó el "ojo", no en la otra.
        compare(tabla0.columnWidth(0), tabla0.collapsedWidth)
        compare(tabla1.columnWidth(0), tabla1.wCell)

        // Colapsar en la otra tabla tampoco debe afectar a la primera.
        tabla1.toggleColumn(1)

        compare(tabla1.collapsedColumns.indexOf(1) !== -1, true)
        compare(tabla0.collapsedColumns.indexOf(1), -1)

        // Expandir de nuevo solo afecta a la tabla en la que se colapsó.
        tabla0.toggleColumn(0)

        compare(tabla0.collapsedColumns.indexOf(0), -1)
        compare(tabla0.columnWidth(0), tabla0.wCell)
        compare(tabla1.collapsedColumns.indexOf(1) !== -1, true)
    }
}
