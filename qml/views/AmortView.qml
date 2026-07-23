pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Cuadro de amortización (Banco / Cooperativa / Propiedades).
Item {
    id: page

    clip: true

    required property AmortModel loan

    // Estado "vacío": se muestran cuando aún no hay importe de préstamo.
    // Si emptyFocusKey queda vacío, esta pestaña nunca entra en modo vacío.
    property string emptyTexto: ""
    property string emptyIcono: ""
    property string emptyBotonTexto: "Ir a Financiación"
    property int emptyTabIndex: 1
    property string emptyFocusKey: ""
    readonly property bool vacio: page.emptyFocusKey !== "" && page.loan.principal <= 0

    readonly property var anchosCol: [48, 76, 112, 104, 104, 104, 112]
    readonly property int columnasResumen: width > 700 ? 4 : (width > 420 ? 2 : 1)

    function resetScroll() { tabla.contentX = 0; tabla.contentY = 0 }

    // Dato del resumen del préstamo
    component Dato: ColumnLayout {
        property string etiqueta
        property string valor
        spacing: 2
        Layout.fillWidth: true
        Text { text: etiqueta; font.pixelSize: 12; color: Tokens.textMuted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        Text { text: valor; font.pixelSize: 15; font.bold: true; color: Tokens.textHeading }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text {
            text: page.loan.title;
            font.pixelSize: 22;
            font.bold: true;
            color: Tokens.textHeading
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // ---------------- estado vacío: aún no hay importe de préstamo
        ColumnLayout {
            visible: page.vacio
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
            spacing: 16

            Item { Layout.fillHeight: true }
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 88
                height: 88
                radius: width / 2
                color: Tokens.bgBrandStrong
                Image {
                    anchors.centerIn: parent
                    source: page.emptyIcono
                    sourceSize: Qt.size(40, 40)
                    width: 40
                    height: 40
                }
            }
            Text {
                text: page.emptyTexto
                font.pixelSize: 15
                color: Tokens.textMuted
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
            }
            Button {
                text: page.emptyBotonTexto
                Layout.alignment: Qt.AlignHCenter
                onClicked: Nav.irA(page.emptyTabIndex, page.emptyFocusKey)
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }
            Item { Layout.fillHeight: true }
        }

        // ---------------- resumen del préstamo
        Rectangle {
            visible: !page.vacio
            Layout.fillWidth: true
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
            implicitHeight: resumen.implicitHeight + 32

            GridLayout {
                id: resumen
                anchors.fill: parent
                anchors.margins: 16
                columns: page.columnasResumen
                columnSpacing: 24
                rowSpacing: 10

                Dato { etiqueta: "Importe del préstamo"; valor: page.loan.info.principal }
                Dato { etiqueta: "Tasa de interés anual"; valor: page.loan.info.annualRate }
                Dato { etiqueta: "Plazo"; valor: page.loan.info.termYears }
                Dato { etiqueta: "Número de pagos"; valor: page.loan.info.numPayments }
                Dato { etiqueta: "Pago mensual"; valor: page.loan.info.monthlyPayment }
                Dato { etiqueta: "Pago anual"; valor: page.loan.info.annualPayment }
                Dato { etiqueta: "Total intereses"; valor: page.loan.info.totalInterest }
                Dato { etiqueta: "Coste total del préstamo"; valor: page.loan.info.totalCost }
            }
        }

        // ---------------- tabla
        Rectangle {
            visible: !page.vacio
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
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
                        color: Tokens.bgBrandStrong
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            text: hdrCell.display
                            color: Tokens.textOnDark; font.bold: true; font.pixelSize: 12
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
                    pressDelay: 150
                    columnWidthProvider: function(col) { return page.anchosCol[col] }
                    rowHeightProvider: function() { return 28 }
                    ScrollBar.vertical: ScrollBar {}
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    FastWheel { flick: tabla }

                    delegate: Rectangle {
                        id: celda
                        required property int row
                        required property int column
                        required property string display
                        implicitHeight: 28
                        color: row % 2 ? Tokens.bgTableRowAlt : Tokens.bgTableRowPrimary
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            text: celda.display
                            font.pixelSize: 12
                            color: celda.display.startsWith("-") ? Tokens.textNegative : Tokens.textPrimary
                        }
                    }
                }
            }
        }
    }
}
