import QtQuick

// En la build de WebAssembly de escritorio, la rueda del ratón llega a
// Flickable con deltas muy pequeños y encima Flickable les aplica su propia
// animación de inercia: el resultado es un scroll lentísimo y a tirones (el
// hilo único de WASM no da abasto con esa física). Este WheelHandler,
// colocado dentro de un Flickable, ignora esa física y mueve
// contentX/contentY directamente, a un ritmo fijo por muesca de rueda.
//
// Si "flick" no puede consumir el evento (dirección no permitida, o
// simplemente no hay overflow que mover — p.ej. una tabla pequeña sin
// scroll propio), se reenvía a mano a "fallback" (normalmente la página que
// contiene la tabla). No basta con dejar accepted=false y confiar en que
// Flickable lo propague solo: su wheelEvent nativo se queda con el evento
// en cuanto es "interactive", tenga o no overflow, así que sin este reenvío
// explícito el scroll de la página muere silenciosamente al pasar el ratón
// por encima de una tabla sin necesidad de scroll (ver fix "Scroll fix for
// webassembly on pc").
WheelHandler {
    id: root

    // Ojo: no se llama "target" porque WheelHandler ya tiene esa propiedad
    // (de tipo Item, para manipulación automática) y no queremos que la
    // tape ni la use: gestionamos el scroll a mano en onWheel.
    required property Flickable flick
    property Flickable fallback: null
    property real pixelsPerDegree: 4

    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    target: null

    function tryScroll(f, dx, dy) {
        const vertOk = f.flickableDirection === Flickable.AutoFlickDirection
                    || f.flickableDirection === Flickable.VerticalFlick
                    || f.flickableDirection === Flickable.HorizontalAndVerticalFlick
        const horOk = f.flickableDirection === Flickable.AutoFlickDirection
                    || f.flickableDirection === Flickable.HorizontalFlick
                    || f.flickableDirection === Flickable.HorizontalAndVerticalFlick

        const canV = vertOk && f.contentHeight > f.height
        const canH = horOk && f.contentWidth > f.width

        let handled = false
        if (canV && dy !== 0) {
            f.contentY = Math.max(0, Math.min(f.contentHeight - f.height, f.contentY - dy))
            handled = true
        }
        if (canH && dx !== 0) {
            f.contentX = Math.max(0, Math.min(f.contentWidth - f.width, f.contentX - dx))
            handled = true
        }
        return handled
    }

    onWheel: (event) => {
        const dy = event.pixelDelta.y !== 0 ? event.pixelDelta.y : (event.angleDelta.y / 8) * root.pixelsPerDegree
        const dx = event.pixelDelta.x !== 0 ? event.pixelDelta.x : (event.angleDelta.x / 8) * root.pixelsPerDegree

        let handled = root.tryScroll(root.flick, dx, dy)
        if (!handled && root.fallback)
            handled = root.tryScroll(root.fallback, dx, dy)

        // Aceptamos siempre que haya un sitio (propio o de fallback) al que
        // este WheelHandler es responsable de llevar la rueda, se haya
        // movido algo o no (p.ej. ya está en el límite): así el wheelEvent
        // nativo del Flickable nunca llega a intervenir.
        event.accepted = true
    }
}
