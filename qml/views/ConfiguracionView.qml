pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Configuración": escalas y series oficiales que usa el motor de
// cálculo (IRPF, RETA, Reales Decretos...). Editables, agrupadas por
// concepto, con los valores vigentes como valor por defecto ("Restaurar
// valores" en el panel lateral también las restaura). Las series del
// escenario Realista (aumento de facturación histórico y margen comercial)
// viven en Datos base, junto al combo de tipo de escenario.
Flickable {
    id: page

    readonly property bool angosto: width < 640

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Listas de claves por grupo, usadas por los botones "Restaurar valores
    // por defecto" (cada uno solo toca las claves de su propio grupo).
    function irpfKeys() {
        var ks = []
        for (var k = 0; k < 6; k++) { ks.push("irpfFrom" + k); ks.push("irpfTo" + k); ks.push("irpfRate" + k) }
        return ks
    }
    function savingsKeys() {
        var ks = []
        for (var k = 0; k < 5; k++) { ks.push("savingsFrom" + k); ks.push("savingsTo" + k); ks.push("savingsRate" + k) }
        return ks
    }
    function retaKeys() {
        var ks = ["retaFlatMonthlyFee"]
        for (var k = 0; k < 15; k++) { ks.push("retaFrom" + k); ks.push("retaQuota" + k) }
        return ks
    }
    function rdKeys() {
        var ks = []
        for (var k = 0; k < 9; k++) { ks.push("rdFrom" + k); ks.push("rdBase" + k); ks.push("rdPct" + k) }
        return ks
    }
    function fixedPctKeys() {
        return ["feesPct", "ivaPct", "itpPct", "ajdPct", "inventoryPctYear10"]
    }
    function salaryKeys() {
        return ["salaryRaisePct"]
    }

    // Cabecera de columnas de una tabla de tramos.
    component CabeceraTabla: Row {
        id: cabecera
        required property var etiquetas
        property int wCell: 148
        Repeater {
            model: cabecera.etiquetas
            Rectangle {
                id: hdrCell
                required property string modelData
                width: cabecera.wCell; height: 30; color: Tokens.bgBrandStrong
                Text {
                    anchors.centerIn: parent
                    text: hdrCell.modelData
                    color: Tokens.textOnDark; font.bold: true; font.pixelSize: 12
                }
            }
        }
    }

    // Celda de tabla con el fondo a rayas habitual; el campo editable va
    // dentro, como hijo directo, con "anchors.centerIn: parent" propio.
    component Celda: Rectangle {
        property bool par: true
        property int wCell: 148
        implicitWidth: wCell
        implicitHeight: 44
        color: par ? Tokens.bgTableRowPrimary : Tokens.bgTableRowAlt
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: page.width - 88
        spacing: 14

        Text {
            text: "Configuración"
            font.pixelSize: 22;
            font.bold: true;
            color: Tokens.textHeading
        }
        Text {
            text: "Escalas y series oficiales usadas por el simulador. Son editables, con los valores "
                + "vigentes como punto de partida; \"Restaurar valores\" (panel lateral) también las restaura."
            font.pixelSize: 13
            color: Tokens.textMuted
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        // ---------------- IRPF
        CollapsibleCard {
            title: "IRPF — escala general 2026"
            headerContent: ResetGroupButton { keys: page.irpfKeys(); compact: page.angosto }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Tramos de la base liquidable general, usados para calcular la cuota escala en la hoja Impuestos."
            }
            Flickable {
                id: scroll1
                Layout.fillWidth: true
                Layout.preferredHeight: tabla1.implicitHeight
                contentWidth: tabla1.implicitWidth
                contentHeight: tabla1.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll1; fallback: page }

                ColumnLayout {
                    id: tabla1
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Desde", "Hasta", "Tipo"] }
                    Repeater {
                        model: 6
                        Row {
                            id: filaIrpf
                            required property int index
                            Celda { par: filaIrpf.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "irpfFrom" + filaIrpf.index } }
                            Celda { par: filaIrpf.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "irpfTo" + filaIrpf.index } }
                            Celda { par: filaIrpf.index % 2 === 0; PctField   { anchors.centerIn: parent; k: "irpfRate"  + filaIrpf.index; decimals: 1 } }
                        }
                    }
                }
            }
        }

        // ---------------- IRPF base del ahorro
        CollapsibleCard {
            title: "IRPF — escala del ahorro 2026"
            headerContent: ResetGroupButton { keys: page.savingsKeys(); compact: page.angosto }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Tramos de la base liquidable del ahorro, usados en la hoja Venta para calcular los impuestos de la plusvalía al vender la farmacia."
            }
            Flickable {
                id: scrollAhorro
                Layout.fillWidth: true
                Layout.preferredHeight: tablaAhorro.implicitHeight
                contentWidth: tablaAhorro.implicitWidth
                contentHeight: tablaAhorro.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scrollAhorro; fallback: page }

                ColumnLayout {
                    id: tablaAhorro
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Desde", "Hasta", "Tipo"] }
                    Repeater {
                        model: 5
                        Row {
                            id: filaAhorro
                            required property int index
                            Celda { par: filaAhorro.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "savingsFrom" + filaAhorro.index } }
                            Celda { par: filaAhorro.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "savingsTo" + filaAhorro.index } }
                            Celda { par: filaAhorro.index % 2 === 0; PctField   { anchors.centerIn: parent; k: "savingsRate" + filaAhorro.index; decimals: 1 } }
                        }
                    }
                }
            }
        }

        // ---------------- RETA autónomos
        CollapsibleCard {
            title: "RETA — cuota de autónomos"
            headerContent: ResetGroupButton { keys: page.retaKeys(); compact: page.angosto }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Tarifa plana durante los primeros 12 meses de actividad. A partir del segundo año se "
                    + "aplica la escala por tramos de rendimientos netos mensuales."
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "Tarifa plana (mensual)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                MoneyField { k: "retaFlatMonthlyFee"; decimals: 2 }
            }
            Flickable {
                id: scroll2
                Layout.fillWidth: true
                Layout.preferredHeight: tabla2.implicitHeight
                contentWidth: tabla2.implicitWidth
                contentHeight: tabla2.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll2; fallback: page }

                ColumnLayout {
                    id: tabla2
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Rend. neto desde", "Cuota mensual"]; wCell: 160 }
                    Repeater {
                        model: 15
                        Row {
                            id: filaReta
                            required property int index
                            Celda { wCell: 160; par: filaReta.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "retaFrom" + filaReta.index; decimals: 2 } }
                            Celda { wCell: 160; par: filaReta.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "retaQuota" + filaReta.index; decimals: 2 } }
                        }
                    }
                }
            }
        }

        // ---------------- Reales Decretos
        CollapsibleCard {
            title: "Reales decretos (RD 823/2008, art. 2.5)"
            headerContent: ResetGroupButton { keys: page.rdKeys(); compact: page.angosto }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Escala de deducción sobre la facturación mensual de recetas (PVP+IVA) al SNS, usada en Datos base."
            }
            Flickable {
                id: scroll3
                Layout.fillWidth: true
                Layout.preferredHeight: tabla3.implicitHeight
                contentWidth: tabla3.implicitWidth
                contentHeight: tabla3.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                FastWheel { flick: scroll3; fallback: page }

                ColumnLayout {
                    id: tabla3
                    spacing: 0
                    CabeceraTabla { etiquetas: ["Facturación desde", "Base deducción", "% s/exceso"]; wCell: 160 }
                    Repeater {
                        model: 9
                        Row {
                            id: filaRd
                            required property int index
                            Celda { wCell: 160; par: filaRd.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "rdFrom" + filaRd.index } }
                            Celda { wCell: 160; par: filaRd.index % 2 === 0; MoneyField { anchors.centerIn: parent; k: "rdBase"  + filaRd.index; decimals: 2 } }
                            Celda { wCell: 160; par: filaRd.index % 2 === 0; PctField   { anchors.centerIn: parent; k: "rdPct"   + filaRd.index; decimals: 1 } }
                        }
                    }
                }
            }
        }

        // ---------------- Porcentajes fijos de la compraventa
        CollapsibleCard {
            title: "Compraventa de la farmacia — porcentajes fijos"
            headerContent: ResetGroupButton { keys: page.fixedPctKeys(); compact: page.angosto }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Usados en Financiación e Impuestos para calcular honorarios, impuestos de la compraventa y existencias estimadas a 10 años."
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "Honorarios (% s/ fondo de comercio + local)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "feesPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "IVA (% s/ honorarios)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "ivaPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "ITP (% s/ local comercial)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "itpPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "AJD (% s/ fondo de comercio + existencias)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "ajdPct"; decimals: 1 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "Existencias a 10 años (% s/ venta total año 10)"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "inventoryPctYear10"; decimals: 1 }
            }
        }

        // ---------------- Personal — IPC
        CollapsibleCard {
            title: "Personal — IPC"
            headerContent: ResetGroupButton { keys: page.salaryKeys(); compact: page.angosto }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Porcentaje fijo (IPC) con el que suben cada año los sueldos de la plantilla, los refuerzos de vacaciones, el alquiler del local y los Otros gastos en la Proyección a 10 años a partir del segundo año (el primer año usa los importes base), independiente del aumento de facturación del escenario de crecimiento."
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text { text: "IPC"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                PctField { k: "salaryRaisePct"; decimals: 1 }
            }
        }

        // ---------------- Carpeta de guardado de PDFs
        // No aplica en la versión web (los informes siempre se descargan a
        // través del navegador), así que la tarjeta entera se oculta ahí.
        CollapsibleCard {
            visible: Qt.platform.os !== "wasm"
            title: "Informes PDF — carpeta de guardado"
            headerContent: Button {
                    id: btnRestaurarCarpeta
                    text: "Restaurar (Documentos)"
                    font.pixelSize: 11
                    implicitHeight: 28
                    leftPadding: page.angosto ? 0 : 10
                    rightPadding: page.angosto ? 0 : 10
                    implicitWidth: page.angosto ? implicitHeight : implicitContentWidth + leftPadding + rightPadding
                    onClicked: Engine.setPdfSaveDir("")
                    background: Rectangle {
                        radius: 6
                        color: btnRestaurarCarpeta.down ? Tokens.bgAccentTint : Tokens.bgButtonGhostDefault
                        border.color: (btnRestaurarCarpeta.hovered || btnRestaurarCarpeta.down) ? Tokens.borderInteractiveHover : Tokens.borderButtonGhostDefault
                        border.width: 1
                    }
                    contentItem: Text {
                        text: page.angosto ? "↺" : btnRestaurarCarpeta.text
                        color: Tokens.textButtonGhost
                        font.family: btnRestaurarCarpeta.font.family
                        font.pixelSize: page.angosto ? 16 : btnRestaurarCarpeta.font.pixelSize
                        font.bold: page.angosto
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    ToolTip.visible: page.angosto && btnRestaurarCarpeta.hovered
                    ToolTip.text: btnRestaurarCarpeta.text
                    ToolTip.delay: 400
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: (mouse) => mouse.accepted = false
                    }
                }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Carpeta donde se guardan (y desde donde se abren) los informes PDF exportados desde Datos base, Comparación y Simulación. Déjala en blanco para usar la carpeta de Documentos del usuario."
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Button {
                    id: btnCarpetaPdf
                    Layout.fillWidth: true
                    implicitHeight: 40
                    font.pixelSize: 13
                    text: Engine.pdfSaveDir.length > 0 ? Engine.pdfSaveDir : Engine.pdfSaveDirDefault
                    onClicked: {
                        dlgCarpetaPdf.currentFolder = Engine.pdfSaveDirUrl()
                        dlgCarpetaPdf.open()
                    }
                    background: Rectangle {
                        radius: 5
                        color: Tokens.bgInput
                        border.color: (btnCarpetaPdf.hovered || btnCarpetaPdf.down) ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
                        border.width: 1
                    }
                    contentItem: Text {
                        text: btnCarpetaPdf.text
                        color: Tokens.textPrimary
                        font: btnCarpetaPdf.font
                        leftPadding: 10
                        rightPadding: 10
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: (mouse) => mouse.accepted = false
                    }
                }
                FolderDialog {
                    id: dlgCarpetaPdf
                    title: "Selecciona la carpeta donde guardar los PDFs"
                    onAccepted: Engine.setPdfSaveDir(selectedFolder)
                }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}
