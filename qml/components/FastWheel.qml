import QtQuick

// En la build de WebAssembly de escritorio, la rueda del ratón llega a
// Flickable con deltas muy pequeños y encima Flickable les aplica su propia
// animación de inercia: el resultado es un scroll lentísimo y a tirones (el
// hilo único de WASM no da abasto con esa física). Este WheelHandler,
// colocado dentro de un Flickable, ignora esa física y mueve
// contentX/contentY directamente, a un ritmo fijo por muesca de rueda.
//
// Si el evento no corresponde a una dirección habilitada en
// flick.flickableDirection (p.ej. rueda vertical sobre una tabla que solo
// permite scroll horizontal), se deja sin aceptar para que siga
// propagándose hacia el Flickable padre (ver fix "Scroll fix for
// webassembly on pc").
WheelHandler {
    id: root

    // Ojo: no se llama "target" porque WheelHandler ya tiene esa propiedad
    // (de tipo Item, para manipulación automática) y no queremos que la
    // tape ni la use: gestionamos el scroll a mano en onWheel.
    required property Flickable flick
    property real pixelsPerDegree: 4

    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    target: null

    readonly property bool vertOk: root.flick.flickableDirection === Flickable.AutoFlickDirection
                                 || root.flick.flickableDirection === Flickable.VerticalFlick
                                 || root.flick.flickableDirection === Flickable.HorizontalAndVerticalFlick
    readonly property bool horOk: root.flick.flickableDirection === Flickable.AutoFlickDirection
                                 || root.flick.flickableDirection === Flickable.HorizontalFlick
                                 || root.flick.flickableDirection === Flickable.HorizontalAndVerticalFlick

    onWheel: (event) => {
        const dy = event.pixelDelta.y !== 0 ? event.pixelDelta.y : (event.angleDelta.y / 8) * root.pixelsPerDegree
        const dx = event.pixelDelta.x !== 0 ? event.pixelDelta.x : (event.angleDelta.x / 8) * root.pixelsPerDegree

        const canV = root.vertOk && root.flick.contentHeight > root.flick.height
        const canH = root.horOk && root.flick.contentWidth > root.flick.width

        let handled = false
        if (canV && dy !== 0) {
            root.flick.contentY = Math.max(0, Math.min(root.flick.contentHeight - root.flick.height, root.flick.contentY - dy))
            handled = true
        }
        if (canH && dx !== 0) {
            root.flick.contentX = Math.max(0, Math.min(root.flick.contentWidth - root.flick.width, root.flick.contentX - dx))
            handled = true
        }
        event.accepted = handled
    }
}
