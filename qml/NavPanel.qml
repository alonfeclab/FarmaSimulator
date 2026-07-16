pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Panel de navegación lateral: usado como sidebar fija (pantallas anchas)
// o como contenido de un Drawer deslizante (pantallas estrechas).
Rectangle {
    id: root

    required property var hojas
    property int currentIndex: 0
    property bool compact: false

    signal navigate(int index)

    color: "#123f31"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4

        Text {
            text: "SIMULACIÓN\nFARMACIA"
            color: "white"
            font.pixelSize: 19
            font.bold: true
            lineHeight: 1.1
            Layout.margins: 8
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#2b6a52" }
        Item { Layout.preferredHeight: 6 }

        // Lista de pestañas con scroll propio: en móvil, con muchas pestañas,
        // no debe empujar fuera de la pantalla los botones de abajo (PDF,
        // restaurar valores); si no caben todas, se hace scroll solo aquí.
        Flickable {
            id: navScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: navCol.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            FastWheel { flick: navScroll }

            ColumnLayout {
                id: navCol
                width: navScroll.width
                spacing: 4

                Repeater {
                    model: root.hojas
                    delegate: Rectangle {
                        id: navItem
                        required property int index
                        required property var modelData

                        objectName: "navItem" + index
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.compact ? 50 : 42
                        radius: 8
                        color: root.currentIndex === index ? "#1a7a5e"
                             : mouse.containsMouse ? "#1a5a45" : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            spacing: 10
                            Image {
                                source: navItem.modelData.icono
                                sourceSize: Qt.size(18, 18)
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                            }
                            Text {
                                text: navItem.modelData.nombre
                                color: "white"
                                font.pixelSize: 14
                                font.bold: root.currentIndex === navItem.index
                                Layout.fillWidth: true
                            }
                        }
                        MouseArea {
                            id: mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.navigate(navItem.index)
                        }
                    }
                }
            }
        }

        Button {
            id: btnPdf
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.topMargin: 46
            text: "Exportar informe PDF"
            font.pixelSize: 12
            font.bold: true
            onClicked: {
                const destino = Engine.exportPdf()
                avisoPdf.mostrar(destino.length > 0
                    ? "Informe guardado en:\n" + destino
                    : "No se pudo crear el PDF")
            }
            background: Rectangle {
                radius: 8
                color: btnPdf.down ? "#0f5a43" : "#1a7a5e"
                border.color: "#2b8a6a"
            }
            contentItem: Text {
                text: btnPdf.text
                color: "#ffe9a8"
                font: btnPdf.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onPressed: mouse.accepted = false
            }
        }
        Text {
            id: avisoPdf
            visible: false
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
            color: "#cfe8de"
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter

            function mostrar(mensaje) {
                text = mensaje
                visible = true
                temporizadorAviso.restart()
            }
            Timer {
                id: temporizadorAviso
                interval: 6000
                onTriggered: avisoPdf.visible = false
            }
        }

        Button {
            id: btnRestaurar
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            text: "Restaurar valores"
            font.pixelSize: 12
            onClicked: Engine.resetToDefaults()
            background: Rectangle {
                radius: 8
                color: btnRestaurar.down ? "#0e3226" : "#1a5a45"
                border.color: "#2b6a52"
            }
            contentItem: Text {
                text: btnRestaurar.text
                color: "#cfe8de"
                font: btnRestaurar.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onPressed: mouse.accepted = false
            }
        }
        Text {
            text: "Los campos amarillos son editables"
            color: "#7fae9c"
            font.pixelSize: 11
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            text: "Cambios guardados automáticamente"
            color: "#7fae9c"
            font.pixelSize: 10
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 4

            ToolTip.visible: zonaTip.containsMouse
            ToolTip.delay: 300
            ToolTip.text: "Se guardan en:\n" + Engine.dataPath
            MouseArea { id: zonaTip; anchors.fill: parent; hoverEnabled: true }
        }
    }
}
