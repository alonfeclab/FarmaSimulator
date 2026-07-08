pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Cuadro de amortización (Banco / Cooperativa / Propiedades).
Item {
    id: page

    required property AmortModel loan

    readonly property var anchosCol: [56, 84, 128, 118, 118, 118, 128]

    // Dato del resumen del préstamo
    component Dato: ColumnLayout {
        property string etiqueta
        property string valor
        spacing: 2
        Layout.fillWidth: true
        Text { text: etiqueta; font.pixelSize: 12; color: "#6b7a76" }
        Text { text: valor; font.pixelSize: 15; font.bold: true; color: "#14523f" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text { text: page.loan.titulo; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        // ---------------- resumen del préstamo
        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: "white"
            border.color: "#dde5e1"
            implicitHeight: resumen.implicitHeight + 32

            GridLayout {
                id: resumen
                anchors.fill: parent
                anchors.margins: 16
                columns: 4
                columnSpacing: 24
                rowSpacing: 10

                Dato { etiqueta: "Importe del préstamo"; valor: page.loan.info.principal }
                Dato { etiqueta: "Tasa de interés anual"; valor: page.loan.info.tasaAnual }
                Dato { etiqueta: "Plazo"; valor: page.loan.info.plazoAnios }
                Dato { etiqueta: "Número de pagos"; valor: page.loan.info.numPagos }
                Dato { etiqueta: "Pago mensual"; valor: page.loan.info.pagoMensual }
                Dato { etiqueta: "Pago anual"; valor: page.loan.info.pagoAnual }
                Dato { etiqueta: "Total intereses"; valor: page.loan.info.totalIntereses }
                Dato { etiqueta: "Coste total del préstamo"; valor: page.loan.info.costeTotal }
            }
        }

        // ---------------- tabla
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "white"
            border.color: "#dde5e1"
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 0

                HorizontalHeaderView {
                    id: cabecera
                    syncView: tabla
                    Layout.fillWidth: true
                    clip: true
                    delegate: Rectangle {
                        id: hdrCell
                        required property int index
                        required property string display
                        implicitHeight: 32
                        implicitWidth: page.anchosCol[index]
                        color: "#14523f"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            text: hdrCell.display
                            color: "white"; font.bold: true; font.pixelSize: 12
                        }
                    }
                }

                TableView {
                    id: tabla
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: page.loan
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    columnWidthProvider: function(col) { return page.anchosCol[col] }
                    rowHeightProvider: function() { return 28 }
                    ScrollBar.vertical: ScrollBar {}

                    delegate: Rectangle {
                        id: celda
                        required property int row
                        required property int column
                        required property string display
                        implicitHeight: 28
                        color: row % 2 ? "#f7faf8" : "white"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            text: celda.display
                            font.pixelSize: 12
                            color: celda.display.startsWith("-") ? "#a33b2e" : "#1e2b28"
                        }
                    }
                }
            }
        }
    }
}
