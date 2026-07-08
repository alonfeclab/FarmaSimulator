import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Campo editable en porcentaje (guarda la fracción: 2,5 % -> 0.025).
TextField {
    id: root

    required property string k
    property int decimals: 1

    readonly property real value: Engine.inputs[k] !== undefined ? Engine.inputs[k] : 0

    function display() { return Fmt.num(value * 100, decimals) + " %" }

    implicitWidth: 80
    horizontalAlignment: TextInput.AlignRight
    font.pixelSize: 13
    color: "#1e2b28"
    selectByMouse: true

    background: Rectangle {
        radius: 5
        color: "#fffbe8"
        border.color: root.activeFocus ? "#1a7a5e" : "#e0d6ac"
        border.width: 1
    }

    Component.onCompleted: text = display()
    onValueChanged: if (!activeFocus) text = display()
    onActiveFocusChanged: {
        if (activeFocus) {
            text = Fmt.num(value * 100, decimals)
            selectAll()
        } else {
            text = display()
        }
    }
    onAccepted: focus = false
    onEditingFinished: {
        const v = Fmt.parse(text)
        if (!isNaN(v))
            Engine.set(k, v / 100)
        text = display()
    }
}
