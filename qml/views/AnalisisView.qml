pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Análisis Inversión": patrimonio, liquidez y amortización del FdC.
Flickable {
    id: page

    readonly property int wCell: 115
    readonly property int wLabelEscenarios: 200
    readonly property bool angosto: width < 640

    contentWidth: Math.max(width, col.implicitWidth)
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }

    // Fila de 3 escenarios / 3 columnas, solo lectura
    component Row3: RowLayout {
        property string label
        property var vals
        property string fmt: "eur"
        property bool destacada: false
        Layout.fillWidth: true
        Text {
            text: label; font.pixelSize: 13; font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"; Layout.preferredWidth: page.wLabelEscenarios
            elide: Text.ElideRight
        }
        Repeater {
            model: vals
            Text {
                required property var modelData
                Layout.preferredWidth: page.wCell
                horizontalAlignment: Text.AlignRight
                text: Fmt.byFmt(modelData, fmt)
                font.pixelSize: 13; font.bold: destacada
                color: modelData < 0 ? "#a33b2e" : destacada ? "#14523f" : "#1e2b28"
            }
        }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: Math.min(page.width - 88, 860)
        spacing: 14

        Text {
            text: "Patrimonio, liquidez y amortización del fondo de comercio"
            font.pixelSize: 22;
            font.bold: true;
            color: "#14523f"
            wrapMode: Text.WordWrap
        }

        // ---------------- Valor patrimonio año 10
        Card {
            SectionTitle { text: "VALOR PATRIMONIO AÑO 10" }

            Flickable {
                id: flickPatrimonio
                Layout.fillWidth: true
                implicitHeight: patrimonioCol.height
                contentWidth: patrimonioCol.width
                contentHeight: patrimonioCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                pressDelay: 150
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: patrimonioCol
                    width: Math.max(implicitWidth, flickPatrimonio.width)
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.preferredWidth: page.wLabelEscenarios }
                        Repeater {
                            model: ["PESIMISTA", "NEUTRAL", "OPTIMISTA"]
                            Text {
                                required property string modelData
                                Layout.preferredWidth: page.wCell
                                horizontalAlignment: Text.AlignRight
                                text: modelData; font.bold: true; font.pixelSize: 13; color: "#14523f"
                            }
                        }
                    }

                    Row3 { label: "Inversión inicial"; vals: Engine.analisis.inversionInicial }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Factor venta"; font.pixelSize: 13; color: "#3c4a46"; Layout.preferredWidth: page.wLabelEscenarios }
                        NumField { k: "factorVenta0"; decimals: 1; Layout.preferredWidth: page.wCell }
                        NumField { k: "factorVenta1"; decimals: 1; Layout.preferredWidth: page.wCell }
                        NumField { k: "factorVenta2"; decimals: 1; Layout.preferredWidth: page.wCell }
                    }
                    Row3 { label: "Valor venta F. de C. año 10"; vals: Engine.analisis.valorVentaFdC }
                    Row3 { label: "Valor venta local (incr. IPC)"; vals: Engine.analisis.valorVentaLocal }
                    Row3 { label: "Exist. (10% facturación año 10)"; vals: Engine.analisis.existencias10 }
                    Row3 { label: "Fondo de comercio pendiente"; vals: Engine.analisis.fdcPendiente }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Impuestos venta"; font.pixelSize: 13; color: "#3c4a46"; Layout.preferredWidth: page.wLabelEscenarios }
                        MoneyField { k: "impuestosVenta0"; Layout.preferredWidth: page.wCell }
                        MoneyField { k: "impuestosVenta1"; Layout.preferredWidth: page.wCell }
                        MoneyField { k: "impuestosVenta2"; Layout.preferredWidth: page.wCell }
                    }
                    Row3 { label: "Deuda pendiente año 10"; vals: Engine.analisis.deuda }
                    Row3 { label: "PATRIMONIO BRUTO año 10"; vals: Engine.analisis.patrimonioBruto; destacada: true }
                    Row3 { label: "PATRIMONIO NETO año 10"; vals: Engine.analisis.patrimonioNeto; destacada: true }
                    Row3 { label: "CAGR PATRIMONIO"; vals: Engine.analisis.cagr; fmt: "pct2"; destacada: true }
                    Row3 { label: "TIR INVERSIÓN TOTAL"; vals: Engine.analisis.tir; fmt: "pct2"; destacada: true }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 6
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "CAGR: revalorización del capital al vender la farmacia a los 10 años, sin contar el salario. "
                    + "TIR: retorno total, incluye el salario neto cobrado cada año más el valor de venta."
            }
        }

        // ---------------- Liquidez mensual
        Card {
            SectionTitle { text: "LIQUIDEZ MENSUAL" }
            Flickable {
                id: flickLiquidez
                Layout.fillWidth: true
                implicitHeight: liquidezCol.height
                contentWidth: liquidezCol.width
                contentHeight: liquidezCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                pressDelay: 150
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: liquidezCol
                    width: Math.max(implicitWidth, flickLiquidez.width)
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.preferredWidth: page.wLabelEscenarios }
                        Repeater {
                            model: ["Año 1", "Año 5", "Año 10"]
                            Text {
                                required property string modelData
                                Layout.preferredWidth: page.wCell
                                horizontalAlignment: Text.AlignRight
                                text: modelData; font.bold: true; font.pixelSize: 13; color: "#14523f"
                            }
                        }
                    }
                    Row3 { label: "Liquidez mensual"; vals: Engine.analisis.liqMensual }
                    Row3 { label: "Devolución capital"; vals: Engine.analisis.devCapitalMensual }
                    Row3 { label: "Intereses"; vals: Engine.analisis.interesesMensual }
                    Row3 { label: "NETO TITULAR"; vals: Engine.analisis.netoTitular; destacada: true }
                }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
