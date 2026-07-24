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
    FastWheel { flick: page }

    component Celda: Text {
        property bool negrita: false
        Layout.preferredWidth: 108
        horizontalAlignment: Text.AlignRight
        font.pixelSize: 13
        font.bold: negrita
        color: negrita ? Tokens.textHeading : Tokens.textPrimary
    }

    component CabeceraCol: Text {
        Layout.preferredWidth: 108
        horizontalAlignment: Text.AlignRight
        font.pixelSize: 12
        font.bold: true
        color: Tokens.textHeading
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: page.width - 88
        spacing: 14

        Text {
            text: "Personal"
            font.pixelSize: 22;
            font.bold: true;
            color: Tokens.textHeading
            wrapMode: Text.WordWrap
        }

        // ---------------- Cuota de autónomo
        Card {
            SectionTitle { text: "Cuota de autónomo (RETA)" }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Se calcula automáticamente cada año según la escala oficial de 15 tramos de rendimientos netos (no editable)."
            }

            ConceptTable {
                Layout.fillWidth: true
                Layout.topMargin: 8
                wLabel: 230
                wCell: 100
                hRow: 28
                fontSize: 12
                // La página (Flickable) ya lleva el scroll vertical.
                flickableDirection: Flickable.HorizontalFlick
                fallback: page
                model: {
                    function rowByLabel(label) {
                        for (const r of Engine.projection)
                            if (r.label === label) return r.values
                        return []
                    }
                    const cuota = rowByLabel("Cuota autónomos")
                    const beneficio = rowByLabel("Beneficio farmacia")
                    const beneficioPreCuota = beneficio.map((v, i) => v + cuota[i])
                    const cuotaMensual = cuota.map(v => v / 12.0)
                    return [
                        { label: "Beneficio de referencia", values: beneficioPreCuota, fmt: "eur", bold: false },
                        { label: "Rendimiento neto mensual (tramo)", values: beneficioPreCuota.map(v => v / 12.0), fmt: "eur", bold: false },
                        { label: "Cuota autónomos anual", values: cuota, fmt: "eur", bold: true },
                        { label: "Cuota autónomos mensual", values: cuotaMensual, fmt: "eur", bold: false },
                    ]
                }
            }
        }

        // ---------------- Datos de personal
        Card {
            SectionTitle { text: "Salarios base de personal" }

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

                FastWheel { flick: flickDatosPersonal; fallback: page }

                ColumnLayout {
                    id: datosPersonalCol
                    width: Math.max(implicitWidth, flickDatosPersonal.width)
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Tipo"; font.pixelSize: 12; font.bold: true; color: Tokens.textHeading; Layout.preferredWidth: 160 }
                        CabeceraCol { text: "Sal. bruto anual (FT)" }
                        CabeceraCol { text: "% SS empresa" }
                        CabeceraCol { text: "Coste SS/año" }
                        CabeceraCol { text: "Sal. real pagado/año" }
                        CabeceraCol { text: "Coste total/año" }
                    }

                    // Farmacéutico
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Farmacéutico"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.preferredWidth: 160 }
                        MoneyField { k: "pharmacistSalary"; Layout.preferredWidth: 108; implicitWidth: 108 }
                        PctField { k: "socialSecurityPct"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                        Celda { text: Fmt.eur(Engine.staff.byRole[0].socialSecurityCost) }
                        Celda { text: Fmt.eur(Engine.staff.byRole[0].actualSalary) }
                        Celda { text: Fmt.eur(Engine.staff.byRole[0].totalCost) }
                    }
                    // Técnico
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Técnico"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.preferredWidth: 160 }
                        MoneyField { k: "technicianSalary"; Layout.preferredWidth: 108; implicitWidth: 108 }
                        PctField { k: "socialSecurityPct"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                        Celda { text: Fmt.eur(Engine.staff.byRole[2].socialSecurityCost) }
                        Celda { text: Fmt.eur(Engine.staff.byRole[2].actualSalary) }
                        Celda { text: Fmt.eur(Engine.staff.byRole[2].totalCost) }
                    }
                    // Auxiliar
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Auxiliar de farmacia"; font.pixelSize: 13; color: Tokens.textSecondary; Layout.preferredWidth: 160 }
                        MoneyField { k: "assistantSalary"; Layout.preferredWidth: 108; implicitWidth: 108 }
                        PctField { k: "socialSecurityPct"; decimals: 0; Layout.preferredWidth: 108; implicitWidth: 108 }
                        Celda { text: Fmt.eur(Engine.staff.byRole[1].socialSecurityCost) }
                        Celda { text: Fmt.eur(Engine.staff.byRole[1].actualSalary) }
                        Celda { text: Fmt.eur(Engine.staff.byRole[1].totalCost) }
                    }
                    // Total
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Tokens.borderDivider }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Total"; font.pixelSize: 13; font.bold: true; color: Tokens.textHeading; Layout.preferredWidth: 160 }
                        Item { Layout.preferredWidth: 108 }
                        Item { Layout.preferredWidth: 108 }
                        Celda { text: Fmt.eur(Engine.staff.totalSocialSecurityCost); negrita: true }
                        Celda { text: Fmt.eur(Engine.staff.totalActualSalary); negrita: true }
                        Celda { text: Fmt.eur(Engine.staff.totalCost); negrita: true }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Salario bruto a jornada completa. El % SS empresa es igual para todos. "
                    + "Cambiar aquí actualiza Datos base y la Proyección."
            }
        }

        // ---------------- Plantilla recomendada
        Card {
            SectionTitle { text: "Plantilla" }

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

                FastWheel { flick: flickPlantilla; fallback: page }

                ColumnLayout {
                    id: plantillaCol
                    width: Math.max(implicitWidth, flickPlantilla.width)
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Tipo de trabajador"; font.pixelSize: 12; font.bold: true; color: Tokens.textHeading; Layout.preferredWidth: 170 }
                        CabeceraCol { text: "Jornada (h)"; Layout.preferredWidth: 80 }
                        CabeceraCol { text: "Nº personas"; Layout.preferredWidth: 80 }
                        CabeceraCol { text: "Subida %"; Layout.preferredWidth: 80 }
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
                            readonly property var r: Engine.staff.headcountPlan[index]
                            Layout.fillWidth: true
                            spacing: 8
                            Text { text: filaPl.r.role; font.pixelSize: 13; color: Tokens.textSecondary; Layout.preferredWidth: 170; wrapMode: Text.WordWrap }
                            Button {
                                id: btnJornada
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                                implicitHeight: 40
                                text: Fmt.num(filaPl.r.fte * 8, 1) + " h"
                                font.pixelSize: 13
                                onClicked: dlgJornada.abrir(filaPl.index, filaPl.r.role)
                                background: Rectangle {
                                    radius: 5
                                    color: Tokens.bgInput
                                    border.color: (btnJornada.hovered || btnJornada.down) ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
                                    border.width: 1
                                }
                                contentItem: Text {
                                    text: btnJornada.text
                                    color: Tokens.textPrimary
                                    font: btnJornada.font
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            Text {
                                visible: filaPl.index === 0
                                text: "1"
                                Layout.preferredWidth: 80
                                horizontalAlignment: Text.AlignRight
                                font.pixelSize: 13
                                color: Tokens.textPrimary
                            }
                            NumField {
                                visible: filaPl.index !== 0
                                k: "staffCount" + filaPl.index
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                            }
                            Text {
                                visible: filaPl.index === 0
                                text: "—"
                                Layout.preferredWidth: 80
                                horizontalAlignment: Text.AlignRight
                                font.pixelSize: 13
                                color: Tokens.textPrimary
                            }
                            PctField {
                                visible: filaPl.index !== 0
                                k: "raisePct" + filaPl.index
                                decimals: 0
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                            }
                            Celda { text: Fmt.eur(filaPl.r.grossFte) }
                            Celda { text: Fmt.eur(filaPl.r.actualGross) }
                            Celda { text: Fmt.eur(filaPl.r.socialSecurityCost) }
                            Celda { text: Fmt.eur(filaPl.r.totalCost) }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Tokens.borderDivider }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Total plantilla"; font.pixelSize: 13; font.bold: true; color: Tokens.textHeading; Layout.preferredWidth: 170; wrapMode: Text.WordWrap }
                        Item { Layout.preferredWidth: 80 }
                        Celda { text: Fmt.num(Engine.staff.totalHeadcount) + " pers."; negrita: true; Layout.preferredWidth: 80 }
                        Item { Layout.preferredWidth: 80 }
                        Item { Layout.preferredWidth: 108 }
                        Celda { text: Fmt.eur(Engine.staff.totalActualGross); negrita: true }
                        Celda { text: Fmt.eur(Engine.staff.totalSocialSecurity); negrita: true }
                        Celda { text: Fmt.eur(Engine.staff.totalHeadcountCost); negrita: true }
                        Item { Layout.preferredWidth: 150 }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "El propietario cubre la presencia legal; coste 0 € (se retribuye vía beneficio) y no lleva subida salarial. "
                    + "Pulsa el botón de Jornada para asignar las horas/día y el año de inicio a cada empleado por separado "
                    + "(8 h = jornada completa). Mientras el año de inicio de un empleado no haya llegado, su coste no se "
                    + "incluye en la Proyección a 10 años. La subida % se aplica cada año sobre el salario base de cada tipo de trabajador."
            }
        }

        // ---------------- Refuerzos de vacaciones
        Card {
            SectionTitle { text: "Refuerzos de vacaciones" }

            Flickable {
                id: flickVacaciones
                Layout.fillWidth: true
                implicitHeight: vacacionesCol.height
                contentWidth: vacacionesCol.width
                contentHeight: vacacionesCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                pressDelay: 150
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                FastWheel { flick: flickVacaciones; fallback: page }

                ColumnLayout {
                    id: vacacionesCol
                    width: Math.max(implicitWidth, flickVacaciones.width)
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Tipo de trabajador"; font.pixelSize: 12; font.bold: true; color: Tokens.textHeading; Layout.preferredWidth: 170 }
                        CabeceraCol { text: "Jornada (h)"; Layout.preferredWidth: 80 }
                        CabeceraCol { text: "Nº personas"; Layout.preferredWidth: 80 }
                        CabeceraCol { text: "Subida %"; Layout.preferredWidth: 80 }
                        CabeceraCol { text: "Meses/año"; Layout.preferredWidth: 80 }
                        CabeceraCol { text: "Sal. bruto FT/año" }
                        CabeceraCol { text: "Sal. bruto real/año" }
                        CabeceraCol { text: "Coste SS/año" }
                        CabeceraCol { text: "Coste total tipo" }
                    }

                    Repeater {
                        model: 3
                        RowLayout {
                            id: filaVac
                            required property int index
                            readonly property var r: Engine.staff.vacationStaffPlan[index]
                            Layout.fillWidth: true
                            spacing: 8
                            Text { text: filaVac.r.role; font.pixelSize: 13; color: Tokens.textSecondary; Layout.preferredWidth: 170; wrapMode: Text.WordWrap }
                            Button {
                                id: btnJornadaVac
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                                implicitHeight: 40
                                text: Fmt.num(filaVac.r.fte * 8, 1) + " h"
                                font.pixelSize: 13
                                onClicked: dlgJornada.abrir(filaVac.index, filaVac.r.role, true)
                                background: Rectangle {
                                    radius: 5
                                    color: Tokens.bgInput
                                    border.color: (btnJornadaVac.hovered || btnJornadaVac.down) ? Tokens.borderInteractiveHover : Tokens.borderInputDefault
                                    border.width: 1
                                }
                                contentItem: Text {
                                    text: btnJornadaVac.text
                                    color: Tokens.textPrimary
                                    font: btnJornadaVac.font
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            NumField {
                                k: "vacationStaffCount" + filaVac.index
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                            }
                            PctField {
                                k: "vacationRaisePct" + filaVac.index
                                decimals: 0
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                            }
                            Celda { text: Fmt.num(filaVac.r.avgMonths, 1) + " m"; Layout.preferredWidth: 80 }
                            Celda { text: Fmt.eur(filaVac.r.grossFte) }
                            Celda { text: Fmt.eur(filaVac.r.actualGross) }
                            Celda { text: Fmt.eur(filaVac.r.socialSecurityCost) }
                            Celda { text: Fmt.eur(filaVac.r.totalCost) }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Tokens.borderDivider }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Total refuerzos"; font.pixelSize: 13; font.bold: true; color: Tokens.textHeading; Layout.preferredWidth: 170; wrapMode: Text.WordWrap }
                        Item { Layout.preferredWidth: 80 }
                        Celda { text: Fmt.num(Engine.staff.totalVacationHeadcount) + " pers."; negrita: true; Layout.preferredWidth: 80 }
                        Item { Layout.preferredWidth: 80 }
                        Item { Layout.preferredWidth: 80 }
                        Item { Layout.preferredWidth: 108 }
                        Celda { text: Fmt.eur(Engine.staff.totalVacationActualGross); negrita: true }
                        Celda { text: Fmt.eur(Engine.staff.totalVacationSocialSecurity); negrita: true }
                        Celda { text: Fmt.eur(Engine.staff.totalVacationCost); negrita: true }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Tokens.textMuted
                text: "Personal contratado temporalmente para cubrir las vacaciones de la plantilla habitual. "
                    + "El coste se calcula sobre el salario base de cada categoría (Salarios base de personal) y se prorratea "
                    + "según los meses que trabaje cada empleado al año (editable en el diálogo de Jornada, junto a las horas/día)."
            }
        }

        // ---------------- salario neto titular año 1
        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Tokens.bgBrandStrong
            implicitHeight: 64

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                Text {
                    text: "Salario neto mensual titular (año 1)"
                    color: Tokens.textOnDark;
                    font.pixelSize: 15;
                    font.bold: true
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Text {
                    text: Fmt.eur(Engine.staff.netMonthlySalaryYear1)
                    color: Tokens.textButtonPrimary; font.pixelSize: 20; font.bold: true
                }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }

    // ---------------- diálogo: jornada individual por empleado
    Popup {
        id: dlgJornada
        anchors.centerIn: page
        width: (!vacation && role !== 0) ? 400 : 320
        modal: true
        focus: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property int role: -1
        property string roleLabel: ""
        property bool vacation: false
        readonly property string fteKeyPrefix: vacation ? "vacationStaffFte" : "staffFte"
        function staffCount(rol) {
            const key = (dlgJornada.vacation ? "vacationStaffCount" : "staffCount") + rol
            const v = Engine.inputs[key]
            return v !== undefined ? v : 0
        }
        readonly property int count: role < 0 ? 0
            : Math.min(Math.max(0, Math.round(staffCount(role))), Engine.maxStaffPerRole)

        function abrir(rol, etiqueta, esVacaciones) {
            role = rol
            roleLabel = etiqueta
            vacation = esVacaciones === true
            open()
        }

        background: Rectangle {
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
        }

        contentItem: ColumnLayout {
            width: dlgJornada.availableWidth
            spacing: 10

            Text {
                text: (!dlgJornada.vacation && dlgJornada.role !== 0)
                    ? "Jornada y año de inicio por empleado"
                    : "Jornada por empleado (horas/día)"
                font.pixelSize: 15
                font.bold: true
                color: Tokens.textHeading
            }
            Text {
                text: dlgJornada.roleLabel
                font.pixelSize: 12
                color: Tokens.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(empleadosCol.implicitHeight, 260)
                contentWidth: width
                contentHeight: empleadosCol.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                ColumnLayout {
                    id: empleadosCol
                    width: parent.width
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: !dlgJornada.vacation && dlgJornada.role !== 0
                        Item { Layout.fillWidth: true }
                        Text {
                            text: "Horas/día"
                            font.pixelSize: 11
                            font.bold: true
                            color: Tokens.textHeading
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                        }
                        Text {
                            text: "Año inicio"
                            font.pixelSize: 11
                            font.bold: true
                            color: Tokens.textHeading
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    Repeater {
                        model: dlgJornada.count
                        RowLayout {
                            required property int index
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: "Empleado " + (index + 1)
                                font.pixelSize: 13
                                color: Tokens.textSecondary
                                Layout.fillWidth: true
                            }
                            HoursField {
                                k: dlgJornada.fteKeyPrefix + dlgJornada.role + "_" + index
                                decimals: 1
                            }
                            NumField {
                                visible: !dlgJornada.vacation && dlgJornada.role !== 0
                                k: "staffHireYear" + dlgJornada.role + "_" + index
                                decimals: 0
                                Layout.preferredWidth: 80
                                implicitWidth: 80
                            }
                            NumField {
                                visible: dlgJornada.vacation
                                k: "vacationStaffMonths" + dlgJornada.role + "_" + index
                                decimals: 0
                                suffix: " m/año"
                                Layout.preferredWidth: 90
                                implicitWidth: 90
                            }
                        }
                    }
                }
            }

            Text {
                visible: dlgJornada.role >= 0 && dlgJornada.staffCount(dlgJornada.role) > Engine.maxStaffPerRole
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                color: Tokens.textWarning
                text: "Solo se pueden personalizar los primeros " + Engine.maxStaffPerRole
                    + " empleados; el resto usa la jornada del último."
            }

            Button {
                id: btnCerrarJornada
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                text: "Cerrar"
                font.pixelSize: 13
                font.bold: true
                onClicked: dlgJornada.close()
                background: Rectangle {
                    radius: 8
                    color: btnCerrarJornada.down ? Tokens.bgButtonPrimaryPressed : Tokens.bgButtonPrimaryDefault
                }
                contentItem: Text {
                    text: btnCerrarJornada.text
                    color: Tokens.textButtonPrimary
                    font: btnCerrarJornada.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => mouse.accepted = false
                }
            }
        }
    }
}
