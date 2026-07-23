pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Simulación": una sola tabla con una columna por escenario, sin
// nombres de columna ni "ojo" — solo el botón "✕" para borrar (salvo en la
// columna 0). La columna 0 ("Actual") es siempre el escenario principal
// (Datos base/Financiación...) sin tocar. El botón "Añadir escenario" congela
// una columna más a partir de los 5 campos de arriba (Facturación Total,
// años/interés de la hipoteca —mobiliaria e inmobiliaria, ligados—,
// aportación inicial y margen comercial), precargados con el valor actual:
// el campo que no se edite hace que ese eje de la nueva columna siga el
// escenario principal para siempre (se recalcula solo si cambia, p.ej. en
// Datos base), y el que se edite queda fijo en ese número. El cálculo en sí
// es puro: no guarda nada ni afecta a Datos base/Financiación (solo los
// escenarios añadidos aquí se guardan, como cualquier otro dato de la app —
// ver Engine::addSimulationScenario()).
Flickable {
    id: page

    readonly property int wLabel: 240
    readonly property int wCell: 120
    readonly property int hRow: 30

    // Se refresca cuando cambia el año elegido, los escenarios guardados o
    // cualquier input del motor (Datos base, Financiación...).
    // simulationForYear() es un método invocable que lee el estado interno
    // del motor directamente: un binding declarativo no basta (una lectura
    // "descartada" de Engine.inputs solo para forzar la dependencia puede
    // optimizarse fuera del QML precompilado, ya que su valor no se usa), así
    // que se recalcula explícitamente con Connections en vez de depender de
    // que el motor de bindings detecte la dependencia por sí solo.
    property var filas: []
    function refreshFilas() {
        page.filas = Engine.simulationForYear(anioCombo.currentIndex)
    }
    Component.onCompleted: page.refreshFilas()
    Connections {
        target: anioCombo
        function onCurrentIndexChanged() { page.refreshFilas() }
    }
    Connections {
        target: Engine
        function onRecalculated() { page.refreshFilas() }
        function onSimulationScenariosChanged() { page.refreshFilas() }
    }

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Campo numérico precargado con el valor actual del escenario principal
    // (como MoneyField/NumField/PctField): mientras el usuario no lo edite
    // ("touched"), sigue ese valor si cambia en Datos base/Financiación. En
    // cuanto lo edita (textEdited, que solo se dispara por interacción del
    // usuario, no al asignar "text" por código), queda fijo como override de
    // ese eje en el próximo escenario.
    component ScenarioField: TextField {
        id: fld
        required property real mainValue
        property string kind: "eur" // "eur" | "years" | "pct1"
        property bool touched: false

        function display(v) {
            if (fld.kind === "pct1") return Fmt.num(v * 100, 1) + " %"
            if (fld.kind === "years") return Fmt.num(v, 0) + " años"
            return Fmt.num(v, 0) + " €"
        }
        // Override fijado por el usuario, o null si el campo aún sigue al
        // escenario principal (no editado desde el último resetToMain()).
        readonly property var overrideValue: {
            if (!fld.touched) return null
            const v = Fmt.parse(fld.text)
            if (isNaN(v)) return null
            return fld.kind === "pct1" ? v / 100 : v
        }
        function resetToMain() {
            fld.touched = false
            fld.text = fld.display(fld.mainValue)
        }

        implicitWidth: fld.kind === "eur" ? 130 : 90
        implicitHeight: 40
        horizontalAlignment: TextInput.AlignRight
        font.pixelSize: 14
        color: Tokens.textPrimary
        selectByMouse: true
        inputMethodHints: Qt.ImhFormattedNumbersOnly

        background: Rectangle {
            radius: 5
            color: Tokens.bgInput
            border.color: fld.activeFocus ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
            border.width: 1
        }

        Component.onCompleted: fld.text = fld.display(fld.mainValue)
        onMainValueChanged: if (!fld.touched) fld.text = fld.display(fld.mainValue)
        onTextEdited: fld.touched = true
        onAccepted: focus = false
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 24
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 1040)
        spacing: 14

        Text { text: "Simulación"; font.pixelSize: 22; font.bold: true; color: Tokens.textHeading }
        Text {
            Layout.fillWidth: true
            text: "La primera columna es siempre el escenario actual. Los campos de abajo arrancan "
                  + "con los valores actuales: edita solo los que quieras fijar en un nuevo escenario "
                  + "(el resto seguirá siempre al escenario actual) y pulsa \"Añadir escenario\" para "
                  + "añadirlo como columna. El cálculo en sí no se guarda ni afecta a Datos base/"
                  + "Financiación."
            font.pixelSize: 12
            color: Tokens.textMuted
            wrapMode: Text.WordWrap
        }

        // Grupo con el mismo estilo (título + línea de acento) que el resto
        // de la app (ver CollapsibleCard.qml, basado en Card + SectionTitle),
        // pero colapsable: expandido por defecto al abrir la app (expanded
        // no se guarda, es solo estado de la sesión), para poder dejarle más
        // hueco a la tabla sin necesidad de hacer scroll.
        CollapsibleCard {
            title: "Nuevo escenario"
            // Su borde derecho coincide con el del botón "Exportar a PDF" de
            // abajo (no Layout.fillWidth, que es lo que trae Card por
            // defecto): btnPdfSimulacion.x es su posición dentro de su Flow,
            // que empieza en el mismo x que esta tarjeta dentro de "col".
            Layout.fillWidth: false
            Layout.preferredWidth: btnPdfSimulacion.x + btnPdfSimulacion.width

            RowCard {
                Text { text: "Facturación Total"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                ScenarioField {
                    id: campoFacturacionNueva
                    kind: "eur"
                    mainValue: Engine.inputs.prescriptionSales + Engine.inputs.otcSales
                    Layout.alignment: Qt.AlignRight
                }
            }

            RowCard {
                Text { text: "Años hipoteca mobiliaria/inmobiliaria"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                ScenarioField { id: campoAniosNueva; kind: "years"; mainValue: Engine.inputs.bankTermYears; Layout.alignment: Qt.AlignRight }
            }

            RowCard {
                Text { text: "Interés hipoteca mobiliaria/inmobiliaria"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                ScenarioField { id: campoInteresNueva; kind: "pct1"; mainValue: Engine.inputs.bankRate; Layout.alignment: Qt.AlignRight }
            }

            RowCard {
                Text { text: "Aportación inicial"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                ScenarioField { id: campoAportacionNueva; kind: "eur"; mainValue: Engine.inputs.contributedCash; Layout.alignment: Qt.AlignRight }
            }

            RowCard {
                Text { text: "Margen comercial"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true }
                ScenarioField { id: campoMargenNueva; kind: "pct1"; mainValue: Engine.inputs.marginPct; Layout.alignment: Qt.AlignRight }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Item { Layout.fillWidth: true }

                Button {
                    id: btnAnadirEscenario
                    height: 40
                    text: "Añadir escenario"
                    font.pixelSize: 13
                    font.bold: true
                    onClicked: {
                        const overrides = {}
                        if (campoFacturacionNueva.overrideValue !== null) overrides.revenueEur = campoFacturacionNueva.overrideValue
                        if (campoAniosNueva.overrideValue !== null) overrides.termYears = campoAniosNueva.overrideValue
                        if (campoInteresNueva.overrideValue !== null) overrides.ratePct = campoInteresNueva.overrideValue
                        if (campoAportacionNueva.overrideValue !== null) overrides.cashEur = campoAportacionNueva.overrideValue
                        if (campoMargenNueva.overrideValue !== null) overrides.marginPct = campoMargenNueva.overrideValue
                        Engine.addSimulationScenario(overrides)
                        campoFacturacionNueva.resetToMain()
                        campoAniosNueva.resetToMain()
                        campoInteresNueva.resetToMain()
                        campoAportacionNueva.resetToMain()
                        campoMargenNueva.resetToMain()
                    }
                    background: Rectangle {
                        radius: 8
                        color: btnAnadirEscenario.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                        border.color: Tokens.borderButtonPrimary
                    }
                    contentItem: Text {
                        text: btnAnadirEscenario.text
                        color: Tokens.textButtonPrimary
                        font: btnAnadirEscenario.font
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
        }

        Flow {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Año"
                font.pixelSize: 13
                color: Tokens.textSecondary
                height: anioCombo.implicitHeight
                verticalAlignment: Text.AlignVCenter
            }
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

            Button {
                id: btnPdfSimulacion
                height: 40
                text: "Exportar a PDF (todos los años)"
                font.pixelSize: 13
                font.bold: true
                onClicked: {
                    const destino = Engine.exportSimulationPdf()
                    avisoPdfSimulacion.mostrar(destino.length > 0
                        ? "PDF guardado en: " + destino
                        : "No se pudo crear el PDF")
                }
                background: Rectangle {
                    radius: 8
                    color: btnPdfSimulacion.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                    border.color: Tokens.borderButtonPrimary
                }
                contentItem: Text {
                    text: btnPdfSimulacion.text
                    color: Tokens.textButtonPrimary
                    font: btnPdfSimulacion.font
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

            Text {
                id: avisoPdfSimulacion
                visible: false
                height: 40
                verticalAlignment: Text.AlignVCenter
                color: Tokens.textHeading
                font.pixelSize: 12

                function mostrar(mensaje) {
                    text = mensaje
                    visible = true
                    temporizadorAvisoPdfSimulacion.restart()
                }
                Timer {
                    id: temporizadorAvisoPdfSimulacion
                    interval: 4000
                    onTriggered: avisoPdfSimulacion.visible = false
                }
            }
        }

        // ---------------- una única tabla: columna 0 = Actual, resto = escenarios
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: tabla.implicitHeight + 28
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
            clip: true

            ConceptTable {
                id: tabla
                // Permite localizar la tabla en los tests QML (tst_SimulacionView.qml).
                objectName: "tabla"
                anchors.fill: parent
                anchors.margins: 14
                wLabel: page.wLabel
                wCell: page.wCell
                hRow: page.hRow
                showHeaderLabels: false // sin "ojo"/nombres: las columnas se identifican por su posición
                model: page.filas
                closableColumns: true
                firstClosableColumn: 1 // la columna 0 ("Actual") no se puede borrar
                onCloseColumn: (index) => Engine.removeSimulationScenario(index - 1)
                // La página (Flickable) ya lleva el scroll vertical: esta tabla
                // solo necesita desplazarse en horizontal.
                flickableDirection: Flickable.HorizontalFlick
                fallback: page
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
