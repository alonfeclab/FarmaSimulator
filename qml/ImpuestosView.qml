pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Impuestos" (v2): cálculo del pago de IRPF por tramos.
Flickable {
    id: page

    readonly property int wLabel: 230
    readonly property int wCell: 100
    readonly property int hRow: 28
    readonly property bool angosto: width < 640

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

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

    component CalcRow: RowLayout {
        property string label
        property real value
        property bool destacada: false
        Layout.fillWidth: true
        Text {
            text: label; font.pixelSize: 13; font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"; Layout.fillWidth: true
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
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 1360)
        spacing: 14

        Text {
            text: "Cálculo del pago de impuestos (IRPF)"
            font.pixelSize: 22; font.bold: true; color: "#14523f"
        }

        // ---------------- Paso 1: parámetros y base amortizable
        Card {
            SectionTitle { text: "PASO 1: PARÁMETROS Y BASE AMORTIZABLE" }

            GridLayout {
                columns: page.angosto ? 1 : 2
                columnSpacing: 40
                rowSpacing: 8
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    CalcRow { label: "Fondo de comercio farmacia"; value: Engine.impuestos.fdc }
                    CalcRow { label: "Honorarios"; value: Engine.impuestos.honorarios }
                    CalcRow { label: "AJD (1,5% s/ F. Comercio + Existencias)"; value: Engine.impuestos.ajd }
                    CalcRow { label: "BASE AMORTIZABLE FARMACIA"; value: Engine.impuestos.baseAmortizable; destacada: true }
                    CalcRow { label: "Coste local comercial"; value: Engine.impuestos.costeLocal }
                }
                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "% amortización local (fijo)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                        PctField { k: "impAmortLocalPct" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "% amortización farmacia (mínimo)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                        PctField { k: "impAmortMinPct" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "% amortización farmacia (máximo)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                        PctField { k: "impAmortMaxPct" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Mínimo personal exento (IRPF)"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                        MoneyField { k: "minimoPersonal" }
                    }
                    CalcRow { label: "Deducción mínimo personal (× 19%)"; value: Engine.impuestos.deduccionMinimo }
                }
            }
        }

        // ---------------- Pasos 2 y 3: tabla a 10 años
        Card {
            SectionTitle { text: "PASOS 2 Y 3: AMORTIZACIONES, BASE IMPONIBLE E IRPF POR TRAMOS (ESCALA 2026)" }

            Flickable {
                Layout.fillWidth: true
                Layout.topMargin: 8
                implicitHeight: tabla.implicitHeight
                contentWidth: page.wLabel + 10 * page.wCell
                clip: true
                ScrollBar.horizontal: ScrollBar {}

                Column {
                    id: tabla

                    Row {
                        Rectangle {
                            width: page.wLabel; height: page.hRow; color: "#14523f"; radius: 4
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 8
                                text: "Concepto"; color: "white"; font.bold: true; font.pixelSize: 12
                            }
                        }
                        Repeater {
                            model: 10
                            Rectangle {
                                required property int index
                                width: page.wCell; height: page.hRow; color: "#14523f"
                                Text {
                                    anchors.centerIn: parent
                                    text: "Año " + (parent.index + 1)
                                    color: "white"; font.bold: true; font.pixelSize: 12
                                }
                            }
                        }
                    }

                    Repeater {
                        model: {
                            const I = Engine.impuestos
                            let filas = [
                                { label: "Beneficio farmacia", values: I.beneficio, fmt: "eur", bold: false },
                                { label: "Amortización local", values: I.amortLocal, fmt: "eur", bold: false },
                                { label: "% amort. farmacia ajustado", values: I.pctAjustado, fmt: "pct2", bold: false },
                                { label: "Amortización F.C. farmacia", values: I.amortFdC, fmt: "eur", bold: false },
                                { label: "BASE IMPONIBLE", values: I.baseImponible, fmt: "eur", bold: true }
                            ]
                            for (const t of I.tramos)
                                filas.push({ label: t.label, values: t.values, fmt: "eur", bold: false })
                            filas.push({ label: "Cuota escala (suma de tramos)", values: I.cuotaEscala, fmt: "eur", bold: true })
                            filas.push({ label: "PAGO IMPUESTOS", values: I.pago, fmt: "eur", bold: true })
                            return filas
                        }
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
                                    text: fila.modelData.label
                                    font.pixelSize: 12
                                    font.bold: fila.modelData.bold
                                    color: fila.modelData.bold ? "#14523f" : "#3c4a46"
                                    elide: Text.ElideRight
                                    width: parent.width - 12
                                }
                            }
                            Repeater {
                                model: fila.modelData.values
                                Rectangle {
                                    id: celda
                                    required property var modelData
                                    width: page.wCell; height: page.hRow
                                    color: fila.modelData.bold ? "#e3efe9"
                                         : (fila.index % 2 ? "#f7faf8" : "white")
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.right: parent.right; anchors.rightMargin: 8
                                        text: Fmt.byFmt(celda.modelData, fila.modelData.fmt)
                                        font.pixelSize: 12
                                        font.bold: fila.modelData.bold
                                        color: fila.modelData.bold ? "#14523f" : "#1e2b28"
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "El % de amortización de la farmacia se ajusta cada año (entre el mínimo y el máximo) para "
                    + "dejar la base imponible en 0 mientras sea posible. El pago alimenta la fila "
                    + "\"Pago Impuestos\" de la Proyección a 10 años."
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
