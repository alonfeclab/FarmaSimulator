pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Datos Base": PyG estimada para el estudio.
Flickable {
    id: page

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Fila calculada (solo lectura). El estilo de la caja (fondo/borde) vive
    // en RowCard; aquí solo se define el contenido de esta hoja.
    component CalcRow: RowCard {
        property string label
        property real value
        Text {
            text: label
            font.pixelSize: 13
            font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(value)
            font.pixelSize: 14
            font.bold: true
            color: destacada ? "#14523f" : "#1e2b28"
            Layout.alignment: Qt.AlignRight
        }
    }

    // Fila editable. Mismo RowCard que CalcRow, para que ambas se lean igual.
    component EditRow: RowCard {
        id: editRow
        property string label
        property string k
        property real multiplier: 1
        Text { text: editRow.label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        MoneyField { k: editRow.k; multiplier: editRow.multiplier; Layout.alignment: Qt.AlignRight }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 24
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 760)
        spacing: 14

        Text { text: "Datos base del estudio"; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        // ---------------- Escenario de crecimiento
        Card {
            SectionTitle { text: "Escenario de crecimiento" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "Tipo"
                    font.pixelSize: 13; color: "#3c4a46"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ComboBox {
                    id: escenarioCombo
                    implicitWidth: 150
                    implicitHeight: 40
                    model: ["Realista", "Optimista"]

                    readonly property real value: Engine.inputs.growthScenario !== undefined
                                                   ? Engine.inputs.growthScenario : 0

                    Component.onCompleted: currentIndex = Math.round(value)
                    onValueChanged: currentIndex = Math.round(value)
                    onActivated: (idx) => Engine.set("growthScenario", idx)

                    background: Rectangle {
                        radius: 5
                        color: "#fffbe8"
                        border.color: escenarioCombo.activeFocus || escenarioCombo.popup.visible
                                      ? "#1a7a5e" : "#e0d6ac"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: escenarioCombo.displayText
                        font.pixelSize: 14
                        color: "#1e2b28"
                        leftPadding: 10
                        rightPadding: escenarioCombo.indicator.width + 8
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Text {
                        x: escenarioCombo.width - width - 10
                        y: (escenarioCombo.height - height) / 2
                        text: "▾"
                        font.pixelSize: 12
                        color: "#14523f"
                    }

                    popup: Popup {
                        y: escenarioCombo.height
                        width: escenarioCombo.width
                        implicitHeight: listView.contentHeight
                        padding: 1

                        contentItem: ListView {
                            id: listView
                            clip: true
                            implicitHeight: contentHeight
                            model: escenarioCombo.popup.visible ? escenarioCombo.delegateModel : null
                            currentIndex: escenarioCombo.highlightedIndex
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
                        width: escenarioCombo.width
                        highlighted: escenarioCombo.highlightedIndex === index

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
                Text {
                    text: "IPC"
                    font.pixelSize: 13; color: "#3c4a46"
                    visible: escenarioCombo.currentIndex === 1
                }
                PctField {
                    k: "ipcOptimistic"
                    visible: escenarioCombo.currentIndex === 1
                }
            }
            Text {
                Layout.fillWidth: true
                text: escenarioCombo.currentIndex === 1
                      ? "Se aplica el IPC indicado, constante, a los 10 años de la proyección (ventas, alquiler y otros gastos; los sueldos suben aparte, según la subida salarial fija de Configuración)."
                      : "Se aplica el IPC histórico de España de los últimos 10 años a la proyección (ventas, alquiler y otros gastos; los sueldos suben aparte, según la subida salarial fija de Configuración)."
                font.pixelSize: 12
                color: "#6b7a76"
                wrapMode: Text.WordWrap
            }

            // Margen comercial fijo del escenario Optimista, aplicado al resto
            // de la proyección (ver kMargenComercialOptimista en simcore.cpp).
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                visible: escenarioCombo.currentIndex === 1
                spacing: 4

                Text {
                    text: "M. Comercial % aplicado"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#14523f"
                }
                RowCard {
                    Text { text: "Año 1"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "optimisticMarginYear1"; Layout.alignment: Qt.AlignRight }
                }
                RowCard {
                    Text { text: "Año 2"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "optimisticMarginYear2"; Layout.alignment: Qt.AlignRight }
                }
                RowCard {
                    Text { text: "Año 3 y siguientes"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                    PctField { k: "optimisticMarginYear3"; Layout.alignment: Qt.AlignRight }
                }
            }
        }

        // ---------------- Ventas
        Card {
            SectionTitle { text: "Ventas" }
            EditRow { label: "Venta receta";  k: "prescriptionSales" }
            EditRow { label: "Venta libre";   k: "otcSales" }
            CalcRow { label: "Venta total";   value: Engine.baseData.totalSales; destacada: true }
        }

        // ---------------- Alquiler
        Card {
            SectionTitle { text: "Alquiler" }
            EditRow { label: "Alquiler local (mensual)"; k: "premisesRent"; multiplier: 12 }
        }

        // ---------------- Otros gastos
        Card {
            SectionTitle { text: "Otros gastos" }
            EditRow { label: "Suministros"; k: "utilities" }
            EditRow { label: "Gastos asesoría"; k: "advisoryFees" }
            EditRow { label: "Mantenimiento informático"; k: "maintenance" }
            EditRow { label: "Robot"; k: "robot" }
            EditRow { label: "Seguros"; k: "insurance" }
            EditRow { label: "Otros gastos"; k: "otherExpenses" }
            CalcRow { label: "Total otros gastos"; value: Engine.baseData.totalOtherExpenses; destacada: true }
        }

        // ---------------- Comparación de escenarios
        Button {
            id: btnComparar
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.topMargin: 4
            text: "Añadir a comparación"
            font.pixelSize: 13
            font.bold: true
            onClicked: {
                Engine.addComparisonScenario()
                avisoComparar.mostrar("Escenario añadido a la comparación")
            }
            background: Rectangle {
                radius: 8
                color: btnComparar.down ? "#0f5a43" : "#1a7a5e"
                border.color: "#2b8a6a"
            }
            contentItem: Text {
                text: btnComparar.text
                color: "#ffe9a8"
                font: btnComparar.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onPressed: (mouse) => mouse.accepted = false
            }
        }
        Text {
            id: avisoComparar
            visible: false
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            color: "#14523f"
            font.pixelSize: 12

            function mostrar(mensaje) {
                text = mensaje
                visible = true
                temporizadorAvisoComparar.restart()
            }
            Timer {
                id: temporizadorAvisoComparar
                interval: 3000
                onTriggered: avisoComparar.visible = false
            }
        }
    }
}
