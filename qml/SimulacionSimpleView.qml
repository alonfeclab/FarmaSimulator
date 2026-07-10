pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Simulación simple": un puñado de valores básicos y los 5 primeros
// años de la proyección. Es una simulación aislada ("de bolsillo"): parte de
// los valores actuales de las demás hojas, pero lo que se toca aquí NO se
// guarda ni se propaga a Datos Base/Financiación/Proyección — vive solo en
// esta pestaña.
Flickable {
    id: page

    readonly property int wLabel: 220
    readonly property int wCell: 108
    readonly property int hRow: 30
    readonly property int numAnios: 5

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }

    // Estado local, aislado del motor: se inicializa con los valores
    // actuales, pero al editarlo aquí deja de seguir a Engine.inputs.
    QtObject {
        id: local
        property real ventaReceta:       Engine.inputs.ventaReceta
        property real ventaLibre:        Engine.inputs.ventaLibre
        property real existencias:       Engine.inputs.existencias
        property real liquidezAportada:  Engine.inputs.liquidezAportada
        property real finPropiedades:    Engine.inputs.finPropiedades
        property real pedidoInicial:     Engine.inputs.pedidoInicial
        property real alquilerLocal:     Engine.inputs.alquilerLocal
    }

    // Proyección "de bolsillo": se recalcula con los valores locales sin
    // tocar el motor compartido.
    readonly property var preview: Engine.simularSimple({
        ventaReceta:      local.ventaReceta,
        ventaLibre:       local.ventaLibre,
        existencias:      local.existencias,
        liquidezAportada: local.liquidezAportada,
        finPropiedades:   local.finPropiedades,
        pedidoInicial:    local.pedidoInicial,
        alquilerLocal:    local.alquilerLocal,
    })
    readonly property var filasEsenciales: page.preview.proyeccion.filter(f => f.bold)

    component Card: Rectangle {
        default property alias content: box.data
        Layout.fillWidth: true
        radius: 12
        color: "white"
        border.color: "#dde5e1"
        implicitHeight: box.implicitHeight + 40
        ColumnLayout {
            id: box
            anchors.fill: parent
            anchors.margins: 20
            spacing: 8
        }
    }

    // Campo editable ligado al estado LOCAL de esta hoja (no a Engine).
    component LocalField: TextField {
        id: field

        property real value: 0
        signal committed(real v)

        function display() { return Fmt.num(value, 0) + " €" }

        implicitWidth: 130
        implicitHeight: 40
        horizontalAlignment: TextInput.AlignRight
        font.pixelSize: 14
        color: "#1e2b28"
        selectByMouse: true
        inputMethodHints: Qt.ImhFormattedNumbersOnly

        background: Rectangle {
            radius: 5
            color: "#fffbe8"
            border.color: field.activeFocus ? "#1a7a5e" : "#e0d6ac"
            border.width: 1
        }

        Component.onCompleted: text = display()
        onValueChanged: if (!activeFocus) text = display()
        onActiveFocusChanged: {
            if (activeFocus) {
                text = Fmt.num(value, 2).replace(/\./g, "")
                selectAll()
            } else {
                text = display()
            }
        }
        onAccepted: focus = false
        onEditingFinished: {
            const v = Fmt.parse(text)
            if (!isNaN(v)) committed(v)
            text = display()
        }
    }

    component EditRow: RowLayout {
        property string label
        property real value
        signal committed(real v)
        Layout.fillWidth: true
        Text { text: parent.label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        LocalField { value: parent.value; onCommitted: (v) => parent.committed(v) }
    }

    component CalcRow: RowLayout {
        property string label
        property real value
        property bool destacada: false
        Layout.fillWidth: true
        Text {
            text: label; font.pixelSize: 13; font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"; Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(value); font.pixelSize: 14; font.bold: true
            color: destacada ? "#14523f" : "#1e2b28"
        }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 24
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 820)
        spacing: 14

        Text { text: "Simulación simple"; font.pixelSize: 22; font.bold: true; color: "#14523f" }
        Text {
            Layout.fillWidth: true
            text: "Simulación aislada: parte de los valores actuales de las demás hojas, pero lo " +
                  "que cambies aquí no se guarda ni afecta a Datos Base, Financiación ni al resto " +
                  "de la app."
            font.pixelSize: 12
            color: "#6b7a76"
            wrapMode: Text.WordWrap
        }

        // ---------------- Valores básicos
        Card {
            SectionTitle { text: "VALORES BÁSICOS" }
            EditRow { label: "Venta receta"; value: local.ventaReceta; onCommitted: (v) => local.ventaReceta = v }
            EditRow { label: "Venta libre"; value: local.ventaLibre; onCommitted: (v) => local.ventaLibre = v }
            CalcRow { label: "VENTA TOTAL"; value: page.preview.ventaTotal; destacada: true }
            EditRow { label: "Existencias"; value: local.existencias; onCommitted: (v) => local.existencias = v }
            EditRow { label: "Aportación liquidez"; value: local.liquidezAportada; onCommitted: (v) => local.liquidezAportada = v }
            EditRow { label: "Aportación propiedad hipotecada"; value: local.finPropiedades; onCommitted: (v) => local.finPropiedades = v }
            EditRow { label: "Aportación cooperativa"; value: local.pedidoInicial; onCommitted: (v) => local.pedidoInicial = v }
            EditRow { label: "Alquiler"; value: local.alquilerLocal; onCommitted: (v) => local.alquilerLocal = v }
        }

        Text { text: "Proyección — primeros 5 años"; font.pixelSize: 18; font.bold: true; color: "#14523f" }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: tabla.implicitHeight + 28
            radius: 12
            color: "white"
            border.color: "#dde5e1"
            clip: true

            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: 14
                contentWidth: page.wLabel + page.numAnios * page.wCell
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
                            width: page.wLabel; height: page.hRow; color: "#14523f"
                            radius: 4
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 8
                                text: "Concepto"; color: "white"; font.bold: true; font.pixelSize: 13
                            }
                        }
                        Repeater {
                            model: page.numAnios
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

                    // -------- filas (solo las destacadas, solo los primeros años)
                    Repeater {
                        model: page.filasEsenciales
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
                                    width: page.wLabel - 12
                                    elide: Text.ElideRight
                                    text: fila.modelData.label
                                    font.pixelSize: 13
                                    font.bold: fila.modelData.bold
                                    color: fila.modelData.bold ? "#14523f" : "#3c4a46"
                                }
                            }
                            Repeater {
                                model: page.numAnios
                                Rectangle {
                                    id: celda
                                    required property int index
                                    readonly property real valor: fila.modelData.values[index]
                                    width: page.wCell; height: page.hRow
                                    color: fila.modelData.bold ? "#e3efe9"
                                         : (fila.index % 2 ? "#f7faf8" : "white")
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.right: parent.right; anchors.rightMargin: 8
                                        text: Fmt.byFmt(celda.valor, fila.modelData.fmt)
                                        font.pixelSize: 13
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
        }

        Item { Layout.preferredHeight: 8 }
    }
}
