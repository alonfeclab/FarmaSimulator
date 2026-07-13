pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Impuestos" (v2): cálculo del pago de IRPF por tramos.
Flickable {
    id: page

    readonly property int wLabel: 230
    readonly property int wCell: 100
    readonly property int hRow: 28

    contentWidth: Math.max(width, col.implicitWidth)
    contentHeight: col.implicitHeight + 48
    clip: false
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }

    component CalcRow: RowLayout {
        property string label
        property real value
        property string fmt: "eur"
        property bool destacada: false
        Layout.fillWidth: true
        Text {
            text: label; font.pixelSize: 13; font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46";
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.byFmt(value, fmt);
            font.pixelSize: 14;
            font.bold: true
            color: destacada ? "#14523f" : "#1e2b28"
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
            text: "Impuestos"
            font.pixelSize: 22;
            font.bold: true;
            color: "#14523f"
        }

        // ---------------- Paso 1: parámetros y base amortizable
        Card {
            SectionTitle { text: "PARÁMETROS Y BASE AMORTIZABLE" }

            GridLayout {
                columns: width >= 2 * (page.wLabel + page.wCell) + columnSpacing ? 2 : 1
                columnSpacing: 40
                rowSpacing: 8
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    CalcRow { label: "Fondo de comercio farmacia"; value: Engine.impuestos.fdc }
                    CalcRow { label: "Honorarios"; value: Engine.impuestos.honorarios }
                    CalcRow { label: "AJD (1,5% s/ F. Comercio + Existencias)"; value: Engine.impuestos.ajd }
                    CalcRow { label: "BASE AMORTIZABLE FARMACIA"; value: Engine.impuestos.baseAmortizable; destacada: true }
                    CalcRow { label: "Coste local comercial"; value: Engine.impuestos.costeLocal }
                }
                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    CalcRow { label: "% amortización local (fijo)"; value: Engine.inputs.impAmortLocalPct; fmt: "pct1" }
                    CalcRow { label: "% amortización farmacia (mínimo)"; value: Engine.inputs.impAmortMinPct; fmt: "pct1" }
                    CalcRow { label: "% amortización farmacia (máximo)"; value: Engine.inputs.impAmortMaxPct; fmt: "pct1" }
                    CalcRow { label: "Mínimo personal exento (IRPF)"; value: Engine.inputs.minimoPersonal }
                    CalcRow { label: "Deducción mínimo personal (× 19%)"; value: Engine.impuestos.deduccionMinimo }
                }
            }
        }

        // ---------------- Pasos 2 y 3: tabla a 10 años
        Card {
            SectionTitle { text: "AMORTIZACIONES, BASE IMPONIBLE E IRPF POR TRAMOS (ESCALA 2026)"; }

            ConceptTable {
                Layout.fillWidth: true
                Layout.topMargin: 8
                wLabel: page.wLabel
                wCell: page.wCell
                hRow: page.hRow
                fontSize: 12
                model: {
                    const I = Engine.impuestos
                    function cumSum(arr) {
                        let sum = 0
                        const out = []
                        for (const v of arr) { sum += v; out.push(sum) }
                        return out
                    }
                    let filas = [
                        { label: "Beneficio farmacia", values: I.beneficio, fmt: "eur", bold: false },
                        { label: "Amortización local", values: I.amortLocal, fmt: "eur", bold: false },
                        { label: "Suma amortización local", values: cumSum(I.amortLocal), fmt: "eur", bold: false },
                        { label: "% amort. farmacia ajustado", values: I.pctAjustado, fmt: "pct2", bold: false },
                        { label: "Amortización F.C. farmacia", values: I.amortFdC, fmt: "eur", bold: false },
                        { label: "Suma amortización farmacia", values: cumSum(I.amortFdC), fmt: "eur", bold: false },
                        { label: "BASE IMPONIBLE", values: I.baseImponible, fmt: "eur", bold: true }
                    ]
                    for (const t of I.tramos)
                        filas.push({ label: t.label, values: t.values, fmt: "eur", bold: false })
                    filas.push({ label: "Cuota escala (suma de tramos)", values: I.cuotaEscala, fmt: "eur", bold: true })
                    filas.push({ label: "PAGO IMPUESTOS", values: I.pago, fmt: "eur", bold: true })
                    return filas
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "El % de amortización de la farmacia se ajusta cada año (entre el mínimo y el máximo) para "
                    + "dejar la base imponible en 0 mientras sea posible. El pago alimenta la fila "
                    + "\"Pago Impuestos\" de la Proyección a 10 años."
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
