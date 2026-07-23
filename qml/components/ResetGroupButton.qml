import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Botón compacto para restaurar a su valor de fábrica solo las claves de un
// grupo de Configuración (a diferencia de "Restaurar valores" del panel
// lateral, que reinicia todo el simulador). En ventanas angostas se reduce a
// un botón cuadrado con el mismo icono "↺" usado en Comparación para aplicar
// un escenario, ya que el texto completo no cabe junto al título de la
// sección.
Button {
    id: btn
    required property var keys
    property bool compact: false

    text: "Restaurar valores por defecto"
    font.pixelSize: 11
    implicitHeight: 28
    leftPadding: compact ? 0 : 10
    rightPadding: compact ? 0 : 10
    implicitWidth: compact ? implicitHeight : implicitContentWidth + leftPadding + rightPadding
    onClicked: Engine.resetKeysToDefaults(btn.keys)
    background: Rectangle {
        radius: 6
        color: btn.down ? Tokens.bgAccentTint : Tokens.bgButtonGhostDefault
        border.color: (btn.hovered || btn.down) ? Tokens.borderInteractiveHover : Tokens.borderButtonGhostDefault
        border.width: 1
    }
    contentItem: Text {
        text: btn.compact ? "↺" : btn.text
        color: Tokens.textButtonGhost
        font.family: btn.font.family
        font.pixelSize: btn.compact ? 16 : btn.font.pixelSize
        font.bold: btn.compact
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    ToolTip.visible: compact && hovered
    ToolTip.text: btn.text
    ToolTip.delay: 400
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: (mouse) => mouse.accepted = false
    }
}
