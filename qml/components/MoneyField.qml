import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Campo editable en euros, ligado a una entrada del motor por su clave.
TextField {
    id: root

    required property string k
    property int decimals: 2
    property string suffix: " €"
    property bool invalid: false
    // Factor entre el valor mostrado/editado y el valor almacenado en el motor
    // (p.ej. 12 para editar en mensual un campo que el motor guarda en anual).
    property real multiplier: 1

    readonly property real value: Engine.inputs[k] !== undefined ? Engine.inputs[k] / multiplier : 0

    function display() { return Fmt.num(value, decimals) + suffix }

    implicitWidth: 130
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
        color: root.invalid ? Tokens.bgInputInvalid : Tokens.bgInput
        border.color: root.invalid ? Tokens.borderInputInvalid : (root.activeFocus ? Tokens.borderInteractiveHover : Tokens.borderInputDefault)
        border.width: root.invalid ? 2 : 1
    }

    Connections {
        target: Nav
        function onIrA(indice, foco) {
            if (foco === root.k) Qt.callLater(root.forceActiveFocus)
        }
    }

    Component.onCompleted: text = display()
    onValueChanged: if (!activeFocus) text = display()
    onActiveFocusChanged: {
        if (activeFocus) {
            text = Fmt.num(value, Math.max(decimals, 2))
            selectAll()
        } else {
            text = display()
        }
    }
    onTextEdited: {
        const r = Fmt.liveGroup(text, cursorPosition)
        if (r.text !== text) {
            text = r.text
            cursorPosition = r.cursor
        }
    }
    onAccepted: focus = false
    onEditingFinished: {
        const v = Fmt.parse(text)
        if (!isNaN(v))
            Engine.set(k, v * multiplier)
        text = display()
    }
}
