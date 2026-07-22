pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Simulación": agrupa por Facturación Total (igual / + un % configurable
// en Configuración) las combinaciones de plazo e interés de la hipoteca
// (mobiliaria e inmobiliaria, ligados) y aportación inicial, partiendo del
// resto de los datos actuales. Cálculo puro (Engine.simulationForYear): no
// guarda nada ni afecta a Datos base/Financiación. Un grupo por facturación,
// apilados uno debajo de otro.
Flickable {
    id: page

    readonly property int wLabel: 240
    readonly property int wCell: 110
    readonly property int hRow: 30

    // Se refresca cuando cambia el año elegido o cualquier input del motor
    // (plazo, tipo, facturación en Datos base, el % de Configuración...).
    // simulationForYear() es un método invocable que lee el estado interno
    // del motor directamente: un binding declarativo no basta (una lectura
    // "descartada" de Engine.inputs solo para forzar la dependencia puede
    // optimizarse fuera del QML precompilado, ya que su valor no se usa), así
    // que se recalcula explícitamente con Connections en vez de depender de
    // que el motor de bindings detecte la dependencia por sí solo.
    property var grupos: []
    function refreshGrupos() {
        page.grupos = Engine.simulationForYear(anioCombo.currentIndex)
    }
    Component.onCompleted: page.refreshGrupos()
    Connections {
        target: anioCombo
        function onCurrentIndexChanged() { page.refreshGrupos() }
    }
    Connections {
        target: Engine
        function onRecalculated() { page.refreshGrupos() }
    }

    // "CN" (combinación N) en vez de "Año N", abreviado para que quepa en la
    // columna, y para no confundirse con los "Escenario N" guardados en
    // Comparación: la columna N es la misma combinación de plazo/interés/
    // aportación en los 2 grupos (mismo orden en cada uno), solo cambia la
    // Facturación Total de un grupo a otro.
    readonly property var combinacionLabels: {
        const n = page.grupos.length > 0 && page.grupos[0].rows.length > 0
                  ? page.grupos[0].rows[0].values.length : 0
        const labels = []
        for (let i = 0; i < n; ++i) labels.push("C" + (i + 1))
        return labels
    }

    // Scroll horizontal común a las 2 tablas (una por Facturación Total),
    // para poder comparar la misma combinación de columna entre grupos.
    // Guard contra el bucle infinito: al igualar el contentX de las otras
    // tablas se dispara su propio onContentXChanged, que debe ser un no-op.
    property bool syncingScroll: false
    function syncScroll(source) {
        if (page.syncingScroll) return
        page.syncingScroll = true
        for (let i = 0; i < repeaterGrupos.count; ++i) {
            const item = repeaterGrupos.itemAt(i)
            if (item && item.tablaRef && item.tablaRef !== source)
                item.tablaRef.contentX = source.contentX
        }
        page.syncingScroll = false
    }

    // Recopila las columnas colapsadas ("ojo") de cada tabla por separado
    // (cada grupo lleva ahora su propio filtro, ver grupoCol.collapsedColumns
    // más abajo), para pasárselas a Engine.exportSimulationPdf en el mismo
    // orden que sus grupos.
    function allCollapsedColumns() {
        const arr = []
        for (let i = 0; i < repeaterGrupos.count; ++i) {
            const item = repeaterGrupos.itemAt(i)
            arr.push(item ? item.collapsedColumns : [])
        }
        return arr
    }

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 24
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 1040)
        spacing: 14

        Text { text: "Simulación"; font.pixelSize: 22; font.bold: true; color: "#14523f" }
        Text {
            Layout.fillWidth: true
            text: "Combinaciones de plazo e interés de la hipoteca (mobiliaria e inmobiliaria) y aportación "
                  + "inicial, agrupadas por Facturación Total (igual / +" + Fmt.pct(Engine.inputs.simulationRevenueIncreasePct, 0)
                  + ", configurable en Configuración). No se guarda ni afecta al resto de la app."
            font.pixelSize: 12
            color: "#6b7a76"
            wrapMode: Text.WordWrap
        }

        Flow {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Año"
                font.pixelSize: 13
                color: "#3c4a46"
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

            Button {
                id: btnPdfSimulacion
                height: 40
                text: "Exportar a PDF (todos los años)"
                font.pixelSize: 13
                font.bold: true
                onClicked: {
                    const destino = Engine.exportSimulationPdf(page.allCollapsedColumns())
                    avisoPdfSimulacion.mostrar(destino.length > 0
                        ? "PDF guardado en: " + destino
                        : "No se pudo crear el PDF")
                }
                background: Rectangle {
                    radius: 8
                    color: btnPdfSimulacion.down ? "#0f5a43" : "#1a7a5e"
                    border.color: "#2b8a6a"
                }
                contentItem: Text {
                    text: btnPdfSimulacion.text
                    color: "#ffe9a8"
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
                color: "#14523f"
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

        // ---------------- un bloque por cada Facturación Total, apilados
        Repeater {
            id: repeaterGrupos
            model: page.grupos
            delegate: ColumnLayout {
                id: grupoCol
                required property var modelData
                required property int index
                // Referencia a la tabla de este grupo, para que page.syncScroll()
                // pueda alcanzarla vía repeaterGrupos.itemAt(i).tablaRef.
                property alias tablaRef: tabla
                Layout.fillWidth: true
                Layout.topMargin: grupoCol.index > 0 ? 8 : 0
                spacing: 6

                // Columnas colapsadas (filtro visual con el "ojo" de la
                // cabecera) de esta tabla en concreto: cada grupo (Facturación
                // Total) lleva su propio filtro, así que ocultar una columna
                // aquí no afecta a las demás tablas.
                property var collapsedColumns: []
                function toggleColumn(index) {
                    const i = grupoCol.collapsedColumns.indexOf(index)
                    const arr = grupoCol.collapsedColumns.slice()
                    if (i === -1) arr.push(index)
                    else arr.splice(i, 1)
                    grupoCol.collapsedColumns = arr
                }

                Text {
                    text: "Facturación total: " + Fmt.eur(grupoCol.modelData.facturacion)
                    font.pixelSize: 15
                    font.bold: true
                    color: "#14523f"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: tabla.implicitHeight + 28
                    radius: 12
                    color: "white"
                    border.color: "#dde5e1"
                    clip: true

                    ConceptTable {
                        id: tabla
                        // Permite localizar cada tabla en los tests QML (tst_SimulacionView.qml).
                        objectName: "tabla" + grupoCol.index
                        anchors.fill: parent
                        anchors.margins: 14
                        wLabel: page.wLabel
                        wCell: page.wCell
                        hRow: page.hRow
                        headerLabels: page.combinacionLabels
                        model: grupoCol.modelData.rows
                        collapsibleColumns: true
                        collapsedColumns: grupoCol.collapsedColumns
                        onToggleColumn: (index) => grupoCol.toggleColumn(index)
                        // La página (Flickable) ya lleva el scroll vertical: esta tabla
                        // solo necesita desplazarse en horizontal (8 columnas).
                        flickableDirection: Flickable.HorizontalFlick
                        fallback: page
                        onContentXChanged: page.syncScroll(tabla)
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
