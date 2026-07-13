pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Comparación": compara la Proyección a 10 años de varios escenarios
// congelados desde "Datos Base", uno al lado del otro, para el año elegido.
Item {
    id: page

    readonly property int wLabel: 250
    readonly property int wCell: 170
    readonly property int hRow: 30

    readonly property bool vacio: Engine.escenariosComparacion.length === 0

    // Se recalcula cuando cambian los escenarios guardados o el año elegido:
    // referenciar Engine.escenariosComparacion aquí es necesario para que el
    // binding se re-evalúe (comparacionAnio es invocable, no una Q_PROPERTY).
    readonly property var filas: Engine.escenariosComparacion.length > 0
                                  ? Engine.comparacionAnio(anioCombo.currentIndex) : []

    // Grupo "Financiación" de la vista completa: valores fijos, no dependen del año.
    readonly property var filasFinanciacion: Engine.escenariosComparacion.length > 0
                                              ? Engine.comparacionFinanciacion() : []

    // Inserta el grupo "Financiación" justo después de la fila "VENTA TOTAL"
    // en vez de al final de la tabla. Las filas del grupo llevan "grupo: true"
    // (fondo propio, sin la franja alterna normal) y la última "groupEnd: true"
    // (línea divisoria inferior), para que se distingan del resto de la tabla.
    readonly property var filasTabla: {
        if (!switchVistaCompleta.checked)
            return page.filas
        const datosGrupo = page.filasFinanciacion.map((f, i, arr) =>
            Object.assign({}, f, { grupo: true, groupEnd: i === arr.length - 1 }))
        const grupo = [{ label: "FINANCIACIÓN", separator: true }].concat(datosGrupo)
        const idx = page.filas.findIndex(f => f.label === "VENTA TOTAL")
        if (idx < 0)
            return page.filas.concat(grupo)
        return page.filas.slice(0, idx + 1).concat(grupo, page.filas.slice(idx + 1))
    }

    function resetScroll() { flick.contentX = 0; flick.contentY = 0 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text { text: "Comparación de escenarios"; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        // ---------------- estado vacío: no hay escenarios guardados
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
                color: "#14523f"
                Image {
                    anchors.centerIn: parent
                    source: "qrc:/qt/qml/FarmaciaSim/icons/comparacion.svg"
                    sourceSize: Qt.size(40, 40)
                    width: 40
                    height: 40
                }
            }
            Text {
                text: "No hay datos para comparar"
                font.pixelSize: 15
                font.bold: true
                color: "#14523f"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: "Ve a la hoja \"Datos Base\" y pulsa \"Añadir a comparación\" para guardar el escenario actual."
                font.pixelSize: 13
                color: "#6b7a76"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 360
            }
            Item { Layout.fillHeight: true }
        }

        // ---------------- selector de año + tabla
        ColumnLayout {
            visible: !page.vacio
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            RowLayout {
                spacing: 12

                Switch {
                    id: switchVistaCompleta
                    text: "Vista completa"
                    checked: false

                    indicator: Rectangle {
                        implicitWidth: 40
                        implicitHeight: 22
                        x: switchVistaCompleta.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 11
                        color: switchVistaCompleta.checked ? "#1a7a5e" : "#c9d6cf"
                        border.color: switchVistaCompleta.checked ? "#1a7a5e" : "#a8b8b0"

                        Rectangle {
                            x: switchVistaCompleta.checked ? parent.width - width - 3 : 3
                            y: 3
                            width: 16; height: 16; radius: 8
                            color: "white"
                            Behavior on x { NumberAnimation { duration: 120 } }
                        }
                    }

                    contentItem: Text {
                        text: switchVistaCompleta.text
                        font.pixelSize: 13
                        color: "#3c4a46"
                        leftPadding: switchVistaCompleta.indicator.width + switchVistaCompleta.spacing
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.preferredWidth: 12 }

                Text { text: "Año"; font.pixelSize: 13; color: "#3c4a46" }
                ComboBox {
                    id: anioCombo
                    implicitWidth: 150
                    implicitHeight: 40
                    model: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map(n => "Año " + n)

                    background: Rectangle {
                        radius: 5
                        color: "#fffbe8"
                        border.color: anioCombo.activeFocus || anioCombo.popup.visible
                                      ? "#1a7a5e" : "#e0d6ac"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: anioCombo.displayText
                        font.pixelSize: 14
                        color: "#1e2b28"
                        leftPadding: 10
                        rightPadding: anioCombo.indicator.width + 8
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Text {
                        x: anioCombo.width - width - 10
                        y: (anioCombo.height - height) / 2
                        text: "▾"
                        font.pixelSize: 12
                        color: "#14523f"
                    }

                    popup: Popup {
                        y: anioCombo.height
                        width: anioCombo.width
                        implicitHeight: listView.contentHeight
                        padding: 1

                        contentItem: ListView {
                            id: listView
                            clip: true
                            implicitHeight: contentHeight
                            model: anioCombo.popup.visible ? anioCombo.delegateModel : null
                            currentIndex: anioCombo.highlightedIndex
                            ScrollIndicator.vertical: ScrollIndicator {}
                        }

                        background: Rectangle {
                            radius: 5
                            color: "#fffbe8"
                            border.color: "#e0d6ac"
                            border.width: 1
                        }
                    }

                    delegate: ItemDelegate {
                        id: comboDelegate
                        required property string modelData
                        required property int index
                        width: anioCombo.width
                        highlighted: anioCombo.highlightedIndex === index

                        contentItem: Text {
                            text: comboDelegate.modelData
                            font.pixelSize: 14
                            color: "#1e2b28"
                            leftPadding: 10
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: comboDelegate.highlighted ? "#eef0c9" : "#fffbe8"
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: "white"
                border.color: "#dde5e1"
                clip: true

                YearTable {
                    id: flick
                    anchors.fill: parent
                    anchors.margins: 14
                    wLabel: page.wLabel
                    wCell: page.wCell
                    hRow: page.hRow
                    headerLabels: Engine.escenariosComparacion.map(e => e.nombre)
                    closableColumns: true
                    model: page.filasTabla
                    onCloseColumn: (index) => Engine.quitarEscenarioComparacion(index)
                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }
    }
}
