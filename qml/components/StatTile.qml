import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Tarjeta KPI compacta para el Dashboard: número grande + etiqueta debajo.
// Mismo lenguaje visual que Card.qml (fondo/borde), pero sin el padding
// generoso pensado para contener secciones enteras.
Rectangle {
    id: root

    property string label: ""
    property real value: 0
    property string fmt: "eur"
    property bool negativoEnRojo: true

    Layout.fillWidth: true
    Layout.minimumWidth: 150
    radius: 12
    color: Tokens.bgSurface
    border.color: Tokens.borderSurface
    implicitHeight: col.implicitHeight + 28

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: 16
        spacing: 4

        Text {
            Layout.fillWidth: true
            text: root.fmt === "years" && root.value < 0
                ? "> 10 años"
                : Fmt.byFmt(root.value, root.fmt)
            font.pixelSize: 22
            font.bold: true
            color: root.negativoEnRojo && root.value < 0 ? Tokens.textNegative : Tokens.textHeading
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            text: root.label
            font.pixelSize: 12
            color: Tokens.textSecondary
            wrapMode: Text.WordWrap
        }
    }
}
