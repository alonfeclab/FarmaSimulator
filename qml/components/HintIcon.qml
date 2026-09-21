import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Icono "?" con un tooltip de ayuda (p. ej. "Modifica el % de ... en
// Configuración"). Se muestra al pasar el ratón o, en táctil, al tocarlo.
// Solo es visible si 'text' no está vacío.
Rectangle {
    id: root

    property string text: ""

    visible: text !== ""
    implicitWidth: 18
    implicitHeight: 18
    radius: 9
    color: "transparent"
    border.color: area.containsMouse ? Tokens.borderInteractiveHover : Tokens.textMuted
    border.width: 1

    Text {
        anchors.centerIn: parent
        text: "?"
        font.pixelSize: 11
        font.bold: true
        color: area.containsMouse ? Tokens.textHeading : Tokens.textMuted
    }
    MouseArea {
        id: area
        anchors.fill: parent
        anchors.margins: -4
        hoverEnabled: true
        onClicked: tip.visible = !tip.visible
        onContainsMouseChanged: tip.visible = containsMouse
    }
    ToolTip {
        id: tip
        text: root.text
        // Textos largos (p. ej. fórmulas) saltan de línea en vez de
        // ocupar todo el ancho de la ventana.
        width: Math.min(implicitWidth, 340)
        delay: 200
        timeout: 5000
    }
}
