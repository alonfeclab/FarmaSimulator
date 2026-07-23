pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

ApplicationWindow {
    id: win

    width: 1360
    height: 860
    minimumWidth: 340
    minimumHeight: 480
    visible: true
    title: "Simulación Farmacia"
    color: Tokens.bgApp

    // Por debajo de este ancho la barra lateral se convierte en un
    // menú deslizante (Drawer) para dejar sitio al contenido en
    // móviles y en iPad en vertical.
    readonly property int breakpointCompacto: 760
    readonly property bool compact: width < breakpointCompacto

    // Barra lateral fija: colapsable a solo iconos para ganar espacio
    // horizontal al contenido (no aplica al Drawer, que ya se abre/cierra).
    property bool navCollapsed: false

    readonly property var hojas: [
        { nombre: "Datos base",          icono: "qrc:/qt/qml/FarmaciaSim/icons/datos_base.svg" },
        { nombre: "Financiación",        icono: "qrc:/qt/qml/FarmaciaSim/icons/financiacion.svg" },
        { nombre: "Proyección 10 años",  icono: "qrc:/qt/qml/FarmaciaSim/icons/proyeccion.svg" },
        // Impuestos y Análisis inversión ocultos temporalmente (no borrar).
        { nombre: "Amort. banco",        icono: "qrc:/qt/qml/FarmaciaSim/icons/banco.svg" },
        { nombre: "Amort. cooperativa",  icono: "qrc:/qt/qml/FarmaciaSim/icons/cooperativa.svg" },
        { nombre: "Amort. familiar",     icono: "qrc:/qt/qml/FarmaciaSim/icons/familiar.svg" },
        { nombre: "Amort. propiedades",  icono: "qrc:/qt/qml/FarmaciaSim/icons/propiedades.svg" },
        { nombre: "Personal",            icono: "qrc:/qt/qml/FarmaciaSim/icons/personal.svg" },
        { nombre: "Comparación",         icono: "qrc:/qt/qml/FarmaciaSim/icons/comparacion.svg" },
        { nombre: "Simulación",          icono: "qrc:/qt/qml/FarmaciaSim/icons/simulacion.svg" },
        { nombre: "Configuración",       icono: "qrc:/qt/qml/FarmaciaSim/icons/configuracion.svg" }
    ]

    header: ToolBar {
        visible: win.compact
        height: visible ? 52 : 0
        background: Rectangle { color: Tokens.bgNav }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 12
            spacing: 4

            ToolButton {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                contentItem: Text {
                    text: "☰"
                    color: Tokens.textOnDark
                    font.pixelSize: 22
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Item {}
                onClicked: drawer.open()
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item { Layout.fillWidth: true }
                Image {
                    source: win.hojas[stack.currentIndex].icono
                    sourceSize: Qt.size(20, 20)
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                }
                Text {
                    text: win.hojas[stack.currentIndex].nombre
                    color: Tokens.textOnDark
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }
                Item { Layout.fillWidth: true }
            }
            Item { Layout.preferredWidth: 48 }
        }
    }

    Connections {
        target: Nav
        function onIrA(indice, foco) {
            // Si vamos a enfocar un campo concreto, no reseteemos el scroll a top.
            if (foco !== "" && indice !== stack.currentIndex) stack.suppressScrollReset = true
            stack.currentIndex = indice
        }
    }

    Drawer {
        id: drawer
        width: Math.min(280, win.width * 0.82)
        height: win.height
        interactive: win.compact
        dragMargin: win.compact ? Qt.styleHints.startDragDistance : 0

        NavPanel {
            anchors.fill: parent
            hojas: win.hojas
            currentIndex: stack.currentIndex
            compact: true
            onNavigate: (index) => {
                stack.currentIndex = index
                drawer.close()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NavPanel {
            visible: !win.compact
            Layout.preferredWidth: win.navCollapsed ? 0 : 230
            Layout.fillHeight: true
            clip: true
            hojas: win.hojas
            currentIndex: stack.currentIndex
            onNavigate: (index) => stack.currentIndex = index

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }
        }

        // Franja angosta pegada al borde derecho del menú: permite
        // colapsarlo (ocultando título, pestañas y botones) o volver a
        // expandirlo, para ganar ancho horizontal para el contenido.
        Rectangle {
            id: navCollapseStrip
            visible: !win.compact
            Layout.preferredWidth: 20
            Layout.fillHeight: true
            color: collapseArea.containsMouse ? Tokens.bgNavItemHover : Tokens.bgNav

            Text {
                anchors.centerIn: parent
                text: win.navCollapsed ? "»" : "«"
                color: Tokens.textOnDark
                font.pixelSize: 14
                font.bold: true
            }
            MouseArea {
                id: collapseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: win.navCollapsed = !win.navCollapsed
            }
            ToolTip.visible: collapseArea.containsMouse
            ToolTip.delay: 400
            ToolTip.text: win.navCollapsed ? "Expandir menú" : "Colapsar menú"
        }

        // ------------------------------------------------ contenido
        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            property bool suppressScrollReset: false

            // Al cambiar de pestaña, la vista siempre arranca con el scroll arriba del todo,
            // salvo que la navegación traiga un campo concreto que enfocar (ver Nav.irA).
            onCurrentIndexChanged: {
                if (stack.suppressScrollReset) {
                    stack.suppressScrollReset = false
                    return
                }
                const item = stack.itemAt(stack.currentIndex)
                if (!item) return
                if (typeof item.resetScroll === "function") item.resetScroll()
                else if (item.contentY !== undefined) item.contentY = 0
            }

            DatosBaseView {}
            FinanciacionView {}
            ProyeccionView {}
            // ImpuestosView y AnalisisView ocultos temporalmente (no borrar).
            AmortView {
                loan: Engine.bank
                emptyIcono: "qrc:/qt/qml/FarmaciaSim/icons/banco.svg"
            }
            AmortView {
                loan: Engine.cooperative
                emptyTexto: "No hay préstamo de cooperativa.\nAñádelo en Financiación."
                emptyIcono: "qrc:/qt/qml/FarmaciaSim/icons/cooperativa.svg"
                emptyFocusKey: "initialOrder"
            }
            AmortView {
                loan: Engine.family
                emptyTexto: "No hay préstamo familiar.\nAñádelo en Financiación."
                emptyIcono: "qrc:/qt/qml/FarmaciaSim/icons/familiar.svg"
                emptyFocusKey: "familyContribution"
            }
            AmortView {
                loan: Engine.properties
                emptyTexto: "No hay financiación de propiedades.\nAñádela en Financiación."
                emptyIcono: "qrc:/qt/qml/FarmaciaSim/icons/propiedades.svg"
                emptyFocusKey: "propertiesFinancing"
            }
            PersonalView {}
            ComparacionView {}
            SimulacionView {}
            ConfiguracionView {}
        }
    }
}
