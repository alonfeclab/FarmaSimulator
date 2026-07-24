pragma Singleton
import QtQuick

// Formateo de números al estilo del Excel (es-ES): "1.234.567 €", "2,5 %"
QtObject {
    // Number.toLocaleString() de QML no agrupa los miles correctamente
    // cuando se piden decimales (p. ej. "1234,50" en vez de "1.234,50" en
    // Qt 6.11), así que se agrupa a mano en su lugar.
    function num(v, dec) {
        const d = dec === undefined ? 0 : dec
        const n = Number(v)
        if (isNaN(n)) return ""
        const negativo = n < 0
        const partes = Math.abs(n).toFixed(d).split(".")
        partes[0] = partes[0].replace(/\B(?=(\d{3})+(?!\d))/g, ".")
        return (negativo ? "-" : "") + partes.join(",")
    }

    function eur(v)  { return num(v, 0) + " €" }
    function eur2(v) { return num(v, 2) + " €" }
    function pct(v, dec) { return num(v * 100, dec === undefined ? 1 : dec) + " %" }

    // "1.234,56" -> 1234.56  (admite espacios, € y %)
    function parse(t) {
        const clean = String(t).replace(/[^\d,.\-]/g, "")
                               .replace(/\./g, "").replace(",", ".")
        return parseFloat(clean)
    }

    // Reagrupa los miles de "text" mientras se escribe, conservando la
    // posición del cursor (en dígitos, no en caracteres) para que insertar/
    // borrar en medio del número no salte el cursor de sitio.
    function groupInt(intPart) {
        const neg = intPart.charAt(0) === "-"
        const digits = neg ? intPart.slice(1) : intPart
        const grouped = digits.replace(/\B(?=(\d{3})+(?!\d))/g, ".")
        return (neg ? "-" : "") + grouped
    }

    function liveGroup(text, cursorPos) {
        const digitsBefore = text.slice(0, cursorPos).replace(/\./g, "").length
        const raw = text.replace(/\./g, "")
        const commaIdx = raw.indexOf(",")
        const intPart = commaIdx === -1 ? raw : raw.slice(0, commaIdx)
        const rest = commaIdx === -1 ? "" : raw.slice(commaIdx)
        const groupedInt = groupInt(intPart)

        let newCursor
        if (digitsBefore <= intPart.length) {
            let seen = 0, periodsBefore = 0
            for (let i = 0; i < groupedInt.length && seen < digitsBefore; i++) {
                if (groupedInt.charAt(i) === ".") periodsBefore++
                else seen++
            }
            newCursor = digitsBefore + periodsBefore
        } else {
            newCursor = groupedInt.length + (digitsBefore - intPart.length)
        }
        return { text: groupedInt + rest, cursor: newCursor }
    }

    function byFmt(v, f) {
        if (f === "pct1") return pct(v, 1)
        if (f === "pct2") return pct(v, 2)
        if (f === "num")  return num(v, 1)
        if (f === "years") return num(v, 0) + " años"
        return eur(v)
    }
}
