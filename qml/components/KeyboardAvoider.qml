import QtQuick
import QtQuick.Window

// En iOS/iPadOS la ventana no se redimensiona al mostrarse el teclado
// virtual: Qt solo expone su rectángulo mediante Qt.inputMethod. Este
// componente, colocado dentro de un Flickable, desplaza su contentY para
// que el campo con foco quede siempre por encima del teclado.
Item {
    id: root

    required property Flickable target
    property int margin: 16

    function ajustar() {
        const win = target.Window.window
        if (!win) return
        const item = win.activeFocusItem
        if (!item) return

        const kb = Qt.inputMethod.keyboardRectangle
        if (kb.height <= 0) return

        // keyboardRectangle llega en píxeles físicos (nativos de iOS/Android);
        // el resto de coordenadas QML son lógicas, hay que escalarlas.
        const dpr = Screen.devicePixelRatio || 1
        const kbY = kb.y / dpr

        // Techo del teclado, en coordenadas del "viewport" del Flickable
        // (0 = borde superior visible, target.height = borde inferior visible).
        const kbTop = target.mapFromItem(win.contentItem, 0, kbY).y
        const visibleBottom = Math.min(target.height, kbTop)

        const itemPos = item.mapToItem(target, 0, 0)
        const itemBottom = itemPos.y + item.height

        const overlap = itemBottom - visibleBottom + root.margin
        if (overlap > 0) {
            const maxY = Math.max(0, target.contentHeight - target.height)
            target.contentY = Math.min(maxY, target.contentY + overlap)
        }
    }

    Connections {
        target: Qt.inputMethod
        function onKeyboardRectangleChanged() { root.ajustar() }
    }

    Connections {
        target: root.target.Window.window
        ignoreUnknownSignals: true
        function onActiveFocusItemChanged() { root.ajustar() }
    }
}
