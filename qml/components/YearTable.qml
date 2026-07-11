import QtQuick
import QtQuick.Controls.Basic
import FarmaciaSim

// Tabla "concepto x años" usada en Proyección, Impuestos y Simulación simple.
// El modelo es un array de filas: { label, values: [...], fmt, bold }.
Flickable {
    id: root

    property var model: []
    property int wLabel: 250
    property int wCell: 108
    property int hRow: 30
    property int fontSize: 13
    property string headerLabel: "Concepto"

    // -1 = usa la longitud de "values" de la primera fila (todas las columnas).
    property int numYears: -1
    readonly property int cols: numYears >= 0 ? numYears : (model.length > 0 ? model[0].values.length : 0)

    implicitHeight: tabla.implicitHeight
    contentWidth: wLabel + cols * wCell
    contentHeight: tabla.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    pressDelay: 150
    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

    Column {
        id: tabla

        // -------- cabecera
        Row {
            Rectangle {
                width: root.wLabel; height: root.hRow; color: "#14523f"; radius: 4
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left; anchors.leftMargin: 8
                    text: root.headerLabel; color: "white"; font.bold: true; font.pixelSize: root.fontSize
                }
            }
            Repeater {
                model: root.cols
                Rectangle {
                    required property int index
                    width: root.wCell; height: root.hRow; color: "#14523f"
                    Text {
                        anchors.centerIn: parent
                        text: "Año " + (parent.index + 1)
                        color: "white"; font.bold: true; font.pixelSize: root.fontSize
                    }
                }
            }
        }

        // -------- filas
        Repeater {
            model: root.model
            Row {
                id: fila
                required property int index
                required property var modelData

                Rectangle {
                    width: root.wLabel; height: root.hRow
                    color: fila.modelData.bold ? "#e3efe9"
                         : (fila.index % 2 ? "#f7faf8" : "white")
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left; anchors.leftMargin: 8
                        width: parent.width - 12
                        elide: Text.ElideRight
                        text: fila.modelData.label
                        font.pixelSize: root.fontSize
                        font.bold: fila.modelData.bold
                        color: fila.modelData.bold ? "#14523f" : "#3c4a46"
                    }
                }
                Repeater {
                    model: root.cols
                    Rectangle {
                        id: celda
                        required property int index
                        readonly property real valor: fila.modelData.values[index]
                        width: root.wCell; height: root.hRow
                        color: fila.modelData.bold ? "#e3efe9"
                             : (fila.index % 2 ? "#f7faf8" : "white")
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right; anchors.rightMargin: 8
                            text: Fmt.byFmt(celda.valor, fila.modelData.fmt)
                            font.pixelSize: root.fontSize
                            font.bold: fila.modelData.bold
                            color: celda.valor < 0 ? "#a33b2e"
                                 : fila.modelData.bold ? "#14523f" : "#1e2b28"
                        }
                    }
                }
            }
        }
    }
}
