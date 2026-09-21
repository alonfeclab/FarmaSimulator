import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Checkbox con el estilo de la app. Se usa en "Escenario de crecimiento"
// (Datos base) para "Mismo crecimiento".
CheckBox {
    id: root

    font.pixelSize: 13

    indicator: Rectangle {
        x: root.leftPadding
        y: (root.height - height) / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 4
        color: root.checked ? Tokens.bgButtonPrimaryDefault : Tokens.bgInput
        border.color: root.checked ? Tokens.borderButtonPrimary : Tokens.borderInputDefault
        border.width: 1
        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: "✓"
            font.pixelSize: 14
            font.bold: true
            color: Tokens.textOnDark
        }
    }
    contentItem: Text {
        text: root.text
        font: root.font
        color: Tokens.textSecondary
        leftPadding: root.indicator.width + root.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
