pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Configuración": escalas y series oficiales que usa el motor de
// cálculo (IRPF, RETA, Reales Decretos, IPC histórico, margen comercial del
// escenario Realista...). Editables, agrupadas por concepto, con los valores
// vigentes como valor por defecto ("Restaurar valores" en el panel lateral
// también las restaura).
Flickable {
    id: page

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Listas de claves por grupo, usadas por los botones "Restaurar valores
    // por defecto" (cada uno solo toca las claves de su propio grupo).
    function irpfKeys() {
        var ks = []
        for (var k = 0; k < 6; k++) { ks.push("irpfFrom" + k); ks.push("irpfTo" + k); ks.push("irpfRate" + k) }
        return ks
    }
    function retaKeys() {
        var ks = ["retaFlatMonthlyFee"]
        for (var k = 0; k < 15; k++) { ks.push("retaFrom" + k); ks.push("retaQuota" + k) }
        return ks
    }
    function rdKeys() {
        var ks = []
        for (var k = 0; k < 9; k++) { ks.push("rdFrom" + k); ks.push("rdBase" + k); ks.push("rdPct" + k) }
        return ks
    }
    function fixedPctKeys() {
        return ["feesPct", "ivaPct", "itpPct", "ajdPct", "inventoryPctYear10"]
    }
    function historicalSeriesKeys() {
        var ks = []
        for (var k = 0; k < 10; k++) { ks.push("ipcHistorical" + k); ks.push("realisticMarginSeries" + k) }
        return ks
    }

    // Cabecera de columnas de una tabla de tramos.
    component CabeceraTabla: Row {
        id: cabecera
        required property var etiquetas
        property int wCell: 148
        Repeater {
            model: cabecera.etiquetas
            Rectangle {
                id: hdrCell
                required property string modelData
                width: cabecera.wCell; height: 30; color: "#14523f"
                Text {
                    anchors.centerIn: parent
                    text: hdrCell.modelData
                    color: "white"; font.bold: true; font.pixelSize: 12
                }
            }
        }
    }

    // Celda de tabla con el fondo a rayas habitual; el campo editable va
    // dentro, como hijo directo, con "anchors.centerIn: parent" propio.
    component Celda: Rectangle {
        property bool par: true
        property int wCell: 148
        implicitWidth: wCell
        implicitHeight: 44
        color: par ? "white" : "#f7faf8"
    }

    // Serie editable de 10 años (IPC histórico, margen comercial...).
    component SerieAnualEdit: ColumnLayout {
        id: serie
        required property string prefix
        property int wCell: 84
        Layout.fillWidth: true
        spacing: 0

        Row {
            Repeater {
                model: 10
                Rectangle {
                    id: hdrCell
                    required property int index
                    width: serie.wCell; height: 26; color: "#14523f"
                    Text {
                        anchors.centerIn: parent
                        text: "Año " + (hdrCell.index + 1)
                        color: "white"; font.bold: true; font.pixelSize: 11
                    }
                }
            }
        }
        Row {
            Repeater {
                model: 10
                Rectangle {
                    id: celda
                    required property int index
                    width: serie.wCell; height: 40
                    color: celda.index % 2 ? "#f7faf8" : "white"
                    PctField {
                        anchors.centerIn: parent
                        implicitWidth: serie.wCell - 8
                        k: serie.prefix + celda.index
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: Math.min(page.width - 88, 1360)
        spacing: 14

        Text {
            text: "Configuración"
            font.pixelSize: 22;
            font.bold: true;
            color: "#14523f"
        }
        Text {
            text: "Escalas y series oficiales usadas por el simulador. Son editables, con los valores "
                + "vigentes como punto de partida; \"Restaurar valores\" (panel lateral) también las restaura."
            font.pixelSize: 13
            color: "#6b7a76"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        // ---------------- IRPF
        Card {
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                SectionTitle { text: "IRPF — escala general 2026"; Layout.fillWidth: true }
                ResetGroupButton { keys: page.irpfKeys() }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Tramos de la base liquidable general, usados para calcular la cuota escala en la hoja Impuestos."
            }
            Flickable {
                id: scroll1
                Layout.fillWidth: true
                Layout.preferredHeight: tabla1.implicitHeight
                contentWidth: tabla1.implicitWidth
                contentHeight: tabla1.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll1; fallback: page }

                ColumnLayout {
                    id: tabla1
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Desde", "Hasta", "Tipo"] }
                    Repeater {
                        model: 6
                        Row {
                            id: filaIrpf
                            required property int index
                            Celda { par: filaIrpf.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "irpfFrom" + filaIrpf.index } }
                            Celda { par: filaIrpf.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "irpfTo" + filaIrpf.index } }
                            Celda { par: filaIrpf.index % 2 === 0; PctField   { anchors.centerIn: parent; k: "irpfRate"  + filaIrpf.index; decimals: 1 } }
                        }
                    }
                }
            }
        }

        // ---------------- RETA autónomos
        Card {
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                SectionTitle { text: "RETA — cuota de autónomos"; Layout.fillWidth: true }
                ResetGroupButton { keys: page.retaKeys() }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Tarifa plana durante los primeros 12 meses de actividad. A partir del segundo año se "
                    + "aplica la escala por tramos de rendimientos netos mensuales."
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "Tarifa plana (mensual)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                MoneyField { k: "retaFlatMonthlyFee"; decimals: 2 }
            }
            Flickable {
                id: scroll2
                Layout.fillWidth: true
                Layout.preferredHeight: tabla2.implicitHeight
                contentWidth: tabla2.implicitWidth
                contentHeight: tabla2.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll2; fallback: page }

                ColumnLayout {
                    id: tabla2
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Rend. neto desde", "Cuota mensual"]; wCell: 160 }
                    Repeater {
                        model: 15
                        Row {
                            id: filaReta
                            required property int index
                            Celda { wCell: 160; par: filaReta.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "retaFrom" + filaReta.index; decimals: 2 } }
                            Celda { wCell: 160; par: filaReta.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "retaQuota" + filaReta.index; decimals: 2 } }
                        }
                    }
                }
            }
        }

        // ---------------- Reales Decretos
        Card {
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                SectionTitle { text: "Reales decretos (RD 823/2008, art. 2.5)"; Layout.fillWidth: true }
                ResetGroupButton { keys: page.rdKeys() }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Escala de deducción sobre la facturación mensual de recetas (PVP+IVA) al SNS, usada en Datos base."
            }
            Flickable {
                id: scroll3
                Layout.fillWidth: true
                Layout.preferredHeight: tabla3.implicitHeight
                contentWidth: tabla3.implicitWidth
                contentHeight: tabla3.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll3; fallback: page }

                ColumnLayout {
                    id: tabla3
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Facturación desde", "Base deducción", "% s/exceso"]; wCell: 160 }
                    Repeater {
                        model: 9
                        Row {
                            id: filaRd
                            required property int index
                            Celda { wCell: 160; par: filaRd.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "rdFrom" + filaRd.index } }
                            Celda { wCell: 160; par: filaRd.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "rdBase"  + filaRd.index; decimals: 2 } }
                            Celda { wCell: 160; par: filaRd.index % 2 === 0; PctField   { anchors.centerIn: parent; k: "rdPct"   + filaRd.index; decimals: 1 } }
                        }
                    }
                }
            }
        }

        // ---------------- Porcentajes fijos de la compraventa
        Card {
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                SectionTitle { text: "Compraventa de la farmacia — porcentajes fijos"; Layout.fillWidth: true }
                ResetGroupButton { keys: page.fixedPctKeys() }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Usados en Financiación e Impuestos para calcular honorarios, impuestos de la compraventa y existencias estimadas a 10 años."
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "Honorarios (% s/ fondo de comercio + local)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "feesPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "IVA (% s/ honorarios)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "ivaPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "ITP (% s/ local comercial)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "itpPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "AJD (% s/ fondo de comercio + existencias)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "ajdPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "Existencias a 10 años (% s/ venta total año 10)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "inventoryPctYear10"; decimals: 1 }
            }
        }

        // ---------------- IPC histórico y margen comercial (escenario Realista)
        Card {
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                SectionTitle { text: "Escenario realista — series históricas"; Layout.fillWidth: true }
                ResetGroupButton { keys: page.historicalSeriesKeys() }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Series que alimenta el escenario de crecimiento \"Realista\" en Financiación y Proyección."
            }
            Text { text: "IPC histórico (INE)"; font.pixelSize: 12; font.bold: true; color: "#3c4a46" }
            Flickable {
                id: scroll4
                Layout.fillWidth: true
                Layout.preferredHeight: serieIpc.implicitHeight
                contentWidth: serieIpc.implicitWidth
                contentHeight: serieIpc.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll4; fallback: page }
                SerieAnualEdit { id: serieIpc; prefix: "ipcHistorical" }
            }
            Text { text: "Margen comercial simulado"; font.pixelSize: 12; font.bold: true; color: "#3c4a46"; Layout.topMargin: 8 }
            Flickable {
                id: scroll5
                Layout.fillWidth: true
                Layout.preferredHeight: serieMargen.implicitHeight
                contentWidth: serieMargen.implicitWidth
                contentHeight: serieMargen.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll5; fallback: page }
                SerieAnualEdit { id: serieMargen; prefix: "realisticMarginSeries" }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
