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
    FastWheel { flick: page }

    // El estilo de la caja (fondo/borde) vive en RowCard; aquí solo se
    // define el contenido propio de esta hoja.
    component CalcRow: RowCard {
        id: calcRow
        property string label
        property real value
        property string fmt: "eur"
        Text {
            text: calcRow.label; font.pixelSize: 13; font.bold: calcRow.destacada
            color: calcRow.destacada ? "#14523f" : "#3c4a46";
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.byFmt(calcRow.value, calcRow.fmt);
            font.pixelSize: 14;
            font.bold: true
            color: calcRow.destacada ? "#14523f" : "#1e2b28"
            Layout.alignment: Qt.AlignRight
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
            SectionTitle { text: "Parámetros y base amortizable" }

            GridLayout {
                columns: width >= 2 * (page.wLabel + page.wCell) + columnSpacing ? 2 : 1
                columnSpacing: 40
                rowSpacing: 8
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    CalcRow { label: "Fondo de comercio farmacia"; value: Engine.taxes.fdc }
                    CalcRow { label: "Honorarios"; value: Engine.taxes.fees }
                    CalcRow { label: "AJD (" + Fmt.pct(Engine.inputs.ajdPct) + ")"; value: Engine.taxes.ajd }
                    CalcRow { label: "Base amortizable farmacia"; value: Engine.taxes.depreciableBase; destacada: true }
                    CalcRow { label: "Coste local comercial"; value: Engine.taxes.premisesCost }
                }
                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    CalcRow { label: "% amort. local"; value: Engine.inputs.taxPremisesDeprPct; fmt: "pct1" }
                    CalcRow { label: "% amort. farmacia mín."; value: Engine.inputs.taxMinGoodwillDeprPct; fmt: "pct1" }
                    CalcRow { label: "% amort. farmacia máx."; value: Engine.inputs.taxMaxGoodwillDeprPct; fmt: "pct1" }
                    CalcRow { label: "Mínimo personal exento (IRPF)"; value: Engine.inputs.personalAllowance }
                    CalcRow { label: "Deducción mín. personal (" + Fmt.pct(Engine.inputs.irpfRate0) + ")"; value: Engine.taxes.minimumDeduction }
                }
            }
        }

        // ---------------- Pasos 2 y 3: tabla a 10 años
        Card {
            SectionTitle { text: "Amortizaciones e IRPF por tramos"; }

            ConceptTable {
                Layout.fillWidth: true
                Layout.topMargin: 8
                wLabel: page.wLabel
                wCell: page.wCell
                hRow: page.hRow
                fontSize: 12
                // La página (Flickable) ya lleva el scroll vertical.
                flickableDirection: Flickable.HorizontalFlick
                fallback: page
                model: {
                    const I = Engine.taxes
                    function cumSum(arr) {
                        let sum = 0
                        const out = []
                        for (const v of arr) { sum += v; out.push(sum) }
                        return out
                    }
                    let filas = [
                        { label: "Beneficio farmacia", values: I.profit, fmt: "eur", bold: false },
                        { label: "Amortización local", values: I.premisesDepreciation, fmt: "eur", bold: false },
                        { label: "Suma amortización local", values: cumSum(I.premisesDepreciation), fmt: "eur", bold: false },
                        { label: "% amort. farmacia ajustado", values: I.adjustedPct, fmt: "pct2", bold: false },
                        { label: "Amortización FdC farmacia", values: I.fdcDepreciation, fmt: "eur", bold: false },
                        { label: "Suma amortización farmacia", values: cumSum(I.fdcDepreciation), fmt: "eur", bold: false },
                        { label: "Base imponible", values: I.taxableBase, fmt: "eur", bold: true }
                    ]
                    for (const t of I.brackets)
                        filas.push({ label: t.label, values: t.values, fmt: "eur", bold: false })
                    filas.push({ label: "Cuota escala (suma de tramos)", values: I.bracketQuota, fmt: "eur", bold: true })
                    filas.push({ label: "Pago impuestos", values: I.payment, fmt: "eur", bold: true })
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
                    + "\"Pago impuestos\" de la Proyección a 10 años."
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
