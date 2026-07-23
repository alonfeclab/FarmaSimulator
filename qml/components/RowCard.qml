import QtQuick
import QtQuick.Layouts
import FarmaciaSim

// Caja con fondo y borde suave que envuelve una fila de "etiqueta: valor"
// (EditRow, CalcRow, PctRow, PlazoRow...) para que ambos queden unidos
// visualmente aunque haya mucho hueco entre ellos en pantallas anchas.
// Único sitio donde vive este estilo: las filas de cada hoja lo extienden
// en vez de duplicar el Rectangle + RowLayout.
// Por debajo de wApilar, la etiqueta y el valor se apilan en vertical en
// vez de compartir una fila demasiado estrecha (texto envuelto, campo
// comprimido).
Rectangle {
    id: root

    default property alias content: rowContent.data
    property bool destacada: false
    property int wApilar: 300
    readonly property bool angosto: width < wApilar

    Layout.fillWidth: true
    radius: 8
    color: destacada ? Tokens.bgRowCardEmphasis : Tokens.bgRowCardDefault
    border.color: destacada ? Tokens.borderRowCardEmphasis : Tokens.borderRowCardDefault
    border.width: 1
    implicitHeight: rowContent.implicitHeight + 12

    GridLayout {
        id: rowContent
        anchors.fill: parent
        anchors.margins: 6
        columns: root.angosto ? 1 : 2
        rowSpacing: 4
        columnSpacing: 8
    }
}
