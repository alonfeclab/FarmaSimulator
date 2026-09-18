pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Venta": patrimonio acumulado año a año si se vende la farmacia al
// cierre de cada año proyectado, en las mismas condiciones en que se compró
// (mismo coeficiente sobre la facturación de ese año y mismo precio del bajo).
// Responde a "¿cuándo puedo vender sin perder un euro?": la aportación
// líquida inicial y la hipoteca del banco se recuperan con el valor de venta
// menos la deuda viva y los impuestos de la plusvalía, y el primer año con
// patrimonio en positivo es el primero en que la operación sale a cuenta.
// No tiene campos editables propios: las condiciones se leen de Financiación
// y la escala del ahorro de Configuración, para que "mismas condiciones" sea
// literal y no haya dos coeficientes que mantener sincronizados.
Flickable {
    id: page

    readonly property int wLabel: 280
    readonly property int wCell: 112
    readonly property int hRow: 30
    readonly property bool angosto: width < 640

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Condición de la venta que se lee de otra hoja (no editable aquí).
    component CalcRow: RowCard {
        id: calcRow
        property string label
        property string value
        Text {
            text: calcRow.label; font.pixelSize: 13; color: Tokens.textSecondary
            Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
        Text {
            text: calcRow.value; font.pixelSize: 14; font.bold: true
            color: Tokens.textPrimary
            Layout.alignment: Qt.AlignRight
        }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: page.width - 48
        spacing: 14

        Text { text: "Venta de la farmacia"; font.pixelSize: 22; font.bold: true; color: Tokens.textHeading }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: Tokens.textMuted
            text: "Qué patrimonio se acumula año a año si se vende la farmacia al cierre de cada año, "
                + "en las mismas condiciones en que se compró: mismo coeficiente sobre la facturación "
                + "de ese año y mismo precio del local. Del valor de venta se descuenta la deuda que "
                + "aún queda viva (hipoteca del banco, local y propiedades, cooperativa y familiar) y "
                + "los impuestos de la plusvalía, y al resultado se le resta la aportación líquida "
                + "inicial. El primer año con patrimonio en positivo es el primero en que se puede "
                + "vender sin perder un euro."
        }

        // ---------------- KPIs
        GridLayout {
            Layout.fillWidth: true
            columns: page.angosto ? 2 : 4
            rowSpacing: 12
            columnSpacing: 12

            StatTile { label: "Aportación líquida inicial"; value: Engine.sale.initialCash; fmt: "eur" }
            StatTile { label: "Hipoteca pedida al banco"; value: Engine.sale.bankFinancing; fmt: "eur" }
            StatTile {
                label: "Años hasta poder vender sin perder dinero"
                value: Engine.sale.breakEvenYear
                fmt: "years"
                negativoEnRojo: false
            }
            StatTile {
                label: "Años, sin contar el salario ya cobrado"
                value: Engine.sale.breakEvenYearExSalary
                fmt: "years"
                negativoEnRojo: false
            }
        }

        // ---------------- Condiciones de la venta
        CollapsibleCard {
            title: "Condiciones de la venta"

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "El coeficiente, el precio del local y el porcentaje de existencias son los "
                    + "mismos de la compra: se editan en Financiación y Configuración, no aquí."
            }

            CalcRow {
                label: "Coeficiente s/venta total"
                value: Fmt.num(Engine.inputs.goodwillMultiple, 2)
            }
            CalcRow {
                label: "Local comercial"
                value: Fmt.eur(Engine.inputs.premisesPrice)
            }
            CalcRow {
                label: "Existencias (% s/venta total del año)"
                value: Fmt.pct(Engine.inputs.inventoryPctYear10)
            }
            CalcRow {
                label: "Impuestos de la venta"
                value: "Escala del ahorro"
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                font.italic: true
                color: Tokens.textFaint
                text: "IRPF por tramos de la base del ahorro (editable en Configuración) sobre la "
                    + "plusvalía = valor de venta bruto − valor contable pendiente (fondo de comercio "
                    + "y local ya amortizados en la hoja Impuestos, más las existencias a coste)."
            }
        }

        // ---------------- Tabla concepto x año
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: tabla.implicitHeight + 28
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
            clip: true

            ConceptTable {
                id: tabla
                // Permite localizar la tabla en los tests QML (tst_VentaView.qml).
                objectName: "tabla"
                anchors.fill: parent
                anchors.margins: 14
                wLabel: page.wLabel
                wCell: page.wCell
                hRow: page.hRow
                model: Engine.sale.rows
                // La página (Flickable) ya lleva el scroll vertical: esta tabla
                // solo necesita desplazarse en horizontal.
                flickableDirection: Flickable.HorizontalFlick
                fallback: page
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
