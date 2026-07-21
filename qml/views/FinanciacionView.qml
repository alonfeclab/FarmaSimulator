pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Financiación": crecimientos, inversión, tipos/plazos y financiación.
Flickable {
    id: page

    readonly property bool angosto: width < 640

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // El estilo de la caja (fondo/borde) vive en RowCard; aquí solo se
    // define el contenido propio de esta hoja.
    component CalcRow: RowCard {
        id: calcRow
        property string label
        property real value
        property bool indent: false
        Text {
            text: calcRow.label; font.pixelSize: 13; font.bold: calcRow.destacada
            color: calcRow.destacada ? "#14523f" : "#3c4a46"; Layout.fillWidth: true
            Layout.leftMargin: calcRow.indent ? 16 : 0
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(calcRow.value); font.pixelSize: 14; font.bold: true
            color: calcRow.destacada ? "#14523f" : "#1e2b28"
        }
    }

    component EditRow: ColumnLayout {
        id: editRow
        property string label
        property string k
        property bool invalid: false
        property bool showMinimo: false
        property real minimo: 0
        property string warningText: ""
        Layout.fillWidth: true
        spacing: 2
        RowCard {
            Text { text: editRow.label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            MoneyField { k: editRow.k; invalid: editRow.invalid }
        }
        Text {
            text: "Mínimo recomendado: " + Fmt.eur(editRow.minimo)
            visible: editRow.showMinimo
            Layout.alignment: Qt.AlignRight
            font.pixelSize: 11
            color: editRow.invalid ? "#c0392b" : "#3c4a46"
        }
        Text {
            text: editRow.warningText
            visible: editRow.invalid && editRow.warningText.length > 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
            font.pixelSize: 11
            color: "#c0392b"
        }
        Text {
            text: "(máx(0, (Total inversión " + Fmt.eur(Engine.financing.totalInvestment)
                  + " − Local comercial " + Fmt.eur(Engine.inputs.premisesPrice)
                  + ") × (1 − % Financiación farmacia " + Fmt.pct(Engine.inputs.pharmacyFinancingPct) + ")"
                  + " − Financiación propiedades " + Fmt.eur(Engine.inputs.propertiesFinancing * Engine.inputs.propertiesFinancingPct)
                  + " − Pedido inicial " + Fmt.eur(Engine.inputs.initialOrder)
                  + "))"
                  + " + Local comercial " + Fmt.eur(Engine.inputs.premisesPrice)
                  + " × (1 − % Financiación local " + Fmt.pct(Engine.inputs.premisesFinancingPct) + ")"
                  + " = Mínimo recomendado " + Fmt.eur(editRow.minimo)
            visible: editRow.showMinimo
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
            font.pixelSize: 10
            font.italic: true
            color: "#7a8985"
        }
    }

    component PctRow: RowCard {
        id: pctRow
        property string label
        property string k
        property int decimals: 1
        Text { text: pctRow.label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        PctField { k: pctRow.k; decimals: pctRow.decimals }
    }

    component PlazoRow: RowCard {
        id: plazoRow
        property string label
        property string k
        Text { text: plazoRow.label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        NumField { k: plazoRow.k; suffix: " años" }
    }

    component MesesRow: RowCard {
        id: mesesRow
        property string label
        property string k
        Text { text: mesesRow.label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        NumField { k: mesesRow.k; suffix: " meses" }
    }

    // Agrupa visualmente todos los valores relacionados con una misma fuente de financiación
    // (tipo de interés, plazo, % financiado e importe).
    component SourceGroup: Rectangle {
        id: srcGroup
        property string title
        default property alias content: box.data
        Layout.fillWidth: true
        radius: 8
        color: "#eef5f1"
        border.color: "#d5e6dc"
        implicitHeight: colWrap.implicitHeight + 16

        ColumnLayout {
            id: colWrap
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 8
            spacing: 6

            Text { text: srcGroup.title; font.bold: true; font.pixelSize: 12; color: "#14523f" }
            ColumnLayout {
                id: box
                Layout.fillWidth: true
                spacing: 6
            }
        }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 820)
        spacing: 14

        Text { text: "Estudio de financiación"; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        // ---------------- Inversión operación
        Card {
            SectionTitle { text: "Inversión operación" }
            RowCard {
                Text { text: "Coeficiente s/venta total"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                NumField { k: "goodwillMultiple"; decimals: 2 }
            }
            CalcRow { label: "Fondo de comercio"; value: Engine.financing.goodwill }
            EditRow { label: "Existencias"; k: "inventory" }
            CalcRow { label: "Honorarios (" + Fmt.pct(Engine.inputs.feesPct) + ")"; value: Engine.financing.fees }
            CalcRow { label: "IVA (" + Fmt.pct(Engine.inputs.ivaPct) + ")"; value: Engine.financing.iva }
            CalcRow { label: "Total impuestos"; value: Engine.financing.taxes; destacada: true }
            CalcRow { label: "ITP (" + Fmt.pct(Engine.inputs.itpPct) + ")"; value: Engine.financing.itpTax; indent: true }
            CalcRow { label: "AJD (" + Fmt.pct(Engine.inputs.ajdPct) + ")"; value: Engine.financing.ajd; indent: true}
            EditRow { label: "Notario"; k: "notaryFees" }
            EditRow { label: "Registro"; k: "registryFees" }
            EditRow { label: "Gastos varios operación"; k: "miscExpenses" }
            PctRow  { label: "% Apertura hipoteca"; k: "mortgageOpeningPct" }
            CalcRow { label: "Gastos de apertura hipoteca"; value: Engine.financing.mortgageOpeningCost }
            CalcRow {
                label: "Otros gastos"
                value: Engine.financing.fees + Engine.financing.iva + Engine.financing.taxes
                       + Engine.inputs.notaryFees + Engine.inputs.registryFees + Engine.inputs.miscExpenses
                       + Engine.financing.mortgageOpeningCost
                destacada: true
            }
            CalcRow { label: "Total inversión"; value: Engine.financing.totalInvestment; destacada: true }
        }

        // ---------------- Financiación (tipo, plazo e importes agrupados por fuente)
        Card {
            SectionTitle { text: "Financiación" }

            EditRow {
                label: "Liquidez aportada"
                k: "contributedCash"
                invalid: Engine.financing.cashBelowMinimum
                showMinimo: true
                minimo: Engine.financing.minimumCash
            }

            SourceGroup {
                title: "Banco"
                PctRow   { label: "Tipo interés"; k: "bankRate"; decimals: 3 }
                PlazoRow { label: "Plazo"; k: "bankTermYears" }
                PctRow   { label: "% Financiación farmacia"; k: "pharmacyFinancingPct" }
                CalcRow  { label: "Financiación farmacia"; value: Engine.financing.pharmacyBankFinancing }
            }

            SourceGroup {
                title: "Cooperativa"
                PctRow   { label: "Tipo interés"; k: "coopRate"; decimals: 3 }
                PlazoRow { label: "Plazo"; k: "coopTermYears" }
                EditRow  { label: "Pedido inicial"; k: "initialOrder" }
            }

            SourceGroup {
                title: "Familiar"
                PctRow   { label: "Tipo interés"; k: "familyRate"; decimals: 3 }
                PlazoRow { label: "Plazo"; k: "familyTermYears" }
                MesesRow { label: "Periodo de carencia"; k: "familyGraceMonths" }
                EditRow  { label: "Aportación familiar"; k: "familyContribution" }
            }

            SourceGroup {
                title: "Propiedades"
                PctRow   { label: "Tipo interés"; k: "propertiesRate"; decimals: 3 }
                PlazoRow { label: "Plazo"; k: "propertiesTermYears" }
                PctRow   { label: "% Financiación"; k: "propertiesFinancingPct" }
                EditRow  { label: "Valor propiedades"; k: "propertiesFinancing" }
                PctRow   { label: "% Financiación local"; k: "premisesFinancingPct" }
                EditRow {
                    label: "Local comercial"
                    k: "premisesPrice"
                    invalid: Engine.inputs.premisesRent > 0 && Engine.inputs.premisesPrice !== 0
                    warningText: "Hay alquiler de local en Datos Base: el precio de compra del local debería ser 0."
                }
                CalcRow  { label: "Financiación local"; value: Engine.financing.premisesBankFinancing }
                CalcRow  {
                    label: "Total"
                    value: Engine.inputs.propertiesFinancing * Engine.inputs.propertiesFinancingPct
                           + Engine.financing.premisesBankFinancing
                    destacada: true
                }
            }

            CalcRow { label: "Total financiación"; value: Engine.financing.totalFinancing; destacada: true }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
