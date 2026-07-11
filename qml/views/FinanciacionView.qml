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

    component CalcRow: RowLayout {
        property string label
        property real value
        property bool destacada: false
        property bool indent: false
        Layout.fillWidth: true
        Text {
            text: label; font.pixelSize: 13; font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"; Layout.fillWidth: true
            Layout.leftMargin: indent ? 16 : 0
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(value); font.pixelSize: 14; font.bold: true
            color: destacada ? "#14523f" : "#1e2b28"
        }
    }

    component EditRow: ColumnLayout {
        id: editRow
        property string label
        property string k
        property bool invalid: false
        property bool showMinimo: false
        property real minimo: 0
        Layout.fillWidth: true
        spacing: 2
        RowLayout {
            Layout.fillWidth: true
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
    }

    component PctRow: RowLayout {
        property string label
        property string k
        Layout.fillWidth: true
        Text { text: label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        PctField { k: parent.k }
    }

    component PlazoRow: RowLayout {
        property string label
        property string k
        Layout.fillWidth: true
        Text { text: label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        NumField { k: parent.k; suffix: " años" }
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
            SectionTitle { text: "INVERSIÓN OPERACIÓN" }
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Coeficiente (sobre venta total)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                NumField { k: "coeficiente"; decimals: 1 }
            }
            CalcRow { label: "Fondo de Comercio"; value: Engine.financiacion.fondoComercio }
            EditRow { label: "Local Comercial"; k: "localComercial" }
            EditRow { label: "Existencias"; k: "existencias" }
            CalcRow { label: "Honorarios (5% FdC + Local)"; value: Engine.financiacion.honorarios }
            CalcRow { label: "IVA (21% honorarios)"; value: Engine.financiacion.iva }
            CalcRow { label: "TOTAL IMPUESTOS (ITP + AJD)"; value: Engine.financiacion.impuestos; destacada: true }
            CalcRow { label: "Impuesto ITP (8% Local)"; value: Engine.financiacion.impuestoITP; indent: true }
            CalcRow { label: "AJD (1,5% FdC + Existencias)"; value: Engine.financiacion.ajd; indent: true}
            EditRow { label: "Gastos varios operación"; k: "gastosVarios" }
            CalcRow { label: "TOTAL INVERSIÓN"; value: Engine.financiacion.totalInversion; destacada: true }
        }

        // ---------------- Financiación (tipo, plazo e importes agrupados por fuente)
        Card {
            SectionTitle { text: "FINANCIACIÓN" }

            EditRow {
                label: "Liquidez aportada"
                k: "liquidezAportada"
                invalid: Engine.financiacion.liquidezInvalida
                showMinimo: true
                minimo: Engine.financiacion.minimoLiquidez
            }

            SourceGroup {
                title: "BANCO"
                PctRow   { label: "Tipo de interés"; k: "tipoBanco" }
                PlazoRow { label: "Plazo del préstamo"; k: "plazoBanco" }
                PctRow   { label: "% Financiación farmacia"; k: "pctFinFarmacia" }
                CalcRow  { label: "Financiación bancaria farmacia"; value: Engine.financiacion.finBancariaFarmacia }
                PctRow   { label: "% Financiación local"; k: "pctFinLocal" }
                CalcRow  { label: "Financiación bancaria local"; value: Engine.financiacion.finBancariaLocal }
            }

            SourceGroup {
                title: "COOPERATIVA"
                PctRow   { label: "Tipo de interés"; k: "tipoCoop" }
                PlazoRow { label: "Plazo del préstamo"; k: "plazoCoop" }
                EditRow  { label: "Pedido inicial"; k: "pedidoInicial" }
            }

            SourceGroup {
                title: "FAMILIAR"
                PctRow   { label: "Tipo de interés"; k: "tipoFamiliar" }
                PlazoRow { label: "Plazo del préstamo"; k: "plazoFamiliar" }
                EditRow  { label: "Aportación familiar"; k: "aportacionFamiliar" }
            }

            SourceGroup {
                title: "PROPIEDADES"
                PctRow   { label: "Tipo de interés"; k: "tipoPropiedades" }
                PlazoRow { label: "Plazo del préstamo"; k: "plazoPropiedades" }
                PctRow   { label: "% Financiación"; k: "pctFinPropiedades" }
                EditRow  { label: "Valor propiedades"; k: "finPropiedades" }
            }

            CalcRow { label: "TOTAL FINANCIACIÓN"; value: Engine.financiacion.totalFinanciacion; destacada: true }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
