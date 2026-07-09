pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Datos Base": PyG estimada para el estudio.
Flickable {
    id: page

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }

    // Fila calculada (solo lectura)
    component CalcRow: RowLayout {
        property string label
        property real value
        property bool destacada: false
        Layout.fillWidth: true
        Text {
            text: label
            font.pixelSize: 13
            font.bold: destacada
            color: destacada ? "#14523f" : "#3c4a46"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: Fmt.eur(value)
            font.pixelSize: 14
            font.bold: true
            color: destacada ? "#14523f" : "#1e2b28"
        }
    }

    // Fila editable
    component EditRow: RowLayout {
        property string label
        property string k
        Layout.fillWidth: true
        Text { text: label; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        MoneyField { k: parent.k }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 24
        anchors.topMargin: 24
        width: Math.min(page.width - 48, 760)
        spacing: 14

        Text { text: "Datos base del estudio"; font.pixelSize: 22; font.bold: true; color: "#14523f" }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: "white"
            border.color: "#dde5e1"
            implicitHeight: inner.implicitHeight + 40

            ColumnLayout {
                id: inner
                anchors.fill: parent
                anchors.margins: 20
                spacing: 8

                SectionTitle { text: "PyG ESTIMADA PARA EL ESTUDIO" }

                EditRow { label: "VENTA RECETA";  k: "ventaReceta" }
                EditRow { label: "VENTA LIBRE";   k: "ventaLibre" }
                CalcRow { label: "VENTA TOTAL";   value: Engine.datosBase.ventaTotal; destacada: true }
                CalcRow { label: "COSTE MERCANCÍA"; value: Engine.datosBase.costeMercancia }
                CalcRow { label: "M. COMERCIAL BRUTO"; value: Engine.datosBase.mComBruto }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "M. COMERCIAL BRUTO %"; font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    PctField { k: "margenPct" }
                }
                EditRow { label: "REALES DECRETOS"; k: "realesDecretos" }
                CalcRow { label: "M. COMERCIAL DESPUÉS RDS"; value: Engine.datosBase.mComDespuesRD; destacada: true }

                Item { Layout.preferredHeight: 6 }
                CalcRow { label: "GASTOS DE PERSONAL  (hoja Personal)"; value: Engine.datosBase.gastosPersonal }
                CalcRow { label: "SEGURIDAD SOCIAL  (hoja Personal)"; value: Engine.datosBase.seguridadSocial }
                EditRow { label: "CUOTA AUTÓNOMOS"; k: "cuotaAutonomos" }
                CalcRow { label: "TOTAL GASTOS PERSONAL"; value: Engine.datosBase.totalGastosPersonal; destacada: true }

                Item { Layout.preferredHeight: 6 }
                EditRow { label: "ALQUILER LOCAL"; k: "alquilerLocal" }
                EditRow { label: "SUMINISTROS"; k: "suministros" }
                EditRow { label: "GASTOS ASESORÍA"; k: "asesoria" }
                EditRow { label: "MANTENIMIENTO INFORMÁTICO"; k: "mantenimiento" }
                EditRow { label: "ROBOT"; k: "robot" }
                EditRow { label: "SEGUROS"; k: "seguros" }
                EditRow { label: "OTROS GASTOS"; k: "otrosGastos" }
                CalcRow { label: "TOTAL OTROS GASTOS"; value: Engine.datosBase.totalOtrosGastos; destacada: true }
            }
        }

        // Resultado final destacado
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
                    text: "Bº ANTES DE IMP. Y AMORT."
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                    Layout.fillWidth: true
                }
                Text {
                    text: Fmt.eur(Engine.datosBase.beneficioAntesImp)
                    color: "#ffe9a8"
                    font.pixelSize: 20
                    font.bold: true
                }
            }
        }
    }
}
