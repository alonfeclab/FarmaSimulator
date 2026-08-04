import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Card con título (mismo estilo que SectionTitle: negrita + línea de acento,
// para que se lea igual que cualquier otra sección de la app) que además se
// puede colapsar/expandir clicando en cualquier punto de la cabecera
// (título + icono ▾/▸), ocultando el contenido sin perderlo. headerContent
// queda fuera de esa zona clicable para no interferir con sus propios
// controles (p.ej. un ResetGroupButton). Componente común para cualquier
// grupo que lo necesite (p.ej. "Nuevo escenario" en SimulacionView.qml);
// con collapsible: false se comporta como un Card + SectionTitle normal,
// sin el icono, para poder reutilizarlo también donde no haga falta
// colapsar.
Card {
    id: root

    default property alias content: innerContent.data
    property alias title: sectionTitle.text
    // Contenido extra en la cabecera, a la derecha del título+icono. No
    // participa del área clicable de colapsar/expandir.
    property alias headerContent: extraHeader.data
    property bool collapsible: true
    property bool expanded: true

    Item {
        id: cabeceraClicable
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
                id: chevron
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

            // A la derecha del icono de colapsar, fuera del área clicable.
            RowLayout {
                id: extraHeader
                spacing: 8
            }
        }

        // Cubre título + icono (no extraHeader, que queda a su derecha,
        // fuera de este Item) para que toda esa zona colapse/expanda al clic.
        MouseArea {
            id: cabeceraHover
            enabled: root.collapsible
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.collapsible ? chevron.x + chevron.width : 0
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
