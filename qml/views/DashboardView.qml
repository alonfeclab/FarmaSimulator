pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtGraphs
import FarmaciaSim

// Hoja "Dashboard": resumen visual de la viabilidad económica (KPIs +
// gráficos), a partir de Engine.dashboard (series pensadas para gráficos,
// ver src/engine.cpp buildMaps()) más Engine.baseData/financing/analysis/
// inputs, que ya se exponen sin cambios para el resto de hojas.
// Cada gráfico se define una vez como component (miniatura en la rejilla y
// versión ampliada en dlgGrafico comparten la misma definición) para no
// duplicar el binding de datos. Usa QtGraphs (no QtCharts): es el módulo
// moderno de Qt Quick, renderiza con la escena gráfica nativa y no sufre el
// desenfoque/aliasing del backend heredado de QtCharts bajo escalado de
// pantalla fraccional. A cambio no trae leyenda propia, así que ChartLegend
// (más abajo) es una leyenda hecha a mano a partir de los mismos {label,
// color} que ya usa cada serie/tramo.
Flickable {
    id: page

    readonly property bool angosto: width < 900
    readonly property int chartHeight: 260

    // Gráfico mostrado en grande en dlgGrafico ("" = ninguno).
    property string grafico: ""
    property string tituloGrafico: ""

    function abrirGrafico(id, titulo) {
        page.grafico = id
        page.tituloGrafico = titulo
        dlgGrafico.open()
    }

    function resetScroll() { contentY = 0 }

    function anios10() {
        const a = []
        for (let i = 1; i <= 10; i++) a.push("Año " + i)
        return a
    }

    function porCien(lista) {
        const r = []
        for (let i = 0; i < lista.length; i++) r.push(lista[i] * 100)
        return r
    }

    // ValueAxis de QtGraphs no autoajusta su rango a los datos (a diferencia
    // de QtCharts): por defecto se queda en min:0/max:10 pase lo que pase en
    // las series, así que hay que calcularlo a mano a partir de los propios
    // valores. 'listas' es un array de arrays de números (una por serie).
    function rangoY(listas) {
        let vals = []
        for (let i = 0; i < listas.length; i++) vals = vals.concat(listas[i])
        let lo = Math.min(0, Math.min.apply(null, vals))
        let hi = Math.max(0, Math.max.apply(null, vals))
        if (hi === lo) hi = lo + 1
        const pad = (hi - lo) * 0.08
        return { min: lo - pad, max: hi + pad }
    }

    contentWidth: width
    contentHeight: col.implicitHeight + 48
    clip: true
    ScrollBar.vertical: ScrollBar {}

    KeyboardAvoider { target: page }
    FastWheel { flick: page }

    // Tema compartido por todos los gráficos (cada GraphsView necesita su
    // propia instancia; no se puede compartir una sola entre varios).
    component DashTheme: GraphsTheme {
        colorScheme: Tokens.dark ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
        backgroundVisible: false
        plotAreaBackgroundVisible: false
        labelTextColor: Tokens.textSecondary
        grid.mainColor: Tokens.borderDivider
        grid.subColor: Tokens.borderDivider
        axisX.mainColor: Tokens.borderDivider
        axisY.mainColor: Tokens.borderDivider
    }

    // Leyenda propia (QtGraphs no trae una): fila de "cuadrito de color +
    // etiqueta" por cada entrada de 'items' ([{label, color}, ...]).
    component ChartLegend: Flow {
        id: legendRoot
        property var items: []
        spacing: 14
        Repeater {
            model: legendRoot.items
            delegate: RowLayout {
                required property var modelData
                spacing: 6
                Rectangle { width: 12; height: 12; radius: 2; color: parent.modelData.color }
                Text { text: parent.modelData.label; font.pixelSize: 12; color: Tokens.textSecondary }
            }
        }
    }

    // ---------------- Definiciones de gráficos (reutilizadas en miniatura y ampliado)
    component VentasChart: ColumnLayout {
        property bool leyenda: false
        spacing: 6
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            axisX: BarCategoryAxis { categories: page.anios10() }
            axisY: ValueAxis {
                readonly property var rango: page.rangoY([Engine.dashboard.totalSales, Engine.dashboard.marginAfterRd])
                min: rango.min
                max: rango.max
            }
            BarSeries {
                BarSet { label: "Venta total"; values: Engine.dashboard.totalSales; color: Tokens.chartSeries1 }
                BarSet { label: "Margen tras RD"; values: Engine.dashboard.marginAfterRd; color: Tokens.chartSeries2 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "Venta total", color: Tokens.chartSeries1 },
                { label: "Margen tras RD", color: Tokens.chartSeries2 }
            ]
        }
    }

    component EbitdaChart: ColumnLayout {
        property bool leyenda: false
        spacing: 6
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            axisX: BarCategoryAxis { categories: page.anios10() }
            axisY: ValueAxis {
                readonly property var rango: page.rangoY([Engine.dashboard.ebitda, Engine.dashboard.profit])
                min: rango.min
                max: rango.max
            }
            BarSeries {
                BarSet { label: "EBITDA"; values: Engine.dashboard.ebitda; color: Tokens.chartSeries1 }
                BarSet { label: "Beneficio neto"; values: Engine.dashboard.profit; color: Tokens.chartSeries2 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "EBITDA", color: Tokens.chartSeries1 },
                { label: "Beneficio neto", color: Tokens.chartSeries2 }
            ]
        }
    }

    component LiquidezChart: ColumnLayout {
        property bool leyenda: false
        spacing: 6
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            axisX: BarCategoryAxis { categories: page.anios10() }
            axisY: ValueAxis {
                readonly property var rango: page.rangoY([Engine.dashboard.cashAfterTax, Engine.dashboard.netAnnualSalary])
                min: rango.min
                max: rango.max
            }
            BarSeries {
                BarSet { label: "Caja después de impuestos"; values: Engine.dashboard.cashAfterTax; color: Tokens.chartSeries1 }
                BarSet { label: "Salario neto titular"; values: Engine.dashboard.netAnnualSalary; color: Tokens.chartSeries2 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "Caja después de impuestos", color: Tokens.chartSeries1 },
                { label: "Salario neto titular", color: Tokens.chartSeries2 }
            ]
        }
    }

    component TirCagrChart: ColumnLayout {
        property bool leyenda: false
        spacing: 6
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            axisX: BarCategoryAxis { categories: ["Pesimista", "Neutral", "Optimista"] }
            axisY: ValueAxis {
                readonly property var rango: page.rangoY([page.porCien(Engine.analysis.cagr), page.porCien(Engine.analysis.irr)])
                labelFormat: "%.0f %%"
                min: rango.min
                max: rango.max
            }
            BarSeries {
                BarSet { label: "CAGR"; values: page.porCien(Engine.analysis.cagr); color: Tokens.chartSeries1 }
                BarSet { label: "TIR"; values: page.porCien(Engine.analysis.irr); color: Tokens.chartSeries2 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "CAGR", color: Tokens.chartSeries1 },
                { label: "TIR", color: Tokens.chartSeries2 }
            ]
        }
    }

    component CostesChart: ColumnLayout {
        property bool leyenda: false
        spacing: 6
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            PieSeries {
                PieSlice { label: "Coste mercancía"; value: Math.max(0, Engine.dashboard.costBreakdownYear1.costOfGoods); color: Tokens.chartSeries1 }
                PieSlice { label: "Personal + SS"; value: Math.max(0, Engine.dashboard.costBreakdownYear1.staffCost); color: Tokens.chartSeries2 }
                PieSlice { label: "Alquiler"; value: Math.max(0, Engine.dashboard.costBreakdownYear1.rent); color: Tokens.chartSeries3 }
                PieSlice { label: "Cuota autónomo"; value: Math.max(0, Engine.dashboard.costBreakdownYear1.selfEmployedQuota); color: Tokens.chartSeries4 }
                PieSlice { label: "Reales Decretos"; value: Math.max(0, Engine.dashboard.costBreakdownYear1.rdDeduction); color: Tokens.chartSeries5 }
                PieSlice { label: "Otros gastos"; value: Math.max(0, Engine.dashboard.costBreakdownYear1.otherExpenses); color: Tokens.chartSeries6 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "Coste mercancía", color: Tokens.chartSeries1 },
                { label: "Personal + SS", color: Tokens.chartSeries2 },
                { label: "Alquiler", color: Tokens.chartSeries3 },
                { label: "Cuota autónomo", color: Tokens.chartSeries4 },
                { label: "Reales Decretos", color: Tokens.chartSeries5 },
                { label: "Otros gastos", color: Tokens.chartSeries6 }
            ]
        }
    }

    component InversionChart: ColumnLayout {
        property bool leyenda: false
        spacing: 4
        Text { text: "Inversión"; font.pixelSize: 12; color: Tokens.textSecondary; Layout.alignment: Qt.AlignHCenter }
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            PieSeries {
                PieSlice { label: "Fondo comercio"; value: Math.max(0, Engine.financing.goodwill); color: Tokens.chartSeries1 }
                PieSlice { label: "Local + existencias"; value: Math.max(0, Engine.inputs.premisesPrice + Engine.inputs.inventory); color: Tokens.chartSeries2 }
                PieSlice { label: "Gastos e impuestos"; value: Math.max(0, Engine.financing.fees + Engine.financing.taxes + Engine.financing.mortgageOpeningCost); color: Tokens.chartSeries3 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "Fondo comercio", color: Tokens.chartSeries1 },
                { label: "Local + existencias", color: Tokens.chartSeries2 },
                { label: "Gastos e impuestos", color: Tokens.chartSeries3 }
            ]
        }
    }

    component FinanciacionChart: ColumnLayout {
        property bool leyenda: false
        spacing: 4
        Text { text: "Financiación"; font.pixelSize: 12; color: Tokens.textSecondary; Layout.alignment: Qt.AlignHCenter }
        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: DashTheme {}
            PieSeries {
                PieSlice { label: "Aportación propia"; value: Math.max(0, Engine.inputs.contributedCash); color: Tokens.chartSeries1 }
                PieSlice { label: "Banco"; value: Math.max(0, Engine.financing.pharmacyBankFinancing + Engine.financing.premisesBankFinancing); color: Tokens.chartSeries2 }
                PieSlice { label: "Cooperativa"; value: Math.max(0, Engine.inputs.initialOrder); color: Tokens.chartSeries3 }
                PieSlice { label: "Familiar"; value: Math.max(0, Engine.inputs.familyContribution); color: Tokens.chartSeries4 }
                PieSlice { label: "Propiedades"; value: Math.max(0, Engine.inputs.propertiesFinancing); color: Tokens.chartSeries5 }
            }
        }
        ChartLegend {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: parent.leyenda
            items: [
                { label: "Aportación propia", color: Tokens.chartSeries1 },
                { label: "Banco", color: Tokens.chartSeries2 },
                { label: "Cooperativa", color: Tokens.chartSeries3 },
                { label: "Familiar", color: Tokens.chartSeries4 },
                { label: "Propiedades", color: Tokens.chartSeries5 }
            ]
        }
    }

    // Miniatura clicable: chartComponent + overlay que abre el modal.
    component ChartThumb: Item {
        default property alias contenido: box.data
        required property string id_
        required property string titulo
        Layout.fillWidth: true
        Layout.preferredHeight: page.chartHeight

        Item { id: box; anchors.fill: parent }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: page.abrirGrafico(parent.id_, parent.titulo)
        }
    }

    ColumnLayout {
        id: col
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 24
        width: page.width - 88
        spacing: 14

        Text {
            text: "Dashboard"
            font.pixelSize: 22
            font.bold: true
            color: Tokens.textHeading
            wrapMode: Text.WordWrap
        }

        // ---------------- KPIs
        GridLayout {
            Layout.fillWidth: true
            columns: page.angosto ? 2 : 3
            rowSpacing: 12
            columnSpacing: 12

            StatTile { label: "Venta total (año 1)"; value: Engine.baseData.totalSales; fmt: "eur" }
            StatTile { label: "Beneficio antes de impuestos (año 1)"; value: Engine.baseData.profitBeforeTax; fmt: "eur" }
            StatTile { label: "Inversión total"; value: Engine.financing.totalInvestment; fmt: "eur" }
            StatTile { label: "TIR a 10 años (esc. neutral)"; value: Engine.analysis.irr[1]; fmt: "pct2" }
            StatTile { label: "Payback (aportación propia)"; value: Engine.dashboard.paybackYears; fmt: "years"; negativoEnRojo: false }
            StatTile { label: "Salario neto mensual titular (año 1)"; value: Engine.staff.netMonthlySalaryYear1; fmt: "eur" }
        }

        // ---------------- Gráficos (clic para ampliar)
        GridLayout {
            Layout.fillWidth: true
            columns: page.angosto ? 1 : 2
            rowSpacing: 14
            columnSpacing: 16

            Card {
                SectionTitle { text: "Ventas y margen comercial (10 años)" }
                ChartThumb {
                    id_: "ventas"; titulo: "Ventas y margen comercial (10 años)"
                    VentasChart { anchors.fill: parent }
                }
            }

            Card {
                SectionTitle { text: "EBITDA y beneficio neto (10 años)" }
                ChartThumb {
                    id_: "ebitda"; titulo: "EBITDA y beneficio neto (10 años)"
                    EbitdaChart { anchors.fill: parent }
                }
            }

            Card {
                SectionTitle { text: "Liquidez (10 años)" }
                ChartThumb {
                    id_: "liquidez"; titulo: "Liquidez (10 años)"
                    LiquidezChart { anchors.fill: parent }
                }
            }

            Card {
                SectionTitle { text: "TIR y CAGR por escenario (año 10)" }
                ChartThumb {
                    id_: "tircagr"; titulo: "TIR y CAGR por escenario (año 10)"
                    TirCagrChart { anchors.fill: parent }
                }
            }

            Card {
                SectionTitle { text: "Estructura de costes (año 1)" }
                ChartThumb {
                    id_: "costes"; titulo: "Estructura de costes (año 1)"
                    CostesChart { anchors.fill: parent }
                }
            }

            Card {
                SectionTitle { text: "Inversión y financiación (año 0)" }
                ChartThumb {
                    id_: "invfin"; titulo: "Inversión y financiación (año 0)"
                    RowLayout {
                        anchors.fill: parent
                        spacing: 8
                        InversionChart { Layout.fillWidth: true; Layout.fillHeight: true }
                        FinanciacionChart { Layout.fillWidth: true; Layout.fillHeight: true }
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 8 }
    }

    // ---------------- Modal: gráfico ampliado, con leyenda
    Popup {
        id: dlgGrafico
        anchors.centerIn: parent
        width: Math.min(page.width - 64, 980)
        height: Math.min(page.height - 64, 640)
        modal: true
        focus: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: page.grafico = ""

        background: Rectangle {
            radius: 12
            color: Tokens.bgSurface
            border.color: Tokens.borderSurface
        }

        contentItem: ColumnLayout {
            width: dlgGrafico.availableWidth
            height: dlgGrafico.availableHeight
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: page.tituloGrafico
                    font.pixelSize: 18
                    font.bold: true
                    color: Tokens.textHeading
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    text: "✕"
                    font.pixelSize: 18
                    color: Tokens.textMuted
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -8
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dlgGrafico.close()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                VentasChart { anchors.fill: parent; leyenda: true; visible: page.grafico === "ventas" }
                EbitdaChart { anchors.fill: parent; leyenda: true; visible: page.grafico === "ebitda" }
                LiquidezChart { anchors.fill: parent; leyenda: true; visible: page.grafico === "liquidez" }
                TirCagrChart { anchors.fill: parent; leyenda: true; visible: page.grafico === "tircagr" }
                CostesChart { anchors.fill: parent; leyenda: true; visible: page.grafico === "costes" }
                RowLayout {
                    anchors.fill: parent
                    spacing: 12
                    visible: page.grafico === "invfin"
                    InversionChart { Layout.fillWidth: true; Layout.fillHeight: true; leyenda: true }
                    FinanciacionChart { Layout.fillWidth: true; Layout.fillHeight: true; leyenda: true }
                }
            }
        }
    }
}
