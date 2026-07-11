import QtQuick
import QtTest
import FarmaciaSim

TestCase {
    id: testCase
    name: "NavPanel"
    width: 300
    height: 600
    visible: true
    when: windowShown

    readonly property var hojasFake: [
        { nombre: "Uno",  icono: "" },
        { nombre: "Dos",  icono: "" },
        { nombre: "Tres", icono: "" }
    ]

    Component {
        id: navPanelComponent
        NavPanel {}
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    function test_clickEmitsNavigateWithClickedIndex() {
        const panel = createTemporaryObject(navPanelComponent, testCase,
            { hojas: testCase.hojasFake, width: 260, height: 500 })
        verify(panel !== null)

        const spy = signalSpyComponent.createObject(panel, { target: panel, signalName: "navigate" })
        const item = findChild(panel, "navItem1")
        verify(item !== null)

        mouseClick(item)

        compare(spy.count, 1)
        compare(spy.signalArguments[0][0], 1)
    }

    function test_currentIndexHighlightsSelectedItem() {
        const panel = createTemporaryObject(navPanelComponent, testCase,
            { hojas: testCase.hojasFake, currentIndex: 2, width: 260, height: 500 })
        verify(panel !== null)

        const seleccionado = findChild(panel, "navItem2")
        const otro = findChild(panel, "navItem0")
        verify(seleccionado !== null)
        verify(otro !== null)

        verify(Qt.colorEqual(seleccionado.color, "#1a7a5e"))
        verify(!Qt.colorEqual(otro.color, "#1a7a5e"))
    }
}
