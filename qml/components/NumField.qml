import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Campo editable numérico simple (coeficientes, plazos, nº de personas…).
TextField {
    id: root

    required property string k
    property int decimals: 0
    property string suffix: ""

    readonly property real value: Engine.inputs[k] !== undefined ? Engine.inputs[k] : 0

    function display() { return Fmt.num(value, decimals) + suffix }

    implicitWidth: 80
    implicitHeight: 40
    horizontalAlignment: TextInput.AlignRight
    font.pixelSize: 14
    color: Tokens.textPrimary
    selectByMouse: true
    selectionColor: Tokens.bgSelection
    selectedTextColor: Tokens.textOnDark
    inputMethodHints: Qt.ImhFormattedNumbersOnly

    background: Rectangle {
        radius: 5
        color: Tokens.bgInput
        border.color: root.activeFocus ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
        border.width: 1
    }

    Component.onCompleted: text = display()
    onValueChanged: if (!activeFocus) text = display()
    onActiveFocusChanged: {
        if (activeFocus) {
            text = Fmt.num(value, decimals)
            selectAll()
        } else {
            text = display()
        }
    }
    onAccepted: focus = false
    onEditingFinished: {
        const v = Fmt.parse(text)
        if (!isNaN(v))
            Engine.set(k, v)
        text = display()
    }
}
