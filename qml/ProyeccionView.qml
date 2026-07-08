pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Proyección 10 Años": tabla de 19 conceptos x 10 años.
Item {
    id: page

    readonly property int wLabel: 250
    readonly property int wCell: 108
    readonly property int hRow: 30

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text { text: "Proyección a 10 años"; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "white"
            border.color: "#dde5e1"
            clip: true

            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: 14
                contentWidth: page.wLabel + 10 * page.wCell
                contentHeight: tabla.implicitHeight
                clip: true
                ScrollBar.horizontal: ScrollBar {}
                ScrollBar.vertical: ScrollBar {}

                Column {
                    id: tabla

                    // -------- cabecera
                    Row {
                        Rectangle {
                            width: page.wLabel; height: page.hRow; color: "#14523f"
                            radius: 4
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 8
                                text: "Concepto"; color: "white"; font.bold: true; font.pixelSize: 13
                            }
                        }
                        Repeater {
                            model: 10
                            Rectangle {
                                required property int index
                                width: page.wCell; height: page.hRow; color: "#14523f"
                                Text {
                                    anchors.centerIn: parent
                                    text: "Año " + (parent.index + 1)
                                    color: "white"; font.bold: true; font.pixelSize: 13
                                }
                            }
                        }
                    }

                    // -------- filas
                    Repeater {
                        model: Engine.proyeccion
                        Row {
                            id: fila
                            required property int index
                            required property var modelData

                            Rectangle {
                                width: page.wLabel; height: page.hRow
                                color: fila.modelData.bold ? "#e3efe9"
                                     : (fila.index % 2 ? "#f7faf8" : "white")
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left; anchors.leftMargin: 8
                                    text: fila.modelData.label
                                    font.pixelSize: 13
                                    font.bold: fila.modelData.bold
                                    color: fila.modelData.bold ? "#14523f" : "#3c4a46"
                                }
                            }
                            Repeater {
                                model: fila.modelData.values
                                Rectangle {
                                    id: celda
                                    required property var modelData
                                    width: page.wCell; height: page.hRow
                                    color: fila.modelData.bold ? "#e3efe9"
                                         : (fila.index % 2 ? "#f7faf8" : "white")
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.right: parent.right; anchors.rightMargin: 8
                                        text: Fmt.byFmt(celda.modelData, fila.modelData.fmt)
                                        font.pixelSize: 13
                                        font.bold: fila.modelData.bold
                                        color: celda.modelData < 0 ? "#a33b2e"
                                             : fila.modelData.bold ? "#14523f" : "#1e2b28"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Text {
            text: "Los crecimientos anuales se editan en la hoja Financiación; el año 3 se aplica hasta el año 10."
            font.pixelSize: 12
            color: "#6b7a76"
        }
    }
}
