pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FarmaciaSim

// Hoja "Personal": datos salariales y plantilla recomendada.
Flickable {
    id: page

    contentWidth: Math.max(width, col.implicitWidth)
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }

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
        width: Math.min(page.width - 88, 960)
        spacing: 14

        Text {
            text: "Personal"
            font.pixelSize: 22;
            font.bold: true;
            color: "#14523f"
            wrapMode: Text.WordWrap
        }

        // ---------------- Cuota de autónomo
        Card {
            SectionTitle { text: "CUOTA DE AUTÓNOMO (RETA)" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: "Cuota anual año 1 (según tramo de rendimientos netos)"
                    font.pixelSize: 13; color: "#3c4a46"; Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                Text {
                    text: Fmt.eur(Engine.datosBase.cuotaAutonomos)
                    font.pixelSize: 15; font.bold: true; color: "#14523f"
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Se calcula automáticamente cada año según la escala oficial de 15 tramos de rendimientos netos (no editable)."
            }

            ConceptTable {
                Layout.fillWidth: true
                Layout.topMargin: 8
                wLabel: 230
                wCell: 100
                hRow: 28
                fontSize: 12
                model: {
                    function rowByLabel(label) {
                        for (const r of Engine.proyeccion)
                            if (r.label === label) return r.values
                        return []
                    }
                    const cuota = rowByLabel("Cuota Autónomos (RETA)")
                    const beneficio = rowByLabel("BENEFICIO FARMACIA")
                    const beneficioPreCuota = beneficio.map((v, i) => v + cuota[i])
                    const cuotaMensual = cuota.map(v => v / 12.0)
                    return [
                        { label: "Beneficio de referencia (año)", values: beneficioPreCuota, fmt: "eur", bold: false },
                        { label: "Rendimiento neto mensual (tramo)", values: beneficioPreCuota.map(v => v / 12.0), fmt: "eur", bold: false },
                        { label: "CUOTA AUTÓNOMOS ANUAL", values: cuota, fmt: "eur", bold: true },
                        { label: "Cuota autónomos mensual", values: cuotaMensual, fmt: "eur", bold: false },
                    ]
                }
            }
        }

        // ---------------- Datos de personal
        Card {
            SectionTitle { text: "SALARIOS BASE DE PERSONAL" }

            Flickable {
                id: flickDatosPersonal
                Layout.fillWidth: true
                implicitHeight: datosPersonalCol.height
                contentWidth: datosPersonalCol.width
                contentHeight: datosPersonalCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                pressDelay: 150
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: datosPersonalCol
                    width: Math.max(implicitWidth, flickDatosPersonal.width)
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Tipo"; font.pixelSize: 12; font.bold: true; color: "#14523f"; Layout.preferredWidth: 160 }
                        CabeceraCol { text: "Sal. bruto anual (FT)" }
                        CabeceraCol { text: "Jornada" }
                        CabeceraCol { text: "% SS empresa" }
                        CabeceraCol { text: "Coste SS/año" }
                        CabeceraCol { text: "Sal. real pagado/año" }
                        CabeceraCol { text: "Coste total/año" }
                    }

                    // Farmacéutico
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Farmacéutico"; font.pixelSize: 13; color: "#3c4a46"; Layout.preferredWidth: 160 }
                        MoneyField { k: "salFarmaceutico"; Layout.preferredWidth: 108; implicitWidth: 108 }
                        PctField { k: "jornFarmaceutico"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                        Celda { text: Fmt.pct(Engine.inputs.pctSS, 0) }
                        Celda { text: Fmt.eur(Engine.personal.datos[0].costeSS) }
                        Celda { text: Fmt.eur(Engine.personal.datos[0].salReal) }
                        Celda { text: Fmt.eur(Engine.personal.datos[0].costeTotal) }
                    }
                    // Auxiliar
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Auxiliar de farmacia"; font.pixelSize: 13; color: "#3c4a46"; Layout.preferredWidth: 160 }
                        MoneyField { k: "salAuxiliar"; Layout.preferredWidth: 108; implicitWidth: 108 }
                        PctField { k: "jornAuxiliar"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                        Celda { text: Fmt.pct(Engine.inputs.pctSS, 0) }
                        Celda { text: Fmt.eur(Engine.personal.datos[1].costeSS) }
                        Celda { text: Fmt.eur(Engine.personal.datos[1].salReal) }
                        Celda { text: Fmt.eur(Engine.personal.datos[1].costeTotal) }
                    }
                    // Técnico
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Técnico"; font.pixelSize: 13; color: "#3c4a46"; Layout.preferredWidth: 160 }
                        MoneyField { k: "salTecnico"; Layout.preferredWidth: 108; implicitWidth: 108 }
                        PctField { k: "jornTecnico"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                        Celda { text: Fmt.pct(Engine.inputs.pctSS, 0) }
                        Celda { text: Fmt.eur(Engine.personal.datos[2].costeSS) }
                        Celda { text: Fmt.eur(Engine.personal.datos[2].salReal) }
                        Celda { text: Fmt.eur(Engine.personal.datos[2].costeTotal) }
                    }
                    // Total
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#dde5e1" }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "TOTAL (base cálculo)"; font.pixelSize: 13; font.bold: true; color: "#14523f"; Layout.preferredWidth: 160 }
                        Item { Layout.preferredWidth: 108 }
                        Item { Layout.preferredWidth: 108 }
                        Item { Layout.preferredWidth: 108 }
                        Celda { text: Fmt.eur(Engine.personal.totCosteSS); negrita: true }
                        Celda { text: Fmt.eur(Engine.personal.totSalReal); negrita: true }
                        Celda { text: Fmt.eur(Engine.personal.totCoste); negrita: true }
                    }
                }
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

        // ---------------- Subida salarial anual
        Card {
            SectionTitle { text: "SUBIDA SALARIAL ANUAL" }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: "% Subida s/ base"
                    font.pixelSize: 13
                    color: "#3c4a46"
                    Layout.fillWidth: true
                }
                PctField { k: "subidaPct"; Layout.preferredWidth: 108; implicitWidth: 108 }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#6b7a76"
                text: "Porcentaje de subida anual aplicado sobre el salario base de todo el personal."
            }
        }

        // ---------------- Plantilla recomendada
        Card {
            SectionTitle { text: "PLANTILLA" }

            Flickable {
                id: flickPlantilla
                Layout.fillWidth: true
                implicitHeight: plantillaCol.height
                contentWidth: plantillaCol.width
                contentHeight: plantillaCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                pressDelay: 150
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: plantillaCol
                    width: Math.max(implicitWidth, flickPlantilla.width)
                    spacing: 8

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
                            Text {
                                visible: filaPl.index === 0
                                text: "1"
                                Layout.preferredWidth: 80
                                horizontalAlignment: Text.AlignRight
                                font.pixelSize: 13
                                color: "#1e2b28"
                            }
                            NumField {
                                visible: filaPl.index !== 0
                                k: "plPersonas" + filaPl.index
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                            }
                            Celda { text: Fmt.eur(filaPl.r.brutoFT) }
                            Celda { text: Fmt.eur(filaPl.r.brutoReal) }
                            Celda { text: Fmt.eur(filaPl.r.costeSS) }
                            Celda { text: Fmt.eur(filaPl.r.costeTotal) }
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
                        Item { Layout.preferredWidth: 150 }
                    }
                }
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
                    color: "white";
                    font.pixelSize: 15;
                    font.bold: true
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
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
