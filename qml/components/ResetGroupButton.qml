import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Botón compacto para restaurar a su valor de fábrica solo las claves de un
// grupo de Configuración (a diferencia de "Restaurar valores" del panel
// lateral, que reinicia todo el simulador).
Button {
    id: btn
    required property var keys

    text: "Restaurar valores por defecto"
    font.pixelSize: 11
    implicitHeight: 28
    leftPadding: 10
    rightPadding: 10
    onClicked: Engine.resetKeysToDefaults(btn.keys)
    background: Rectangle {
        radius: 6
        color: btn.down ? "#eef5f1" : "white"
        border.color: (btn.hovered || btn.down) ? "#1a7a5e" : "#c9d6d0"
        border.width: 1
    }
    contentItem: Text {
        text: btn.text
        color: "#1a5a45"
        font: btn.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: (mouse) => mouse.accepted = false
    }
}
