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
    color: "#eef2f0"

    // Por debajo de este ancho la barra lateral se convierte en un
    // menú deslizante (Drawer) para dejar sitio al contenido en
    // móviles y en iPad en vertical.
    readonly property int breakpointCompacto: 760
    readonly property bool compact: width < breakpointCompacto

    readonly property var hojas: [
        { nombre: "Datos Base",          icono: "📋" },
        { nombre: "Financiación",        icono: "🏦" },
        { nombre: "Proyección 10 Años",  icono: "📈" },
        { nombre: "Impuestos",           icono: "🧾" },
        { nombre: "Análisis Inversión",  icono: "🔍" },
        { nombre: "Amort. Banco",        icono: "💶" },
        { nombre: "Amort. Cooperativa",  icono: "🤝" },
        { nombre: "Amort. Propiedades",  icono: "🏠" },
        { nombre: "Personal",            icono: "👥" }
    ]

    header: ToolBar {
        visible: win.compact
        height: visible ? 52 : 0
        background: Rectangle { color: "#123f31" }

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
                    color: "white"
                    font.pixelSize: 22
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Item {}
                onClicked: drawer.open()
            }
            Text {
                text: win.hojas[stack.currentIndex].nombre
                color: "white"
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            Item { Layout.preferredWidth: 48 }
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
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            hojas: win.hojas
            currentIndex: stack.currentIndex
            onNavigate: (index) => stack.currentIndex = index
        }

        // ------------------------------------------------ contenido
        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            DatosBaseView {}
            FinanciacionView {}
            ProyeccionView {}
            ImpuestosView {}
            AnalisisView {}
            AmortView { loan: Engine.banco }
            AmortView { loan: Engine.cooperativa }
            AmortView { loan: Engine.propiedades }
            PersonalView {}
        }
    }
}
