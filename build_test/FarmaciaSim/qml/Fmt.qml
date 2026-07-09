pragma Singleton
import QtQuick

// Formateo de números al estilo del Excel (es-ES): "1.234.567 €", "2,5 %"
QtObject {
    readonly property var loc: Qt.locale("es_ES")

    function eur(v)  { return Number(v).toLocaleString(loc, 'f', 0) + " €" }
    function eur2(v) { return Number(v).toLocaleString(loc, 'f', 2) + " €" }
    function pct(v, dec) {
        return Number(v * 100).toLocaleString(loc, 'f', dec === undefined ? 1 : dec) + " %"
    }
    function num(v, dec) { return Number(v).toLocaleString(loc, 'f', dec || 0) }

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
        return eur(v)
    }
}
