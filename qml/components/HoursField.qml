import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Campo editable en horas/día de jornada (8 h = 100%; guarda la fracción: 4 h -> 0.5).
TextField {
    id: root

    required property string k
    property int decimals: 1
    readonly property real hoursFullTime: 8

    readonly property real value: Engine.inputs[k] !== undefined ? Engine.inputs[k] : 0

    function display() { return Fmt.num(value * hoursFullTime, decimals) + " h" }

    implicitWidth: 80
    implicitHeight: 40
    horizontalAlignment: TextInput.AlignRight
    font.pixelSize: 14
    color: "#1e2b28"
    selectByMouse: true
    inputMethodHints: Qt.ImhFormattedNumbersOnly

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
            text = Fmt.num(value * hoursFullTime, decimals)
            selectAll()
        } else {
            text = display()
        }
    }
    onAccepted: focus = false
    onEditingFinished: {
        const v = Fmt.parse(text)
        if (!isNaN(v))
            Engine.set(k, v / hoursFullTime)
        text = display()
    }
}
