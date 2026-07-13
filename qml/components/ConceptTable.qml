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
                    width: root.wCell; height: root.hRow; color: "#14523f"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: root.closableColumns ? 8 : 0
                        anchors.right: root.closableColumns ? btnCerrar.left : parent.right
                        anchors.rightMargin: root.closableColumns ? 4 : 0
                        horizontalAlignment: root.closableColumns ? Text.AlignLeft : Text.AlignHCenter
                        elide: Text.ElideRight
                        text: root.headerLabels.length > hdrCell.index
                              ? root.headerLabels[hdrCell.index]
                              : ("Año " + (hdrCell.index + 1))
                        color: "white"; font.bold: true; font.pixelSize: root.fontSize
                    }
                    Text {
                        id: btnCerrar
                        visible: root.closableColumns
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: "✕"
                        color: "white"
                        font.bold: true
                        font.pixelSize: root.fontSize
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -6
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.closeColumn(hdrCell.index)
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
                // (p.ej. "FINANCIACIÓN" en la vista completa de Comparación).
                readonly property bool esSeparador: !!fila.modelData.separator
                // Fila perteneciente a un grupo (fondo propio, no alterna con las
                // filas normales) y, si es la última del grupo, línea divisoria abajo.
                readonly property bool esGrupo: !!fila.modelData.grupo
                readonly property bool finGrupo: !!fila.modelData.groupEnd

                Rectangle {
                    width: fila.esSeparador ? (root.wLabel + root.cols * root.wCell) : root.wLabel
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
                        readonly property real valor: fila.modelData.values[index]
                        width: root.wCell; height: root.hRow
                        color: fila.esGrupo ? "#eef5f1"
                             : fila.modelData.bold ? "#e3efe9"
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
