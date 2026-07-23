import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Título de sección con barra de acento, estilo cabecera de la hoja Excel.
ColumnLayout {
    property alias text: label.text

    Layout.fillWidth: true
    spacing: 4

    Text {
        id: label
        Layout.fillWidth: true
        font.pixelSize: 15
        font.bold: true
        color: Tokens.textHeading
        wrapMode: Text.WordWrap
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 2
        radius: 1
        color: Tokens.borderAccent
        opacity: 0.35
    }
}
