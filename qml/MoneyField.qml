import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Campo editable en euros, ligado a una entrada del motor por su clave.
TextField {
    id: root

    required property string k
    property int decimals: 0
    property string suffix: " €"

    readonly property real value: Engine.inputs[k] !== undefined ? Engine.inputs[k] : 0

    function display() { return Fmt.num(value, decimals) + suffix }

    implicitWidth: 130
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
            text = Fmt.num(value, Math.max(decimals, 2)).replace(/\./g, "")
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
