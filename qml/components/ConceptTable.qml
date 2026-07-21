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

    // Cabeceras de columna alternativas a "Año N" (p.ej. nombres de escenario
    // en la hoja Comparación). Si está vacío se usa "Año N" como siempre.
    property var headerLabels: []
    // Si true, cada cabecera de columna muestra un botón "✕" para quitarla.
    property bool closableColumns: false
    signal closeColumn(int index)

    // Si true, cada cabecera de columna muestra un botón "aplicar" para
    // volcar ese escenario sobre los datos actuales (ver Comparación).
    property bool applyableColumns: false
    signal applyColumn(int index)

    // Si true, cada cabecera de columna muestra un botón "ojo" para
    // colapsarla/expandirla (filtro visual, ver Simulación). El array de
    // índices colapsados vive fuera del componente (p.ej. en la página que
    // lo controla), para poder compartir el mismo estado entre varias tablas
    // sincronizadas.
    property bool collapsibleColumns: false
    property var collapsedColumns: []
    readonly property int collapsedWidth: 32
    signal toggleColumn(int index)

    function isColumnCollapsed(index) {
        return root.collapsedColumns.indexOf(index) !== -1
    }
    function columnWidth(index) {
        return (root.collapsibleColumns && root.isColumnCollapsed(index)) ? root.collapsedWidth : root.wCell
    }
    function totalColumnsWidth() {
        let w = 0
        for (let i = 0; i < root.cols; ++i) w += root.columnWidth(i)
        return w
    }

    // Resumen "concepto: valor" de una columna colapsada, para mostrarlo en
    // el tooltip del ojo (una columna colapsada no muestra sus valores).
    function columnSummary(index) {
        const header = root.headerLabels.length > index
                       ? root.headerLabels[index] : ("Año " + (index + 1))
        const lineas = [header]
        for (let r = 0; r < root.model.length; ++r) {
            const fila = root.model[r]
            if (!fila || fila.separator) continue
            const valores = fila.values
            const valor = (valores && index < valores.length) ? valores[index] : 0
            lineas.push(fila.label + ": " + Fmt.byFmt(valor, fila.fmt))
        }
        return lineas.join("\n")
    }

    // Página que debe recibir el scroll de rueda cuando esta tabla no lo
    // necesite (ver FastWheel). Null cuando la tabla es el contenido
    // principal de la página (Proyección, Comparación).
    property Flickable fallback: null

    implicitHeight: tabla.implicitHeight
    contentWidth: wLabel + root.totalColumnsWidth()
    contentHeight: tabla.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    // Por defecto se permite el scroll en ambas direcciones: en las hojas
    // donde la tabla ocupa el espacio restante de la página (Proyección,
    // Comparación) es ella misma quien debe llevar el scroll vertical. En
    // las hojas donde la tabla vive dentro de una página que ya es
    // Flickable (Impuestos, Personal, Simulación simple), la vista fija
    // flickableDirection a HorizontalFlick para no capturar la rueda del
    // ratón en un "rebote" vertical inútil (ver ese fix de WASM).
    pressDelay: 150
    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

    FastWheel { flick: root; fallback: root.fallback }

    // Tooltip único y reutilizado por todas las cabeceras "ojo" y "aplicar"
    // (una tabla puede tener muchas columnas, cada una con su propio
    // HoverHandler). Usar
    // el atributo ToolTip.visible/text por celda crea un popup por celda, y
    // pasar el ratón rápido por varias a la vez dispara una carrera al crear
    // el componente del estilo ("QQmlComponent: Component is not ready") que
    // deja el tooltip sin mostrarse. Con una sola instancia reaprovechada
    // (reparentada a la celda que esté bajo el ratón) no hay carrera.
    ToolTip {
        id: ojoTooltip
        visible: false
    }

    Column {
        id: tabla

        // -------- cabecera
        Row {
            Rectangle {
                width: root.wLabel; height: root.hRow; color: "#14523f"
                radius: 4; topRightRadius: 0; bottomRightRadius: 0
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left; anchors.leftMargin: 8
                    text: root.headerLabel; color: "white"; font.bold: true; font.pixelSize: root.fontSize
                }
            }
            Repeater {
                model: root.cols
                Rectangle {
                    id: hdrCell
                    required property int index
                    readonly property bool colapsada: root.collapsibleColumns && root.isColumnCollapsed(hdrCell.index)
                    readonly property bool hasIcons: root.closableColumns || root.applyableColumns || root.collapsibleColumns
                    // Ancho reservado para los iconos (incluido el hueco que Row deja
                    // antes de cada uno), para poder acotar el ancho del nombre y que
                    // el conjunto nombre+iconos quede centrado sin desbordar la columna.
                    readonly property real iconsWidth: (root.collapsibleColumns ? btnOjo.implicitWidth + headerContent.spacing : 0)
                                                       + (root.applyableColumns ? btnAplicar.implicitWidth + headerContent.spacing : 0)
                                                       + (root.closableColumns ? btnCerrar.implicitWidth + headerContent.spacing : 0)
                    width: root.columnWidth(hdrCell.index); height: root.hRow; color: "#14523f"

                    Row {
                        id: headerContent
                        anchors.centerIn: parent
                        spacing: 10

                        Text {
                            id: lbl
                            visible: !hdrCell.colapsada
                            anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                            width: Math.min(implicitWidth, Math.max(0, hdrCell.width - 16 - hdrCell.iconsWidth))
                            text: root.headerLabels.length > hdrCell.index
                                  ? root.headerLabels[hdrCell.index]
                                  : ("Año " + (hdrCell.index + 1))
                            color: "white"; font.bold: true; font.pixelSize: root.fontSize
                        }
                        Image {
                            id: btnOjo
                            visible: root.collapsibleColumns
                            anchors.verticalCenter: parent.verticalCenter
                            source: "qrc:/qt/qml/FarmaciaSim/icons/eye.svg"
                            sourceSize.width: root.fontSize + 3
                            sourceSize.height: root.fontSize + 3
                            width: root.fontSize + 3
                            height: root.fontSize + 3
                            opacity: hdrCell.colapsada ? 0.55 : 1
                            HoverHandler {
                                id: ojoHover
                                onHoveredChanged: {
                                    if (ojoHover.hovered) {
                                        ojoTooltip.text = hdrCell.colapsada
                                            ? root.columnSummary(hdrCell.index) : "Colapsar columna"
                                        ojoTooltip.parent = btnOjo
                                        ojoTooltip.y = btnOjo.height
                                        ojoTooltip.visible = true
                                    } else if (ojoTooltip.parent === btnOjo) {
                                        ojoTooltip.visible = false
                                    }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.toggleColumn(hdrCell.index)
                            }
                        }
                        Text {
                            id: btnAplicar
                            visible: root.applyableColumns
                            anchors.verticalCenter: parent.verticalCenter
                            text: "↺"
                            color: "white"
                            font.bold: true
                            font.pixelSize: root.fontSize + 4
                            HoverHandler {
                                id: aplicarHover
                                onHoveredChanged: {
                                    if (aplicarHover.hovered) {
                                        ojoTooltip.text = "Aplicar este escenario a los datos actuales"
                                        ojoTooltip.parent = btnAplicar
                                        ojoTooltip.y = btnAplicar.height
                                        ojoTooltip.visible = true
                                    } else if (ojoTooltip.parent === btnAplicar) {
                                        ojoTooltip.visible = false
                                    }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.applyColumn(hdrCell.index)
                            }
                        }
                        Text {
                            id: btnCerrar
                            visible: root.closableColumns
                            anchors.verticalCenter: parent.verticalCenter
                            text: "✕"
                            color: "white"
                            font.bold: true
                            font.pixelSize: root.fontSize + 3
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.closeColumn(hdrCell.index)
                            }
                        }
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
                // Fila separadora de grupo: ocupa todo el ancho, sin celdas de valor
                // (p.ej. "Financiación" en la vista completa de Comparación).
                readonly property bool esSeparador: !!fila.modelData.separator
                // Fila perteneciente a un grupo (fondo propio, no alterna con las
                // filas normales) y, si es la última del grupo, línea divisoria abajo.
                readonly property bool esGrupo: !!fila.modelData.grupo
                readonly property bool finGrupo: !!fila.modelData.groupEnd

                Rectangle {
                    width: fila.esSeparador ? (root.wLabel + root.totalColumnsWidth()) : root.wLabel
                    height: root.hRow
                    color: fila.esSeparador ? "#dde9e2"
                         : fila.esGrupo ? "#eef5f1"
                         : fila.modelData.bold ? "#e3efe9"
                         : (fila.index % 2 ? "#f7faf8" : "white")
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left; anchors.leftMargin: 8
                        width: parent.width - 12
                        elide: Text.ElideRight
                        text: fila.modelData.label
                        font.pixelSize: root.fontSize
                        font.bold: fila.esSeparador || fila.modelData.bold
                        color: fila.esSeparador ? "#14523f"
                             : fila.modelData.bold ? "#14523f" : "#3c4a46"
                    }
                    Rectangle {
                        visible: fila.finGrupo
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 2
                        color: "#8fb3a3"
                    }
                }
                Repeater {
                    model: fila.esSeparador ? 0 : root.cols
                    Rectangle {
                        id: celda
                        required property int index
                        // Comprobación de límites: durante una actualización de "model" que
                    // cambia el número de columnas (p.ej. al cerrar un escenario en
                    // Comparación), el Repeater interior puede evaluarse un instante
                    // con un "index" que ya no existe en el "values" nuevo.
                    readonly property real valor: (fila.modelData.values && index < fila.modelData.values.length)
                                                   ? fila.modelData.values[index] : 0
                    readonly property bool colapsada: root.collapsibleColumns && root.isColumnCollapsed(celda.index)
                        width: root.columnWidth(celda.index); height: root.hRow
                        color: fila.esGrupo ? "#eef5f1"
                             : fila.modelData.bold ? "#e3efe9"
                             : (fila.index % 2 ? "#f7faf8" : "white")
                        Text {
                            visible: !celda.colapsada
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right; anchors.rightMargin: 8
                            text: Fmt.byFmt(celda.valor, fila.modelData.fmt)
                            font.pixelSize: root.fontSize
                            font.bold: fila.modelData.bold
                            color: celda.valor < 0 ? "#a33b2e"
                                 : fila.modelData.bold ? "#14523f" : "#1e2b28"
                        }
                        Rectangle {
                            visible: fila.finGrupo
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 2
                            color: "#8fb3a3"
                        }
                    }
                }
            }
        }
    }
}
