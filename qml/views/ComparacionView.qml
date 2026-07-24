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

    readonly property bool vacio: Engine.comparisonScenarios.length === 0

    // Se recalcula cuando cambian los escenarios guardados o el año elegido:
    // referenciar Engine.comparisonScenarios aquí es necesario para que el
    // binding se re-evalúe (comparisonForYear es invocable, no una Q_PROPERTY).
    readonly property var filas: Engine.comparisonScenarios.length > 0
                                  ? Engine.comparisonForYear(anioCombo.currentIndex) : []

    // Grupo "Financiación" de la vista completa: valores fijos, no dependen del año.
    readonly property var filasFinanciacion: Engine.comparisonScenarios.length > 0
                                              ? Engine.financingComparison() : []

    // Inserta el grupo "Financiación" justo después de la fila "Venta total"
    // en vez de al final de la tabla. Las filas del grupo llevan "grupo: true"
    // (fondo propio, sin la franja alterna normal) y la última "groupEnd: true"
    // (línea divisoria inferior), para que se distingan del resto de la tabla.
    readonly property var filasTabla: {
        if (!switchVistaCompleta.checked)
            return page.filas
        const datosGrupo = page.filasFinanciacion.map((f, i, arr) =>
            Object.assign({}, f, { grupo: true, groupEnd: i === arr.length - 1 }))
        const grupo = [{ label: "Financiación", separator: true }].concat(datosGrupo)
        const idx = page.filas.findIndex(f => f.label === "Venta total")
        if (idx < 0)
            return page.filas.concat(grupo)
        return page.filas.slice(0, idx + 1).concat(grupo, page.filas.slice(idx + 1))
    }

    function resetScroll() { flick.contentX = 0; flick.contentY = 0 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text { text: "Comparación de escenarios"; font.pixelSize: 22; font.bold: true; color: Tokens.textHeading }

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
                color: Tokens.bgBrandStrong
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
                color: Tokens.textHeading
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 360
            }
            Text {
                text: "Ve a la hoja \"Datos base\" y pulsa \"Añadir a comparación\" para guardar el escenario actual."
                font.pixelSize: 13
                color: Tokens.textMuted
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

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Switch {
                    id: switchVistaCompleta
                    height: 40
                    text: "Vista completa"
                    checked: false

                    indicator: Rectangle {
                        implicitWidth: 40
                        implicitHeight: 22
                        x: switchVistaCompleta.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 11
                        color: switchVistaCompleta.checked ? Tokens.bgSwitchTrackOn : Tokens.bgSwitchTrackOff
                        border.color: switchVistaCompleta.checked ? Tokens.bgSwitchTrackOn : Tokens.borderSwitchTrackOff

                        Rectangle {
                            x: switchVistaCompleta.checked ? parent.width - width - 3 : 3
                            y: 3
                            width: 16; height: 16; radius: 8
                            color: Tokens.bgSwitchThumb
                            Behavior on x { NumberAnimation { duration: 120 } }
                        }
                    }

                    contentItem: Text {
                        text: switchVistaCompleta.text
                        font.pixelSize: 13
                        color: Tokens.textSecondary
                        leftPadding: switchVistaCompleta.indicator.width + switchVistaCompleta.spacing
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                RowLayout {
                    spacing: 8

                    Text { text: "Año"; font.pixelSize: 13; color: Tokens.textSecondary }
                    ComboBox {
                        id: anioCombo
                        implicitWidth: 150
                        implicitHeight: 40
                        model: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map(n => "Año " + n)

                        background: Rectangle {
                            radius: 5
                            color: Tokens.bgInput
                            border.color: anioCombo.activeFocus || anioCombo.popup.visible
                                          ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
                            border.width: 1
                        }

                        contentItem: Text {
                            text: anioCombo.displayText
                            font.pixelSize: 14
                            color: Tokens.textPrimary
                            leftPadding: 10
                            rightPadding: anioCombo.indicator.width + 8
                            verticalAlignment: Text.AlignVCenter
                        }

                        indicator: Text {
                            x: anioCombo.width - width - 10
                            y: (anioCombo.height - height) / 2
                            text: "▾"
                            font.pixelSize: 12
                            color: Tokens.textHeading
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
                                color: Tokens.bgInput
                                border.color: Tokens.borderInputDefault
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

                Text {
                    id: avisoPdfComparacion
                    visible: false
                    height: 40
                    verticalAlignment: Text.AlignVCenter
                    color: Tokens.textHeading
                    font.pixelSize: 12

                    function mostrar(mensaje) {
                        text = mensaje
                        visible = true
                        temporizadorAvisoPdfComparacion.restart()
                    }
                    Timer {
                        id: temporizadorAvisoPdfComparacion
                        interval: 4000
                        onTriggered: avisoPdfComparacion.visible = false
                    }
                }

                Button {
                    id: btnPdfComparacion
                    height: 40
                    text: "Exportar tabla a PDF"
                    font.pixelSize: 13
                    font.bold: true
                    onClicked: dlgExportarPdf.open()
                    background: Rectangle {
                        radius: 8
                        color: btnPdfComparacion.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                        border.color: Tokens.borderButtonPrimary
                    }
                    contentItem: Text {
                        text: btnPdfComparacion.text
                        color: Tokens.textButtonPrimary
                        font: btnPdfComparacion.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 12
                        rightPadding: 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: (mouse) => mouse.accepted = false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: Tokens.bgSurface
                border.color: Tokens.borderSurface
                clip: true

                ConceptTable {
                    id: flick
                    anchors.fill: parent
                    anchors.margins: 14
                    wLabel: page.wLabel
                    wCell: page.wCell
                    hRow: page.hRow
                    headerLabels: Engine.comparisonScenarios.map(e => e.name)
                    closableColumns: true
                    applyableColumns: true
                    model: page.filasTabla
                    onCloseColumn: (index) => dlgBorrarEscenario.confirmar(index)
                    onApplyColumn: (index) => dlgAplicarEscenario.confirmar(index)
                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }
    }

    // ---------------- messagebox: elegir año seleccionado o todos los años
    Popup {
        id: dlgExportarPdf
        anchors.centerIn: parent
        width: 340
        modal: true
        focus: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
        }

        function exportar(anio) {
            const destino = Engine.exportComparisonPdf(anio)
            avisoPdfComparacion.mostrar(destino.length > 0
                ? "PDF guardado en: " + destino
                : "No se pudo crear el PDF")
            dlgExportarPdf.close()
        }

        contentItem: ColumnLayout {
            width: dlgExportarPdf.availableWidth
            spacing: 14

            Text {
                text: "Exportar tabla a PDF"
                font.pixelSize: 15
                font.bold: true
                color: Tokens.textHeading
            }
            Text {
                text: "¿Exportar solo el año actual o los 10 años?"
                font.pixelSize: 13
                color: Tokens.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Button {
                id: btnAnioSeleccionado
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "Solo año " + (anioCombo.currentIndex + 1)
                font.pixelSize: 13
                font.bold: true
                onClicked: dlgExportarPdf.exportar(anioCombo.currentIndex)
                background: Rectangle {
                    radius: 8
                    color: btnAnioSeleccionado.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                    border.color: Tokens.borderButtonPrimary
                }
                contentItem: Text {
                    text: btnAnioSeleccionado.text
                    color: Tokens.textButtonPrimary
                    font: btnAnioSeleccionado.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }

            Button {
                id: btnTodosAnios
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "Todos los años (10 páginas)"
                font.pixelSize: 13
                font.bold: true
                onClicked: dlgExportarPdf.exportar(-1)
                background: Rectangle {
                    radius: 8
                    color: btnTodosAnios.down ? Tokens.bgInputPressed : Tokens.bgInput
                    border.color: Tokens.borderInputDefault
                }
                contentItem: Text {
                    text: btnTodosAnios.text
                    color: Tokens.textHeading
                    font: btnTodosAnios.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }

            Button {
                id: btnCancelarExportar
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                text: "Cancelar"
                font.pixelSize: 12
                flat: true
                onClicked: dlgExportarPdf.close()
                background: Rectangle { color: Tokens.bgTransparent }
                contentItem: Text {
                    text: btnCancelarExportar.text
                    color: Tokens.textMuted
                    font: btnCancelarExportar.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }
        }
    }

    // ---------------- messagebox: confirmar aplicar un escenario al actual
    Popup {
        id: dlgAplicarEscenario
        anchors.centerIn: parent
        width: 340
        modal: true
        focus: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property int pendingIndex: -1
        property string pendingName: ""

        background: Rectangle {
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
        }

        function confirmar(index) {
            pendingIndex = index
            pendingName = Engine.comparisonScenarios[index].name
            open()
        }

        contentItem: ColumnLayout {
            width: dlgAplicarEscenario.availableWidth
            spacing: 14

            Text {
                text: "Aplicar \"" + dlgAplicarEscenario.pendingName + "\""
                font.pixelSize: 15
                font.bold: true
                color: Tokens.textHeading
            }
            Text {
                text: "Se sobrescribirán los datos actuales (Datos base, Financiación, Personal, Configuración) con los valores guardados en este escenario. Esta acción no se puede deshacer."
                font.pixelSize: 13
                color: Tokens.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Button {
                id: btnConfirmarAplicar
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "Aplicar al escenario actual"
                font.pixelSize: 13
                font.bold: true
                onClicked: {
                    const ok = Engine.applyComparisonScenario(dlgAplicarEscenario.pendingIndex)
                    dlgAplicarEscenario.close()
                    avisoPdfComparacion.mostrar(ok
                        ? "Escenario aplicado a los datos actuales"
                        : "Este escenario es demasiado antiguo y no guarda datos para aplicar")
                }
                background: Rectangle {
                    radius: 8
                    color: btnConfirmarAplicar.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                    border.color: Tokens.borderButtonPrimary
                }
                contentItem: Text {
                    text: btnConfirmarAplicar.text
                    color: Tokens.textButtonPrimary
                    font: btnConfirmarAplicar.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }

            Button {
                id: btnCancelarAplicar
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                text: "Cancelar"
                font.pixelSize: 12
                flat: true
                onClicked: dlgAplicarEscenario.close()
                background: Rectangle { color: Tokens.bgTransparent }
                contentItem: Text {
                    text: btnCancelarAplicar.text
                    color: Tokens.textMuted
                    font: btnCancelarAplicar.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }
        }
    }

    // ---------------- messagebox: confirmar borrar un escenario
    Popup {
        id: dlgBorrarEscenario
        anchors.centerIn: parent
        width: 340
        modal: true
        focus: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property int pendingIndex: -1
        property string pendingName: ""

        background: Rectangle {
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
        }

        function confirmar(index) {
            pendingIndex = index
            pendingName = Engine.comparisonScenarios[index].name
            open()
        }

        contentItem: ColumnLayout {
            width: dlgBorrarEscenario.availableWidth
            spacing: 14

            Text {
                text: "Borrar \"" + dlgBorrarEscenario.pendingName + "\""
                font.pixelSize: 15
                font.bold: true
                color: Tokens.textHeading
            }
            Text {
                text: "Se eliminará este escenario de la comparación. Esta acción no se puede deshacer."
                font.pixelSize: 13
                color: Tokens.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Button {
                id: btnConfirmarBorrar
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "Borrar escenario"
                font.pixelSize: 13
                font.bold: true
                onClicked: {
                    Engine.removeComparisonScenario(dlgBorrarEscenario.pendingIndex)
                    dlgBorrarEscenario.close()
                }
                background: Rectangle {
                    radius: 8
                    color: btnConfirmarBorrar.down ? Tokens.bgButtonDangerPressed : Tokens.bgButtonDangerDefault
                    border.color: Tokens.borderButtonDanger
                }
                contentItem: Text {
                    text: btnConfirmarBorrar.text
                    color: Tokens.textOnDark
                    font: btnConfirmarBorrar.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }

            Button {
                id: btnCancelarBorrar
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                text: "Cancelar"
                font.pixelSize: 12
                flat: true
                onClicked: dlgBorrarEscenario.close()
                background: Rectangle { color: Tokens.bgTransparent }
                contentItem: Text {
                    text: btnCancelarBorrar.text
                    color: Tokens.textMuted
                    font: btnCancelarBorrar.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }
        }
    }
}
