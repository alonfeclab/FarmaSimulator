pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Proyección 10 Años": tabla de 20 conceptos x 10 años.
Item {
    id: page

    readonly property int wLabel: 250
    readonly property int wCell: 108
    readonly property int hRow: 30

    function resetScroll() { flick.contentX = 0; flick.contentY = 0 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text { text: "Proyección a 10 años"; font.pixelSize: 22; font.bold: true; color: Tokens.textHeading }

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
                model: Engine.projection
                ScrollBar.vertical: ScrollBar {}
            }
        }
    }
}
