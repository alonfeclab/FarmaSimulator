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

    component Card: Rectangle {
        default property alias content: box.data
        Layout.fillWidth: true
        radius: 12
        color: "white"
        border.color: "#dde5e1"
        implicitHeight: box.implicitHeight + 40
        ColumnLayout {
            id: box
            anchors.fill: parent
            anchors.margins: 20
            spacing: 8
        }
    }

    component CalcRow: RowLayout {
        property string label
        property real value
        property bool destacada: false
        Layout.fillWidth: true
        Text {
            text: label; font.pixelSize: 13; font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"; Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(value); font.pixelSize: 14; font.bold: true
            color: destacada ? "#14523f" : "#1e2b28"
        }
    }

    component EditRow: RowLayout {
        property string label
        property string k
        Layout.fillWidth: true
        Text { text: label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        MoneyField { k: parent.k }
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

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 820)
        spacing: 14

        Text { text: "Estudio de financiación"; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        // ---------------- Crecimientos previstos
        Card {
            SectionTitle { text: "CRECIMIENTOS PREVISTOS" }
            Flickable {
                Layout.fillWidth: true
                implicitHeight: crecGrid.implicitHeight
                contentWidth: crecGrid.implicitWidth
                contentHeight: crecGrid.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                pressDelay: 150
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOn }

                GridLayout {
                    id: crecGrid
                    columns: 4
                    columnSpacing: 12
                    rowSpacing: 6

                    Item { Layout.preferredWidth: 150 }
                    Text { text: "Año 1"; font.bold: true; font.pixelSize: 13; color: "#14523f"; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: "Año 2"; font.bold: true; font.pixelSize: 13; color: "#14523f"; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: "Año 3+"; font.bold: true; font.pixelSize: 13; color: "#14523f"; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }

                    Text { text: "IPC"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "ipc0" } PctField { k: "ipc1" } PctField { k: "ipc2" }

                    Text { text: "Venta seguro"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "crecSeguro0" } PctField { k: "crecSeguro1" } PctField { k: "crecSeguro2" }

                    Text { text: "Venta libre"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "crecLibre0" } PctField { k: "crecLibre1" } PctField { k: "crecLibre2" }

                    Text { text: "M. Comercial"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "margen0" } PctField { k: "margen1" } PctField { k: "margen2" }
                }
            }
        }

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
            CalcRow { label: "    Impuesto IBI (8% Local)"; value: Engine.financiacion.impuestoIBI }
            CalcRow { label: "    AJD (1,5% FdC + Existencias)"; value: Engine.financiacion.ajd }
            EditRow { label: "Gastos varios operación"; k: "gastosVarios" }
            CalcRow { label: "Impuestos (IBI + AJD)"; value: Engine.financiacion.impuestos }
            CalcRow { label: "TOTAL INVERSIÓN"; value: Engine.financiacion.totalInversion; destacada: true }
        }

        // ---------------- Tipo y plazo
        Card {
            SectionTitle { text: "TIPO Y PLAZO" }
            GridLayout {
                columns: page.angosto ? 1 : 2
                columnSpacing: 40
                rowSpacing: 8
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    PctRow  { label: "Tipo de interés banco"; k: "tipoBanco" }
                    PctRow  { label: "Tipo de interés cooperativa"; k: "tipoCoop" }
                    PctRow  { label: "Tipo de interés familiar"; k: "tipoFamiliar" }
                    PctRow  { label: "Tipo de interés propiedades"; k: "tipoPropiedades" }
                    PctRow  { label: "% Financiación farmacia"; k: "pctFinFarmacia" }
                }
                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    PlazoRow { label: "Plazo préstamo banco"; k: "plazoBanco" }
                    PlazoRow { label: "Plazo préstamo cooperativa"; k: "plazoCoop" }
                    PlazoRow { label: "Plazo préstamo familiar"; k: "plazoFamiliar" }
                    PlazoRow { label: "Plazo préstamo propiedades"; k: "plazoPropiedades" }
                    PctRow  { label: "% Financiación local"; k: "pctFinLocal" }
                    PctRow  { label: "% Financiación propiedades"; k: "pctFinPropiedades" }
                }
            }
        }

        // ---------------- Financiación
        Card {
            SectionTitle { text: "FINANCIACIÓN" }
            EditRow { label: "Liquidez aportada"; k: "liquidezAportada" }
            EditRow { label: "Aportación familiar"; k: "aportacionFamiliar" }
            CalcRow { label: "Financiación bancaria farmacia"; value: Engine.financiacion.finBancariaFarmacia }
            CalcRow { label: "Financiación bancaria local"; value: Engine.financiacion.finBancariaLocal }
            EditRow { label: "Financiación propiedades"; k: "finPropiedades" }
            EditRow { label: "Exceso/defecto de aportación"; k: "excesoAportacion" }
            EditRow { label: "Pedido inicial (cooperativa)"; k: "pedidoInicial" }
            CalcRow { label: "TOTAL FINANCIACIÓN"; value: Engine.financiacion.totalFinanciacion; destacada: true }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
