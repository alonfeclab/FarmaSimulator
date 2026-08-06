import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Serie editable de 10 años (aumento de facturación histórico, margen comercial...).
// Usada en Datos base, dentro del grupo "Escenario de crecimiento".
ColumnLayout {
    id: serie
    required property string prefix
    property int wCell: 84
    Layout.fillWidth: true
    spacing: 0

    Row {
        Repeater {
            model: 10
            Item {
                id: hdrCell
                required property int index
                width: serie.wCell; height: 26
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 5
                    color: Tokens.bgBrandStrong
                    Text {
                        anchors.centerIn: parent
                        text: "Año " + (hdrCell.index + 1)
                        color: Tokens.textOnDark; font.bold: true; font.pixelSize: 11
                    }
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
                color: celda.index % 2 ? Tokens.bgTableRowAlt : Tokens.bgTableRowPrimary
                PctField {
                    anchors.centerIn: parent
                    implicitWidth: serie.wCell - 8
                    k: serie.prefix + celda.index
                }
            }
        }
    }
}
