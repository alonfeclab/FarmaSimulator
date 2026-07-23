import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Card con título (mismo estilo que SectionTitle: negrita + línea de acento,
// para que se lea igual que cualquier otra sección de la app) que además se
// puede colapsar/expandir con un icono junto al título, ocultando el
// contenido sin perderlo. Componente común para cualquier grupo que lo
// necesite (p.ej. "Nuevo escenario" en SimulacionView.qml); con
// collapsible: false se comporta como un Card + SectionTitle normal, sin el
// icono, para poder reutilizarlo también donde no haga falta colapsar.
Card {
    id: root

    default property alias content: innerContent.data
    property alias title: sectionTitle.text
    property bool collapsible: true
    property bool expanded: true

    Item {
        Layout.fillWidth: true
        implicitHeight: cabecera.implicitHeight

        RowLayout {
            id: cabecera
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 10

            SectionTitle {
                id: sectionTitle
                Layout.fillWidth: true
            }
            Rectangle {
                visible: root.collapsible
                implicitWidth: 28
                implicitHeight: 28
                radius: 6
                color: cabeceraHover.hovered ? Tokens.bgAccentTint : Tokens.bgButtonGhostDefault
                border.color: cabeceraHover.hovered ? Tokens.borderInteractiveHover : Tokens.borderButtonGhostDefault
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: root.expanded ? "▾" : "▸"
                    font.pixelSize: 16
                    font.bold: true
                    color: Tokens.textButtonGhost
                }
            }
        }
        MouseArea {
            id: cabeceraHover
            enabled: root.collapsible
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.expanded = !root.expanded
        }
    }

    ColumnLayout {
        id: innerContent
        Layout.fillWidth: true
        visible: !root.collapsible || root.expanded
        spacing: 10
    }
}
