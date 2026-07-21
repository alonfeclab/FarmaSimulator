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

    function byFmt(v, f) {
        if (f === "pct1") return pct(v, 1)
        if (f === "pct2") return pct(v, 2)
        if (f === "num")  return num(v, 1)
        if (f === "years") return num(v, 0) + " años"
        return eur(v)
    }
}
