import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Panel blanco estándar (borde suave, esquinas redondeadas) usado como
// contenedor de sección en la mayoría de las hojas.
Rectangle {
    default property alias content: box.data
    Layout.fillWidth: true
    radius: 12
    color: Tokens.bgSurface
    border.color: Tokens.borderSurface
    implicitHeight: box.implicitHeight + 40

    ColumnLayout {
        id: box
        anchors.fill: parent
        anchors.margins: 20
        spacing: 8
    }
}
