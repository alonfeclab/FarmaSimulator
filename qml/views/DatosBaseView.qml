pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Datos Base": PyG estimada para el estudio.
Flickable {
    id: page

    readonly property bool angosto: width < 640

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Claves de las series históricas (aumento de facturación INE y margen
    // comercial simulado), usadas por el botón "Restaurar valores por
    // defecto" del grupo "Escenario de crecimiento".
    function historicalSeriesKeys() {
        var ks = []
        for (var k = 0; k < 10; k++) {
            ks.push("annualRevenueIncrease" + k)
            ks.push("annualRevenueIncreasePrescription" + k)
            ks.push("realisticMarginSeries" + k)
        }
        ks.push("sameRealisticGrowth")
        return ks
    }

    // Fila calculada (solo lectura). El estilo de la caja (fondo/borde) vive
    // en RowCard; aquí solo se define el contenido de esta hoja.
    component CalcRow: RowCard {
        property string label
        property real value
        Text {
            text: label
            font.pixelSize: 13
            font.bold: destacada
            color: destacada ? Tokens.textHeading : Tokens.textSecondary
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(value)
            font.pixelSize: 14
            font.bold: true
            color: destacada ? Tokens.textHeading : Tokens.textPrimary
            Layout.alignment: Qt.AlignRight
        }
    }

    // Fila editable. Mismo RowCard que CalcRow, para que ambas se lean igual.
    component EditRow: RowCard {
        id: editRow
        property string label
        property string k
        property real multiplier: 1
        Text { text: editRow.label; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
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

        Text { text: "Datos base del estudio"; font.pixelSize: 22; font.bold: true; color: Tokens.textHeading }

        // ---------------- Escenario de crecimiento
        CollapsibleCard {
            title: "Escenario de crecimiento de facturación"
            headerContent: ResetGroupButton {
                keys: page.historicalSeriesKeys()
                compact: page.angosto
                visible: escenarioCombo.currentIndex === 0
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "Tipo"
                    font.pixelSize: 13; color: Tokens.textSecondary
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
                        color: Tokens.bgInput
                        border.color: escenarioCombo.activeFocus || escenarioCombo.popup.visible
                                      ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
                        border.width: 1
                    }

                    contentItem: Text {
                        text: escenarioCombo.displayText
                        font.pixelSize: 14
                        color: Tokens.textPrimary
                        leftPadding: 10
                        rightPadding: escenarioCombo.indicator.width + 8
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Text {
                        x: escenarioCombo.width - width - 10
                        y: (escenarioCombo.height - height) / 2
                        text: "▾"
                        font.pixelSize: 12
                        color: Tokens.textHeading
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
                            color: Tokens.bgInput
                            border.color: Tokens.borderInputDefault
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
                            color: Tokens.textPrimary
                            leftPadding: 10
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: comboDelegate.highlighted ? Tokens.bgDropdownItemHighlighted : Tokens.bgInput
                        }
                    }
                }
            }

            // Aumento de facturación del escenario Optimista, separado por
            // tipo de venta. Con "Mismo crecimiento" marcado, el de venta
            // receta copia siempre el de venta libre y su campo se oculta.
            ColumnLayout {
                id: aumentoOptimista
                Layout.fillWidth: true
                visible: escenarioCombo.currentIndex === 1
                spacing: 4

                readonly property bool mismoCrecimiento: Engine.inputs.sameOptimisticGrowth === undefined
                                                         || Engine.inputs.sameOptimisticGrowth !== 0

                CheckField {
                    text: "Mismo crecimiento"
                    checked: aumentoOptimista.mismoCrecimiento
                    onToggled: {
                        Engine.set("sameOptimisticGrowth", checked ? 1 : 0)
                        if (checked)
                            Engine.set("ipcOptimisticPrescription", Engine.inputs.ipcOptimistic)
                    }
                }
                RowCard {
                    Text { text: aumentoOptimista.mismoCrecimiento ? "Aumento de facturación" : "Aumento de facturación venta libre"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    PctField {
                        k: "ipcOptimistic"
                        Layout.alignment: Qt.AlignRight
                        onValueChanged: if (aumentoOptimista.mismoCrecimiento
                                            && Engine.inputs.ipcOptimisticPrescription !== value)
                                            Engine.set("ipcOptimisticPrescription", value)
                    }
                }
                RowCard {
                    visible: !aumentoOptimista.mismoCrecimiento
                    Text { text: "Aumento de facturación venta receta (seguro)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    PctField {
                        k: "ipcOptimisticPrescription"
                        Layout.alignment: Qt.AlignRight
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                visible: escenarioCombo.currentIndex === 1
                text: "Se aplica el aumento de facturación indicado, constante, a los 10 años de la proyección: el de venta libre a la venta libre; el de venta receta a la venta receta y a los Reales Decretos. Los sueldos, el alquiler y otros gastos suben aparte, según el IPC fijo de Configuración."
                font.pixelSize: 12
                color: Tokens.textMuted
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
                    color: Tokens.textHeading
                }
                RowCard {
                    Text { text: "Año 1"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                    PctField { k: "optimisticMarginYear1"; Layout.alignment: Qt.AlignRight }
                }
                RowCard {
                    Text { text: "Año 2"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                    PctField { k: "optimisticMarginYear2"; Layout.alignment: Qt.AlignRight }
                }
                RowCard {
                    Text { text: "Año 3 y siguientes"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                    PctField { k: "optimisticMarginYear3"; Layout.alignment: Qt.AlignRight }
                }
            }

            // Series históricas del escenario Realista (aumento de
            // facturación INE y margen comercial simulado), editables desde
            // aquí en lugar de Configuración.
            // El aumento de facturación va separado por tipo de venta, igual
            // que en Optimista: con "Mismo crecimiento" marcado, la serie de
            // venta receta copia siempre la de venta libre y se oculta.
            ColumnLayout {
                id: seriesRealista
                Layout.fillWidth: true
                Layout.topMargin: 4
                visible: escenarioCombo.currentIndex === 0
                spacing: 8

                readonly property bool mismoCrecimiento: Engine.inputs.sameRealisticGrowth === undefined
                                                         || Engine.inputs.sameRealisticGrowth !== 0

                CheckField {
                    text: "Mismo crecimiento"
                    checked: seriesRealista.mismoCrecimiento
                    onToggled: {
                        Engine.set("sameRealisticGrowth", checked ? 1 : 0)
                        if (checked)
                            for (var y = 0; y < 10; y++)
                                Engine.set("annualRevenueIncreasePrescription" + y,
                                           Engine.inputs["annualRevenueIncrease" + y])
                    }
                }
                Text { text: seriesRealista.mismoCrecimiento ? "Aumento de facturación" : "Aumento de facturación venta libre"; font.pixelSize: 12; font.bold: true; color: Tokens.textSecondary }
                Flickable {
                    id: scrollIpc
                    Layout.fillWidth: true
                    Layout.preferredHeight: serieIpc.implicitHeight
                    contentWidth: serieIpc.implicitWidth
                    contentHeight: serieIpc.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                    FastWheel { flick: scrollIpc; fallback: page }
                    SerieAnualEdit {
                        id: serieIpc
                        prefix: "annualRevenueIncrease"
                        mirrorPrefix: seriesRealista.mismoCrecimiento ? "annualRevenueIncreasePrescription" : ""
                    }
                }
                Text { text: "Aumento de facturación venta receta (seguro)"; font.pixelSize: 12; font.bold: true; color: Tokens.textSecondary; Layout.topMargin: 8; visible: !seriesRealista.mismoCrecimiento }
                Flickable {
                    id: scrollIpcReceta
                    visible: !seriesRealista.mismoCrecimiento
                    Layout.fillWidth: true
                    Layout.preferredHeight: serieIpcReceta.implicitHeight
                    contentWidth: serieIpcReceta.implicitWidth
                    contentHeight: serieIpcReceta.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                    FastWheel { flick: scrollIpcReceta; fallback: page }
                    SerieAnualEdit {
                        id: serieIpcReceta
                        prefix: "annualRevenueIncreasePrescription"
                    }
                }
                Text { text: "Margen comercial simulado"; font.pixelSize: 12; font.bold: true; color: Tokens.textSecondary; Layout.topMargin: 8 }
                Flickable {
                    id: scrollMargen
                    Layout.fillWidth: true
                    Layout.preferredHeight: serieMargen.implicitHeight
                    contentWidth: serieMargen.implicitWidth
                    contentHeight: serieMargen.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                    FastWheel { flick: scrollMargen; fallback: page }
                    SerieAnualEdit { id: serieMargen; prefix: "realisticMarginSeries" }
                }
            }
        }

        // ---------------- Ventas
        CollapsibleCard {
            title: "Ventas"
            EditRow { label: "Venta receta";  k: "prescriptionSales" }
            EditRow { label: "Venta libre";   k: "otcSales" }
            CalcRow { label: "Venta total";   value: Engine.baseData.totalSales; destacada: true }
        }

        // ---------------- Alquiler
        CollapsibleCard {
            title: "Alquiler"
            EditRow { label: "Alquiler local (mensual, sin IVA)"; k: "premisesRent"; multiplier: 12 }
        }

        // ---------------- Otros gastos
        CollapsibleCard {
            title: "Otros gastos"
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
                color: btnComparar.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                border.color: Tokens.borderButtonPrimary
            }
            contentItem: Text {
                text: btnComparar.text
                color: Tokens.textButtonPrimary
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
            color: Tokens.textHeading
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
