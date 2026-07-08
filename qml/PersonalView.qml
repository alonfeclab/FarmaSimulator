pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Personal": datos salariales y plantilla recomendada.
Flickable {
    id: page

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

    component Celda: Text {
        property bool negrita: false
        Layout.preferredWidth: 108
        horizontalAlignment: Text.AlignRight
        font.pixelSize: 13
        font.bold: negrita
        color: negrita ? "#14523f" : "#1e2b28"
    }

    component CabeceraCol: Text {
        Layout.preferredWidth: 108
        horizontalAlignment: Text.AlignRight
        font.pixelSize: 12
        font.bold: true
        color: "#14523f"
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 960)
        spacing: 14

        Text {
            text: "Personal recomendado (propietario farmacéutico + empleados)"
            font.pixelSize: 22; font.bold: true; color: "#14523f"
        }

        // ---------------- 1. Datos de personal
        Card {
            SectionTitle { text: "1. DATOS DE PERSONAL — salarios y condiciones" }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "Tipo"; font.pixelSize: 12; font.bold: true; color: "#14523f"; Layout.fillWidth: true }
                CabeceraCol { text: "Sal. bruto anual (FT)" }
                CabeceraCol { text: "Jornada" }
                CabeceraCol { text: "% SS empresa" }
                CabeceraCol { text: "Coste SS/año" }
                CabeceraCol { text: "Sal. real pagado/año" }
                CabeceraCol { text: "Coste total/año" }
                CabeceraCol { text: "% Subida s/ base" }
            }

            // Farmacéutico
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "Farmacéutico"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                MoneyField { k: "salFarmaceutico"; Layout.preferredWidth: 108; implicitWidth: 108 }
                PctField { k: "jornFarmaceutico"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                PctField { k: "pctSS"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                Celda { text: Fmt.eur(Engine.personal.datos[0].costeSS) }
                Celda { text: Fmt.eur(Engine.personal.datos[0].salReal) }
                Celda { text: Fmt.eur(Engine.personal.datos[0].costeTotal) }
                PctField { k: "subidaPct"; Layout.preferredWidth: 108; implicitWidth: 108 }
            }
            // Auxiliar
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "Auxiliar de farmacia"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                MoneyField { k: "salAuxiliar"; Layout.preferredWidth: 108; implicitWidth: 108 }
                PctField { k: "jornAuxiliar"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                Celda { text: Fmt.pct(Engine.inputs.pctSS, 0) }
                Celda { text: Fmt.eur(Engine.personal.datos[1].costeSS) }
                Celda { text: Fmt.eur(Engine.personal.datos[1].salReal) }
                Celda { text: Fmt.eur(Engine.personal.datos[1].costeTotal) }
                Item { Layout.preferredWidth: 108 }
            }
            // Técnico
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "Técnico"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true }
                MoneyField { k: "salTecnico"; Layout.preferredWidth: 108; implicitWidth: 108 }
                PctField { k: "jornTecnico"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                Celda { text: Fmt.pct(Engine.inputs.pctSS, 0) }
                Celda { text: Fmt.eur(Engine.personal.datos[2].costeSS) }
                Celda { text: Fmt.eur(Engine.personal.datos[2].salReal) }
                Celda { text: Fmt.eur(Engine.personal.datos[2].costeTotal) }
                Item { Layout.preferredWidth: 108 }
            }
            // Total
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#dde5e1" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "TOTAL (base cálculo)"; font.pixelSize: 13; font.bold: true; color: "#14523f"; Layout.fillWidth: true }
                Item { Layout.preferredWidth: 108 }
                Item { Layout.preferredWidth: 108 }
                Item { Layout.preferredWidth: 108 }
                Celda { text: Fmt.eur(Engine.personal.totCosteSS); negrita: true }
                Celda { text: Fmt.eur(Engine.personal.totSalReal); negrita: true }
                Celda { text: Fmt.eur(Engine.personal.totCoste); negrita: true }
                Item { Layout.preferredWidth: 108 }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Salario bruto a jornada completa; la jornada reduce el coste. El % SS empresa es igual "
                    + "para todos. Cambiar aquí actualiza Datos Base y la Proyección."
            }
        }

        // ---------------- 2. Plantilla recomendada
        Card {
            SectionTitle { text: "2. PLANTILLA RECOMENDADA" }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "Tipo de trabajador"; font.pixelSize: 12; font.bold: true; color: "#14523f"; Layout.preferredWidth: 170 }
                CabeceraCol { text: "Jornada"; Layout.preferredWidth: 80 }
                CabeceraCol { text: "Nº personas"; Layout.preferredWidth: 80 }
                CabeceraCol { text: "Sal. bruto FT/año" }
                CabeceraCol { text: "Sal. bruto real/año" }
                CabeceraCol { text: "Coste SS/año" }
                CabeceraCol { text: "Coste total tipo" }
                Text { text: "Rol / turno"; font.pixelSize: 12; font.bold: true; color: "#14523f"; Layout.fillWidth: true }
            }

            Repeater {
                model: 4
                RowLayout {
                    id: filaPl
                    required property int index
                    readonly property var r: Engine.personal.plantilla[index]
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: filaPl.r.tipo; font.pixelSize: 13; color: "#3c4a46"; Layout.preferredWidth: 170; wrapMode: Text.WordWrap }
                    PctField { k: "plJornada" + filaPl.index; decimals: 0; Layout.preferredWidth: 80; implicitWidth: 80 }
                    NumField { k: "plPersonas" + filaPl.index; Layout.preferredWidth: 80; implicitWidth: 80 }
                    Celda { text: Fmt.eur(filaPl.r.brutoFT) }
                    Celda { text: Fmt.eur(filaPl.r.brutoReal) }
                    Celda { text: Fmt.eur(filaPl.r.costeSS) }
                    Celda { text: Fmt.eur(filaPl.r.costeTotal) }
                    Text { text: filaPl.r.rol; font.pixelSize: 11; color: "#6b7a76"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#dde5e1" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "TOTAL PLANTILLA (coste empresa/año)"; font.pixelSize: 13; font.bold: true; color: "#14523f"; Layout.preferredWidth: 170; wrapMode: Text.WordWrap }
                Item { Layout.preferredWidth: 80 }
                Celda { text: Fmt.num(Engine.personal.totPersonas) + " pers."; negrita: true; Layout.preferredWidth: 80 }
                Item { Layout.preferredWidth: 108 }
                Celda { text: Fmt.eur(Engine.personal.totBrutoReal); negrita: true }
                Celda { text: Fmt.eur(Engine.personal.totSS); negrita: true }
                Celda { text: Fmt.eur(Engine.personal.totPlantilla); negrita: true }
                Item { Layout.fillWidth: true }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "El propietario cubre la presencia legal; coste 0 € (se retribuye vía beneficio)."
            }
        }

        // ---------------- salario neto titular año 1
        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: "#14523f"
            implicitHeight: 64

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                Text {
                    text: "SALARIO NETO MENSUAL TITULAR (Año 1)"
                    color: "white"; font.pixelSize: 15; font.bold: true
                    Layout.fillWidth: true
                }
                Text {
                    text: Fmt.eur(Engine.personal.salarioNetoMensualAnio1)
                    color: "#ffe9a8"; font.pixelSize: 20; font.bold: true
                }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
