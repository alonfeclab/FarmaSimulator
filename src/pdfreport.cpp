// pdfreport.cpp — Generates the PDF report with QPdfWriter + QPainter.
// Custom layout (not a screenshot): cover page, header/footer per page, key
// figure cards and zebra tables, in the same visual style (green #14523f)
// as the app.
#include "pdfreport.h"

#include <QBuffer>
#include <QDate>
#include <QFontMetricsF>
#include <QLocale>
#include <QPainter>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace {

// ---------------------------------------------------------------- style
const QColor kGreen      ("#14523f");
const QColor kGreenMedium("#1a7a5e");
const QColor kGreenSoft  ("#e3efe9");
const QColor kZebra      ("#f7faf8");
const QColor kText       ("#1e2b28");
const QColor kGray       ("#3c4a46");
const QColor kGrayLight  ("#6b7a76");
const QColor kRed        ("#a33b2e");
const QColor kBorder     ("#dde5e1");
const QColor kYellow     ("#ffe9a8");

constexpr int   kDpi    = 96;   // ~screen-pixel coordinates
constexpr qreal kMargin = 42;   // ≈ 11 mm

// ---------------------------------------------------------------- document
struct Doc {
    QPdfWriter  wr;
    QPainter    p;
    QLocale     loc{QLocale::Spanish, QLocale::Spain};
    QString     fontFamily = QStringLiteral("Segoe UI"); // automatic fallback
    QString     section;
    QString     date;
    int         pageNum = 0;
    qreal       y = 0;

    explicit Doc(QIODevice* dev) : wr(dev)
    {
        wr.setResolution(kDpi);
        wr.setPageSize(QPageSize(QPageSize::A4));
        wr.setPageMargins(QMarginsF(0, 0, 0, 0));
        wr.setTitle(QStringLiteral("Simulación Farmacia — Informe"));
        wr.setCreator(QStringLiteral("FarmaciaSim"));
        date = loc.toString(QDate::currentDate(), QStringLiteral("d 'de' MMMM 'de' yyyy"));
    }

    qreal pageWidth()  const { return wr.width();  }
    qreal pageHeight() const { return wr.height(); }
    qreal leftX()  const { return kMargin; }
    qreal rightX() const { return pageWidth() - kMargin; }
    qreal bottomY() const { return pageHeight() - kMargin - 14; } // room for the footer

    QFont f(qreal pt, bool bold = false) const
    {
        QFont fu(fontFamily);
        fu.setPointSizeF(pt);
        fu.setBold(bold);
        return fu;
    }
    qreal textHeight(const QFont& fu) const { return QFontMetricsF(fu, &wr).height(); }

    // ---- es-ES formatting. Custom thousands grouping ("1.234.567"): Qt's
    // Spanish CLDR omits the dot when the first group has 1 digit.
    QString num(double v, int dec = 0) const
    {
        QString s = QString::number(v, 'f', dec);
        const bool neg = s.startsWith(QLatin1Char('-'));
        if (neg) s.remove(0, 1);
        const int dot = s.indexOf(QLatin1Char('.'));
        QString intPart  = dot < 0 ? s : s.left(dot);
        QString fracPart = dot < 0 ? QString() : s.mid(dot + 1);
        QString res;
        for (int i = 0; i < intPart.size(); ++i) {
            if (i && (intPart.size() - i) % 3 == 0) res += QLatin1Char('.');
            res += intPart.at(i);
        }
        if (!fracPart.isEmpty()) res += QLatin1Char(',') + fracPart;
        // avoid "-0"
        if (neg && res.contains(QRegularExpression(QStringLiteral("[1-9]"))))
            res.prepend(QLatin1Char('-'));
        return res;
    }
    QString eur (double v) const { return num(v, 0) + QStringLiteral(" €"); }
    QString eur2(double v) const { return num(v, 2) + QStringLiteral(" €"); }
    QString pct (double v, int dec = 1) const
    { return num(v * 100.0, dec) + QStringLiteral(" %"); }

    // ---- page header and footer
    void header()
    {
        p.setFont(f(7.5, true));
        p.setPen(kGreen);
        p.drawText(QRectF(leftX(), kMargin - 24, pageWidth() - 2 * kMargin, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("SIMULACIÓN FARMACIA · INFORME ECONÓMICO-FINANCIERO"));
        p.setFont(f(7.5));
        p.setPen(kGrayLight);
        p.drawText(QRectF(leftX(), kMargin - 24, pageWidth() - 2 * kMargin, 14),
                   Qt::AlignRight | Qt::AlignVCenter, section);
        p.setPen(QPen(kGreen, 1.2));
        p.drawLine(QPointF(leftX(), kMargin - 6), QPointF(rightX(), kMargin - 6));
    }

    void footer()
    {
        p.setFont(f(7));
        p.setPen(kGrayLight);
        const QRectF r(leftX(), pageHeight() - kMargin + 6, pageWidth() - 2 * kMargin, 14);
        p.drawText(r, Qt::AlignLeft  | Qt::AlignVCenter, date);
        p.drawText(r, Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("Página %1").arg(pageNum));
    }

    // New section (may change the page orientation).
    void newPage(QPageLayout::Orientation o, const QString& s)
    {
        wr.setPageOrientation(o);
        if (pageNum == 0)
            p.begin(&wr);
        else
            wr.newPage();
        ++pageNum;
        section = s;
        if (pageNum > 1) {   // the cover page has no header/footer
            header();
            footer();
        }
        y = kMargin + 14;
    }

    // Page break within the same section (same orientation).
    void pageBreak()
    {
        wr.newPage();
        ++pageNum;
        header();
        footer();
        y = kMargin + 14;
    }

    void ensureSpace(qreal h)
    {
        if (y + h > bottomY())
            pageBreak();
    }

    // ---- text blocks
    void sheetTitle(const QString& t)
    {
        ensureSpace(46);
        p.setFont(f(16, true));
        p.setPen(kGreen);
        p.drawText(QPointF(leftX(), y + 20), t);
        y += 34;
    }

    void sectionTitle(const QString& t)
    {
        ensureSpace(64);
        y += 8;
        p.setFont(f(9, true));
        p.setPen(kGreenMedium);
        p.drawText(QPointF(leftX(), y + 11), t.toUpper());
        p.setPen(QPen(kBorder, 1));
        p.drawLine(QPointF(leftX(), y + 17), QPointF(rightX(), y + 17));
        y += 26;
    }

    void note(const QString& t)
    {
        const QFont fu = f(7.5);
        const QRectF measured = QFontMetricsF(fu, &wr).boundingRect(
            QRectF(0, 0, pageWidth() - 2 * kMargin, 1000), Qt::TextWordWrap, t);
        ensureSpace(measured.height() + 8);
        p.setFont(fu);
        p.setPen(kGrayLight);
        p.drawText(QRectF(leftX(), y, pageWidth() - 2 * kMargin, measured.height() + 4),
                   Qt::TextWordWrap, t);
        y += measured.height() + 8;
    }
};

// ---------------------------------------------------------------- tables
struct Col {
    QString title;
    qreal   w;
    Qt::Alignment align = Qt::AlignRight;
};

struct Table {
    Doc&          d;
    QVector<Col>  cols;
    bool          hasHeader;
    qreal         rowHeight;
    qreal         fontPt;
    int           row = 0;

    Table(Doc& doc, QVector<Col> c, bool withHeader = true,
          qreal height = 19, qreal pt = 8.5)
        : d(doc), cols(std::move(c)), hasHeader(withHeader), rowHeight(height), fontPt(pt)
    {
        if (hasHeader)
            header();
    }

    qreal totalWidth() const
    {
        qreal w = 0;
        for (const auto& c : cols) w += c.w;
        return w;
    }

    void header()
    {
        d.ensureSpace(rowHeight + 4);
        d.p.setPen(Qt::NoPen);
        d.p.setBrush(kGreen);
        d.p.drawRect(QRectF(d.leftX(), d.y, totalWidth(), rowHeight + 2));
        d.p.setFont(d.f(fontPt, true));
        d.p.setPen(Qt::white);
        qreal x = d.leftX();
        for (const auto& c : cols) {
            d.p.drawText(QRectF(x + 6, d.y, c.w - 12, rowHeight + 2),
                         c.align | Qt::AlignVCenter, c.title);
            x += c.w;
        }
        d.y += rowHeight + 2;
    }

    // 'highlighted' = totals row (soft green background, green bold text).
    // 'group'/'groupEnd' = row belonging to a separate group (e.g.
    // "Financiación" in the comparison table): own background, and if it's
    // the last row of the group, a divider line underneath — same as
    // ConceptTable.qml. 'merged' = single value shared by every column
    // (e.g. "Coste total farmacia" in Simulación): drawn once, centered,
    // spanning every column after the label — 'cells' only needs the label
    // (index 0) and that one value (index 1) in this case, same as
    // ConceptTable.qml's merged row.
    void dataRow(const QStringList& cells, bool highlighted = false,
                    bool group = false, bool groupEnd = false, bool merged = false)
    {
        if (d.y + rowHeight > d.bottomY()) {
            d.pageBreak();
            if (hasHeader)
                header();
        }
        const QColor bg = group ? QColor("#eef5f1")
                            : highlighted ? kGreenSoft
                            : (row % 2 ? kZebra : QColor(Qt::white));
        d.p.setPen(Qt::NoPen);
        d.p.setBrush(bg);
        d.p.drawRect(QRectF(d.leftX(), d.y, totalWidth(), rowHeight));

        d.p.setFont(d.f(fontPt, highlighted));
        if (merged && cols.size() >= 2) {
            d.p.setPen(highlighted ? kGreen : kGray);
            d.p.drawText(QRectF(d.leftX() + 6, d.y, cols.at(0).w - 12, rowHeight),
                         cols.at(0).align | Qt::AlignVCenter, cells.value(0));

            const QString& t = cells.value(1);
            QColor color = highlighted ? kGreen : kText;
            if (t.startsWith(QLatin1Char('-')))
                color = kRed;
            d.p.setPen(color);
            d.p.drawText(QRectF(d.leftX() + cols.at(0).w, d.y, totalWidth() - cols.at(0).w, rowHeight),
                         Qt::AlignHCenter | Qt::AlignVCenter, t);
        } else {
            qreal x = d.leftX();
            for (int i = 0; i < cols.size() && i < cells.size(); ++i) {
                const QString& t = cells.at(i);
                QColor color = highlighted ? kGreen : (i == 0 ? kGray : kText);
                if (i > 0 && t.startsWith(QLatin1Char('-')))
                    color = kRed;                      // negative amounts in red
                d.p.setPen(color);
                d.p.drawText(QRectF(x + 6, d.y, cols.at(i).w - 12, rowHeight),
                             cols.at(i).align | Qt::AlignVCenter, t);
                x += cols.at(i).w;
            }
        }
        if (groupEnd) {
            d.p.setPen(QPen(QColor("#8fb3a3"), 2));
            d.p.drawLine(QPointF(d.leftX(), d.y + rowHeight), QPointF(d.leftX() + totalWidth(), d.y + rowHeight));
        }
        d.y += rowHeight;
        ++row;
    }

    // Full-width separator row (e.g. the "Financiación" title).
    void separatorRow(const QString& label)
    {
        if (d.y + rowHeight > d.bottomY()) {
            d.pageBreak();
            if (hasHeader)
                header();
        }
        d.p.setPen(Qt::NoPen);
        d.p.setBrush(QColor("#dde9e2"));
        d.p.drawRect(QRectF(d.leftX(), d.y, totalWidth(), rowHeight));
        d.p.setFont(d.f(fontPt, true));
        d.p.setPen(kGreen);
        d.p.drawText(QRectF(d.leftX() + 6, d.y, totalWidth() - 12, rowHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, label);
        d.y += rowHeight;
    }

    void close()
    {
        d.p.setPen(QPen(kBorder, 1));
        d.p.setBrush(Qt::NoBrush);
        const qreal h = d.y;
        Q_UNUSED(h)
        d.y += 4;
    }
};

// Two-column label/value table (like the app's cards).
Table kv(Doc& d)
{
    const qreal w = d.pageWidth() - 2 * kMargin;
    return Table(d, { { QString(), w - 170, Qt::AlignLeft },
                      { QString(), 170,     Qt::AlignRight } },
                 /*header*/ false, 19, 9);
}

// ---------------------------------------------------------------- cover page
void coverPage(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    Q_UNUSED(in)
    d.newPage(QPageLayout::Portrait, QStringLiteral("Portada"));

    // Green top band with pharmacy cross.
    d.p.setPen(Qt::NoPen);
    d.p.setBrush(kGreen);
    d.p.drawRect(QRectF(0, 0, d.pageWidth(), 390));

    d.p.setBrush(kGreenMedium);
    const qreal cx = d.pageWidth() - 150, cy = 120, b = 26; // cross
    d.p.drawRoundedRect(QRectF(cx - b * 1.5, cy - b / 2, b * 3, b), 6, 6);
    d.p.drawRoundedRect(QRectF(cx - b / 2, cy - b * 1.5, b, b * 3), 6, 6);

    d.p.setPen(QColor("#cfe8de"));
    d.p.setFont(d.f(11, true));
    d.p.drawText(QPointF(kMargin + 14, 150), QStringLiteral("FARMACIASIM"));

    d.p.setPen(Qt::white);
    d.p.setFont(d.f(27, true));
    d.p.drawText(QPointF(kMargin + 14, 196), QStringLiteral("Simulación Farmacia"));

    d.p.setPen(QColor("#cfe8de"));
    d.p.setFont(d.f(12.5));
    d.p.drawText(QPointF(kMargin + 14, 226),
                 QStringLiteral("Informe económico-financiero · Proyección a 10 años"));

    d.p.setPen(QColor("#9fc6b6"));
    d.p.setFont(d.f(9.5));
    d.p.drawText(QPointF(kMargin + 14, 252), d.date);

    // Cards with the key figures.
    struct Metric { QString label, value; };
    const Metric metrics[6] = {
        { QStringLiteral("Venta total (año base)"),          d.eur(r.baseData.totalSales) },
        { QStringLiteral("Bº antes de imp. y amort."),       d.eur(r.baseData.profitBeforeTax) },
        { QStringLiteral("Inversión total"),                 d.eur(r.financing.totalInvestment) },
        { QStringLiteral("Financiación total"),              d.eur(r.financing.totalFinancing) },
        { QStringLiteral("TIR a 10 años (esc. neutral)"),    d.pct(r.analysis.irr[1], 2) },
        { QStringLiteral("Salario neto mensual (año 1)"),    d.eur(r.staff.netMonthlySalaryYear1) },
    };

    const qreal x0 = kMargin + 14, gap = 16;
    const qreal wCard = (d.pageWidth() - 2 * x0 - gap) / 2, hCard = 86;
    qreal yy = 450;
    for (int i = 0; i < 6; ++i) {
        const qreal x = x0 + (i % 2) * (wCard + gap);
        d.p.setPen(QPen(kBorder, 1.2));
        d.p.setBrush(Qt::white);
        d.p.drawRoundedRect(QRectF(x, yy, wCard, hCard), 10, 10);
        d.p.setPen(kGrayLight);
        d.p.setFont(d.f(8.5));
        d.p.drawText(QPointF(x + 18, yy + 28), metrics[i].label);
        d.p.setPen(kGreen);
        d.p.setFont(d.f(17, true));
        d.p.drawText(QPointF(x + 18, yy + 62), metrics[i].value);
        if (i % 2 == 1)
            yy += hCard + gap;
    }

    // Table of contents.
    yy += 26;
    d.p.setPen(kGreen);
    d.p.setFont(d.f(10, true));
    d.p.drawText(QPointF(x0, yy), QStringLiteral("Contenido"));
    yy += 8;
    const char* tableOfContents[] = {
        "1.  Datos base — PyG estimada del estudio",
        "2.  Estudio de financiación",
        "3.  Proyección a 10 años",
        "4.  Impuestos (IRPF por tramos)",
        "5.  Análisis de la inversión",
        "6.  Personal",
    };
    d.p.setFont(d.f(9.5));
    for (const char* line : tableOfContents) {
        yy += 20;
        d.p.setPen(kGray);
        d.p.drawText(QPointF(x0 + 6, yy), QString::fromUtf8(line));
    }

    d.p.setPen(kGrayLight);
    d.p.setFont(d.f(7.5));
    d.p.drawText(QRectF(0, d.pageHeight() - 60, d.pageWidth(), 20), Qt::AlignCenter,
                 QStringLiteral("Generado con FarmaciaSim · Importes en euros · %1").arg(d.date));
}

// ---------------------------------------------------------------- base data
void baseDataSheet(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.newPage(QPageLayout::Portrait, QStringLiteral("1 · Datos base"));
    d.sheetTitle(QStringLiteral("1. Datos base del estudio"));

    const auto& D = r.baseData;

    d.sectionTitle(QStringLiteral("Ventas"));
    Table t = kv(d);
    t.dataRow({ QStringLiteral("Venta receta"),               d.eur(in.prescriptionSales) });
    t.dataRow({ QStringLiteral("Venta libre"),                d.eur(in.otcSales) });
    t.dataRow({ QStringLiteral("Venta total"),                d.eur(D.totalSales) }, true);

    d.sectionTitle(QStringLiteral("Margen comercial"));
    Table t1b = kv(d);
    t1b.dataRow({ QStringLiteral("Coste mercancía"),          d.eur(D.costOfGoods) });
    t1b.dataRow({ QStringLiteral("M. comercial bruto"),       d.eur(D.grossMargin) });
    t1b.dataRow({ QStringLiteral("M. comercial bruto %"),     d.pct(in.marginPct) });
    t1b.dataRow({ QStringLiteral("Reales decretos"),          d.eur(D.rdDeduction) });
    t1b.dataRow({ QStringLiteral("M. comercial después RDs"), d.eur(D.marginAfterRd) }, true);

    d.sectionTitle(QStringLiteral("Gastos de personal"));
    Table t2 = kv(d);
    t2.dataRow({ QStringLiteral("Gastos de personal"), d.eur(D.staffCost) });
    t2.dataRow({ QStringLiteral("Seguridad social"),   d.eur(D.socialSecurity) });
    t2.dataRow({ QStringLiteral("Cuota autónomos (RETA, año 1)"),      d.eur(r.projection.selfEmployedQuota[0]) });
    t2.dataRow({ QStringLiteral("Total gastos personal"),              d.eur(D.totalStaffCost) }, true);

    d.sectionTitle(QStringLiteral("Alquiler"));
    Table tRent = kv(d);
    tRent.dataRow({ QStringLiteral("Alquiler local"),          d.eur(in.premisesRent) }, true);

    d.sectionTitle(QStringLiteral("Otros gastos"));
    Table t3 = kv(d);
    t3.dataRow({ QStringLiteral("Suministros"),                d.eur(in.utilities) });
    t3.dataRow({ QStringLiteral("Gastos asesoría"),            d.eur(in.advisoryFees) });
    t3.dataRow({ QStringLiteral("Mantenimiento informático"),  d.eur(in.maintenance) });
    t3.dataRow({ QStringLiteral("Robot"),                      d.eur(in.robot) });
    t3.dataRow({ QStringLiteral("Seguros"),                    d.eur(in.insurance) });
    t3.dataRow({ QStringLiteral("Otros gastos"),               d.eur(in.otherExpenses) });
    t3.dataRow({ QStringLiteral("Total otros gastos"),         d.eur(D.totalOtherExpenses) }, true);

    // Highlighted final result (green band).
    d.ensureSpace(64);
    d.y += 12;
    d.p.setPen(Qt::NoPen);
    d.p.setBrush(kGreen);
    d.p.drawRoundedRect(QRectF(d.leftX(), d.y, d.pageWidth() - 2 * kMargin, 44), 10, 10);
    d.p.setPen(Qt::white);
    d.p.setFont(d.f(11, true));
    d.p.drawText(QRectF(d.leftX() + 18, d.y, 400, 44), Qt::AlignLeft | Qt::AlignVCenter,
                 QStringLiteral("Bº antes de impuestos y amortizaciones"));
    d.p.setPen(kYellow);
    d.p.setFont(d.f(15, true));
    d.p.drawText(QRectF(d.leftX(), d.y, d.pageWidth() - 2 * kMargin - 18, 44),
                 Qt::AlignRight | Qt::AlignVCenter, d.eur(D.profitBeforeTax));
    d.y += 56;
}

// ---------------------------------------------------------------- financing
void financingSheet(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.newPage(QPageLayout::Portrait, QStringLiteral("2 · Financiación"));
    d.sheetTitle(QStringLiteral("2. Estudio de financiación"));

    const auto& F = r.financing;

    d.sectionTitle(QStringLiteral("Escenario de crecimiento"));
    {
        Table t = kv(d);
        t.dataRow({ QStringLiteral("Escenario"),
                      in.growthScenario >= 0.5 ? QStringLiteral("Optimista") : QStringLiteral("Realista") });
        if (in.growthScenario >= 0.5)
            t.dataRow({ QStringLiteral("IPC"), d.pct(in.ipcOptimistic) });
    }

    d.sectionTitle(QStringLiteral("Inversión operación"));
    Table t2 = kv(d);
    t2.dataRow({ QStringLiteral("Coeficiente s/venta total"),         d.num(in.goodwillMultiple, 2) });
    t2.dataRow({ QStringLiteral("Fondo de comercio"),                 d.eur(F.goodwill) });
    t2.dataRow({ QStringLiteral("Local comercial"),                   d.eur(in.premisesPrice) });
    t2.dataRow({ QStringLiteral("Existencias"),                       d.eur(in.inventory) });
    t2.dataRow({ QStringLiteral("Honorarios (%1)").arg(d.pct(in.feesPct)), d.eur(F.fees) });
    t2.dataRow({ QStringLiteral("IVA (%1)").arg(d.pct(in.ivaPct)),               d.eur(F.iva) });
    t2.dataRow({ QStringLiteral("ITP (%1)").arg(d.pct(in.itpPct)),               d.eur(F.itpTax) });
    t2.dataRow({ QStringLiteral("AJD (%1)").arg(d.pct(in.ajdPct)),               d.eur(F.ajd) });
    t2.dataRow({ QStringLiteral("Impuestos"),                         d.eur(F.taxes) });
    t2.dataRow({ QStringLiteral("Notario"),                           d.eur(in.notaryFees) });
    t2.dataRow({ QStringLiteral("Registro"),                          d.eur(in.registryFees) });
    t2.dataRow({ QStringLiteral("Gastos varios operación"),           d.eur(in.miscExpenses) });
    t2.dataRow({ QStringLiteral("Gastos de apertura hipoteca"),       d.eur(F.mortgageOpeningCost) });
    t2.dataRow({ QStringLiteral("Total inversión"),                   d.eur(F.totalInvestment) }, true);

    d.sectionTitle(QStringLiteral("Tipos y plazos"));
    {
        const qreal wl = d.pageWidth() - 2 * kMargin - 3 * 110;
        Table t(d, { { QStringLiteral("Préstamo"), wl, Qt::AlignLeft },
                     { QStringLiteral("Tipo interés"), 110 },
                     { QStringLiteral("Plazo"), 110 },
                     { QStringLiteral("% financiación"), 110 } });
        t.dataRow({ QStringLiteral("Banco (farmacia)"), d.pct(in.bankRate),
                      QStringLiteral("%1 años").arg(in.bankTermYears), d.pct(in.pharmacyFinancingPct, 0) });
        t.dataRow({ QStringLiteral("Cooperativa"), d.pct(in.coopRate),
                      QStringLiteral("%1 años").arg(in.coopTermYears), QStringLiteral("—") });
        t.dataRow({ QStringLiteral("Familiar"), d.pct(in.familyRate),
                      QStringLiteral("%1 años").arg(in.familyTermYears), QStringLiteral("—") });
        t.dataRow({ QStringLiteral("Propiedades"), d.pct(in.propertiesRate),
                      QStringLiteral("%1 años").arg(in.propertiesTermYears), d.pct(in.propertiesFinancingPct, 0) });
        t.dataRow({ QStringLiteral("Local"), d.pct(in.propertiesRate),
                      QStringLiteral("%1 años").arg(in.propertiesTermYears), d.pct(in.premisesFinancingPct, 0) });
    }

    d.sectionTitle(QStringLiteral("Financiación"));
    Table t3 = kv(d);
    t3.dataRow({ QStringLiteral("Liquidez aportada"),                 d.eur(in.contributedCash) });
    t3.dataRow({ QStringLiteral("Aportación familiar"),               d.eur(in.familyContribution) });
    t3.dataRow({ QStringLiteral("Financiación bancaria farmacia"),    d.eur(F.pharmacyBankFinancing) });
    t3.dataRow({ QStringLiteral("Financiación propiedades"),          d.eur(in.propertiesFinancing * in.propertiesFinancingPct) });
    t3.dataRow({ QStringLiteral("Financiación bancaria local"),       d.eur(F.premisesBankFinancing) });
    t3.dataRow({ QStringLiteral("Total propiedades"),                 d.eur(in.propertiesFinancing * in.propertiesFinancingPct + F.premisesBankFinancing) }, true);
    t3.dataRow({ QStringLiteral("Exceso/defecto de aportación"),      d.eur(in.contributionExcess) });
    t3.dataRow({ QStringLiteral("Pedido inicial (cooperativa)"),      d.eur(in.initialOrder) });
    t3.dataRow({ QStringLiteral("Total financiación"),                d.eur(F.totalFinancing) }, true);

    d.note(QStringLiteral("Inicio de los préstamos: %1/%2.")
               .arg(in.startMonth, 2, 10, QLatin1Char('0')).arg(in.startYear));
}

// ---------------------------------------------------------------- projection
QVector<Col> yearColumns(Doc& d, int n = 10)
{
    const qreal wVal = 81;
    QVector<Col> c{ { QStringLiteral("Concepto"),
                      d.pageWidth() - 2 * kMargin - n * wVal, Qt::AlignLeft } };
    for (int k = 1; k <= n; ++k)
        c.append({ QStringLiteral("Año %1").arg(k), wVal });
    return c;
}

void projectionSheet(Doc& d, const sim::Results& r)
{
    d.newPage(QPageLayout::Landscape, QStringLiteral("3 · Proyección 10 años"));
    d.sheetTitle(QStringLiteral("3. Proyección a 10 años"));

    const auto& Y = r.projection;
    struct Row { QString label; const std::array<double,10>& v; bool isEur; bool bold; };
    const Row rows[] = {
        { QStringLiteral("Venta receta"),                     Y.prescriptionSales, true,  false },
        { QStringLiteral("Venta libre"),                      Y.otcSales,          true,  false },
        { QStringLiteral("Venta total"),                      Y.totalSales,        true,  true  },
        { QStringLiteral("Coste mercancía"),                  Y.costOfGoods,       true,  false },
        { QStringLiteral("M. comercial bruto"),               Y.grossMargin,       true,  false },
        { QStringLiteral("Reales decretos"),                  Y.rdDeduction,       true,  false },
        { QStringLiteral("M. comercial después de RDs"),      Y.marginAfterRd,     true,  true  },
        { QStringLiteral("Alquiler local"),                   Y.rent,              true,  false },
        { QStringLiteral("Gastos personal + SS"),             Y.staffCost,         true,  false },
        { QStringLiteral("Cuota autónomos"),                  Y.selfEmployedQuota, true,  false },
        { QStringLiteral("Otros gastos"),                     Y.otherExpenses,     true,  false },
        { QStringLiteral("Intereses de deudas"),              Y.interest,          true,  false },
        { QStringLiteral("Beneficio farmacia"),                Y.profit,           true,  true  },
        { QStringLiteral("Pago impuestos"),                   Y.taxPayment,        true,  false },
        { QStringLiteral("Liquidez después de impuestos"),    Y.cashAfterTax,      true,  true  },
        { QStringLiteral("Devolución banco"),                 Y.bankPrincipalRepayment, true, false },
        { QStringLiteral("Devolución cooperativa"),           Y.coopPrincipalRepayment, true, false },
        { QStringLiteral("Salario neto anual titular"),       Y.netAnnualSalary,   true,  true  },
        { QStringLiteral("Salario neto mensual titular"),     Y.netMonthlySalary,  true, true },
        { QStringLiteral("% gasto personal s/ facturación"),  Y.staffCostPct,      false, false },
    };

    Table t(d, yearColumns(d), true, 19, 7.6);
    for (const auto& row : rows) {
        QStringList cells{ row.label };
        for (double v : row.v)
            cells << (row.isEur ? d.eur(v) : d.pct(v, 1));
        t.dataRow(cells, row.bold);
    }
}

// Label of an IRPF bracket (e.g. "12.450 – 20.200 € (24%)" or, for the last
// bracket, "> 300.000 € (47%)"), generated from the actual bracket (editable
// from the Configuración sheet) instead of a fixed string.
QString irpfBracketLabel(const sim::IrpfBracket& t, bool isLast)
{
    static const QLocale loc(QLocale::Spanish, QLocale::Spain);
    const double pct100 = t.rate * 100.0;
    const int decPct = (std::abs(pct100 - std::round(pct100)) < 1e-6) ? 0 : 1;
    const QString pctStr = loc.toString(pct100, 'f', decPct) + QStringLiteral("%");
    if (isLast)
        return QStringLiteral("> ") + loc.toString(t.from, 'f', 0)
             + QStringLiteral(" € (") + pctStr + QStringLiteral(")");
    return loc.toString(t.from, 'f', 0) + QStringLiteral(" – ") + loc.toString(t.to, 'f', 0)
         + QStringLiteral(" € (") + pctStr + QStringLiteral(")");
}

// ---------------------------------------------------------------- taxes
void taxesSheet(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.newPage(QPageLayout::Landscape, QStringLiteral("4 · Impuestos (IRPF)"));
    d.sheetTitle(QStringLiteral("4. Impuestos — IRPF por tramos (escala 2026)"));

    const auto& I = r.taxes;

    d.sectionTitle(QStringLiteral("Base amortizable"));
    Table t0 = kv(d);
    t0.dataRow({ QStringLiteral("Fondo de comercio"),        d.eur(I.fdc) });
    t0.dataRow({ QStringLiteral("Honorarios"),               d.eur(I.fees) });
    t0.dataRow({ QStringLiteral("AJD"),                      d.eur(I.ajd) });
    t0.dataRow({ QStringLiteral("Base amortizable"),         d.eur(I.depreciableBase) }, true);
    t0.dataRow({ QStringLiteral("Coste del local"),          d.eur(I.premisesCost) });
    t0.dataRow({ QStringLiteral("Deducción mínimo personal (%1)").arg(d.pct(in.irpfBrackets[0].rate)), d.eur(I.minimumDeduction) });

    d.sectionTitle(QStringLiteral("Cálculo del IRPF (10 años)"));
    struct Row { QString label; const std::array<double,10>& v; QString fmt; bool bold; };
    const Row rows[] = {
        { QStringLiteral("Beneficio"),                 I.profit,               QStringLiteral("eur"),  false },
        { QStringLiteral("Amortización local"),        I.premisesDepreciation, QStringLiteral("eur"),  false },
        { QStringLiteral("% amort. FdC ajustado"),     I.adjustedPct,          QStringLiteral("pct2"), false },
        { QStringLiteral("Amortización FdC"),          I.fdcDepreciation,      QStringLiteral("eur"),  false },
        { QStringLiteral("Base imponible"),            I.taxableBase,          QStringLiteral("eur"),  true  },
        { QStringLiteral("Cuota según escala"),        I.bracketQuota,         QStringLiteral("eur"),  false },
        { QStringLiteral("Pago impuestos"),            I.payment,              QStringLiteral("eur"),  true  },
    };
    Table t(d, yearColumns(d), true, 19, 7.6);
    for (const auto& row : rows) {
        QStringList cells{ row.label };
        for (double v : row.v)
            cells << (row.fmt == QLatin1String("pct2") ? d.pct(v, 2) : d.eur(v));
        t.dataRow(cells, row.bold);
    }

    d.sectionTitle(QStringLiteral("Desglose por tramos de la escala"));
    Table t2(d, yearColumns(d), true, 19, 7.6);
    for (int k = 0; k < 6; ++k) {
        QStringList cells{ irpfBracketLabel(in.irpfBrackets[k], k == 5) };
        for (double v : I.brackets[k])
            cells << d.eur(v);
        t2.dataRow(cells);
    }
}

// ---------------------------------------------------------------- analysis
void analysisSheet(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.newPage(QPageLayout::Landscape, QStringLiteral("5 · Análisis inversión"));
    d.sheetTitle(QStringLiteral("5. Análisis de la inversión"));

    const auto& A = r.analysis;
    const qreal wVal = 150;
    const qreal wLbl = d.pageWidth() - 2 * kMargin - 3 * wVal;

    d.sectionTitle(QStringLiteral("Valor del patrimonio en el año 10"));
    {
        Table t(d, { { QStringLiteral("Concepto"), wLbl, Qt::AlignLeft },
                     { QStringLiteral("Pesimista"), wVal },
                     { QStringLiteral("Neutral"),   wVal },
                     { QStringLiteral("Optimista"), wVal } });
        auto row3 = [&](const QString& l, const std::array<double,3>& v,
                      const QString& fmt = QStringLiteral("eur"), bool bold = false) {
            QStringList c{ l };
            for (double x : v)
                c << (fmt == QLatin1String("pct2") ? d.pct(x, 2)
                    : fmt == QLatin1String("num")  ? d.num(x, 1) : d.eur(x));
            t.dataRow(c, bold);
        };
        row3(QStringLiteral("Inversión inicial"),               A.initialInvestment);
        row3(QStringLiteral("Factor de venta"),                 { in.saleFactor[0], in.saleFactor[1], in.saleFactor[2] }, QStringLiteral("num"));
        row3(QStringLiteral("Valor venta FdC año 10"),          A.fdcSaleValue);
        row3(QStringLiteral("Valor venta local (incr. IPC)"),   A.premisesSaleValue);
        row3(QStringLiteral("Existencias (%1 factur.)").arg(d.pct(in.inventoryPctYear10)), A.inventoryYear10);
        row3(QStringLiteral("Fondo de comercio pendiente"),     A.fdcOutstanding);
        row3(QStringLiteral("Impuestos venta"),                 { in.saleTaxes[0], in.saleTaxes[1], in.saleTaxes[2] });
        row3(QStringLiteral("Deuda pendiente año 10"),          A.debt);
        row3(QStringLiteral("Patrimonio bruto año 10"),         A.grossEquity, QStringLiteral("eur"), true);
        row3(QStringLiteral("Patrimonio neto año 10"),          A.netEquity,  QStringLiteral("eur"), true);
        row3(QStringLiteral("CAGR patrimonio"),                 A.cagr, QStringLiteral("pct2"), true);
        row3(QStringLiteral("TIR inversión total"),             A.irr,  QStringLiteral("pct2"), true);
    }
    d.note(QStringLiteral("CAGR: revalorización del capital al vender la farmacia a los 10 años, sin contar el salario. "
                          "TIR: retorno total, incluye el salario neto cobrado cada año más el valor de venta."));

    d.sectionTitle(QStringLiteral("Liquidez mensual"));
    {
        Table t(d, { { QStringLiteral("Concepto"), wLbl, Qt::AlignLeft },
                     { QStringLiteral("Año 1"),  wVal },
                     { QStringLiteral("Año 5"),  wVal },
                     { QStringLiteral("Año 10"), wVal } });
        auto row3 = [&](const QString& l, const std::array<double,3>& v, bool bold = false) {
            QStringList c{ l };
            for (double x : v) c << d.eur(x);
            t.dataRow(c, bold);
        };
        row3(QStringLiteral("Liquidez mensual"),     A.monthlyCashFlow);
        row3(QStringLiteral("Devolución de capital"),A.monthlyPrincipalRepayment);
        row3(QStringLiteral("Intereses"),            A.monthlyInterest);
        row3(QStringLiteral("Neto titular"),         A.ownerNetIncome, true);
    }

    d.sectionTitle(QStringLiteral("Simulación amortización del fondo de comercio"));
    {
        struct Row { QString label; const std::array<double,10>& v; QString fmt; bool bold; };
        const Row rows[] = {
            { QStringLiteral("Beneficio (antes amort.)"), A.pharmacyProfit,     QStringLiteral("eur"),  false },
            { QStringLiteral("% amort. FdC (óptimo)"),    A.fdcDepreciationPct, QStringLiteral("pct2"), false },
            { QStringLiteral("Amort. fondo de comercio"), A.fdcDepreciation,    QStringLiteral("eur"),  false },
            { QStringLiteral("Amort. local comercial"),   A.premisesDepreciation, QStringLiteral("eur"),  false },
            { QStringLiteral("Base imponible"),           A.taxableBase,        QStringLiteral("eur"),  true  },
            { QStringLiteral("FdC pendiente"),            A.fdcOutstandingSim,  QStringLiteral("eur"),  false },
        };
        Table t(d, yearColumns(d), true, 19, 7.6);
        for (const auto& row : rows) {
            QStringList cells{ row.label };
            for (double v : row.v)
                cells << (row.fmt == QLatin1String("pct2") ? d.pct(v, 2) : d.eur(v));
            t.dataRow(cells, row.bold);
        }
    }
    d.note(QStringLiteral("Amortización del local comercial: %1/año (%2 del coste).")
               .arg(d.eur(A.annualPremisesDepreciation), d.pct(in.investmentPremisesDeprPct, 0)));
}

// ---------------------------------------------------------------- staff
void staffSheet(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    Q_UNUSED(in)
    d.newPage(QPageLayout::Portrait, QStringLiteral("6 · Personal"));
    d.sheetTitle(QStringLiteral("6. Personal"));

    const auto& P = r.staff;
    const qreal wTotal = d.pageWidth() - 2 * kMargin;

    static const char* roleLabelsData[3] = { "Farmacéutico", "Auxiliar de farmacia", "Técnico" };
    d.sectionTitle(QStringLiteral("Datos salariales"));
    {
        const qreal wP = wTotal - 6 * 88;
        Table t(d, { { QStringLiteral("Puesto"),       wP, Qt::AlignLeft },
                     { QStringLiteral("Bruto FT"),     88 },
                     { QStringLiteral("Jornada"),      88 },
                     { QStringLiteral("% S.S."),       88 },
                     { QStringLiteral("Coste S.S."),   88 },
                     { QStringLiteral("Salario real"), 88 },
                     { QStringLiteral("Coste total"),  88 } });
        for (int k = 0; k < 3; ++k) {
            const auto& row = P.byRole[size_t(k)];
            t.dataRow({ QString::fromUtf8(roleLabelsData[k]), d.eur(row.grossFte),
                          d.num(row.fte * 8.0, 1) + QStringLiteral(" h"), d.pct(row.socialSecurityPct, 0),
                          d.eur(row.socialSecurityCost), d.eur(row.actualSalary), d.eur(row.totalCost) });
        }
        t.dataRow({ QStringLiteral("Total"), QString(), QString(), QString(),
                      d.eur(P.totalSocialSecurityCost), d.eur(P.totalActualSalary), d.eur(P.totalCost) }, true);
    }

    static const char* roleLabelsHeadcount[4] = { "Propietario farmacéutico", "Farmacéutico empleado",
                                         "Auxiliar de farmacia", "Técnico especialista" };
    static const char* roles[4] = {
        "L-V 9:00–17:00 · 20% mostrador · 80% gestión/dirección",
        "L-V 13:00–21:00 · Turno de cierre",
        "Turnos escalonados · Apoyo en mostrador en hora punta",
        "Media jornada · Stock, pedidos, administración" };

    d.sectionTitle(QStringLiteral("Plantilla recomendada"));
    {
        const qreal wP = wTotal - 6 * 88;
        Table t(d, { { QStringLiteral("Puesto"),        wP, Qt::AlignLeft },
                     { QStringLiteral("Jornada"),       88 },
                     { QStringLiteral("Personas"),      88 },
                     { QStringLiteral("Bruto real"),    88 },
                     { QStringLiteral("Coste S.S."),    88 },
                     { QStringLiteral("Coste/persona"), 88 },
                     { QStringLiteral("Coste total"),   88 } });
        for (int k = 0; k < 4; ++k) {
            const auto& row = P.headcountPlan[size_t(k)];
            t.dataRow({ QString::fromUtf8(roleLabelsHeadcount[k]), d.num(row.fte * 8.0, 1) + QStringLiteral(" h"),
                          d.num(row.headcount, 0), d.eur(row.actualGross),
                          d.eur(row.socialSecurityCost), d.eur(row.costPerPerson), d.eur(row.totalCost) });
        }
        t.dataRow({ QStringLiteral("Total"), QString(), d.num(P.totalHeadcount, 0),
                      d.eur(P.totalActualGross), d.eur(P.totalSocialSecurity), QString(),
                      d.eur(P.totalHeadcountCost) }, true);
    }

    d.sectionTitle(QStringLiteral("Organización orientativa"));
    for (int k = 0; k < 4; ++k)
        d.note(QStringLiteral("%1 — %2").arg(QString::fromUtf8(roleLabelsHeadcount[k]),
                                             QString::fromUtf8(roles[k])));

    d.y += 8;
    d.ensureSpace(52);
    d.p.setPen(Qt::NoPen);
    d.p.setBrush(kGreenSoft);
    d.p.drawRoundedRect(QRectF(d.leftX(), d.y, wTotal, 40), 10, 10);
    d.p.setPen(kGreen);
    d.p.setFont(d.f(10.5, true));
    d.p.drawText(QRectF(d.leftX() + 16, d.y, wTotal - 32, 40), Qt::AlignLeft | Qt::AlignVCenter,
                 QStringLiteral("Salario neto mensual del titular (año 1)"));
    d.p.drawText(QRectF(d.leftX() + 16, d.y, wTotal - 32, 40), Qt::AlignRight | Qt::AlignVCenter,
                 d.eur(P.netMonthlySalaryYear1));
    d.y += 52;
}

// ---------------------------------------------------------------- comparison
// Draws one page (comparison table for one year). Called once per requested
// year, each on its own page (all landscape).
void comparisonPage(Doc& d, const QVariantList& rows, const QStringList& names, int year)
{
    d.newPage(QPageLayout::Landscape, QStringLiteral("Comparación de escenarios"));
    if (d.pageNum == 1) {   // no prior cover page: this page does get a header/footer
        d.header();
        d.footer();
    }
    d.sheetTitle(QStringLiteral("Comparación de escenarios — Año %1").arg(year));

    const qreal wLabel = 220;
    const qreal wTotal = d.pageWidth() - 2 * kMargin;
    const int n = std::max(1, int(names.size()));
    const qreal wVal = (wTotal - wLabel) / n;

    QVector<Col> cols{ { QStringLiteral("Concepto"), wLabel, Qt::AlignLeft } };
    for (const QString& name : names)
        cols.append({ name, wVal });

    Table t(d, cols, true, 19, 8.5);
    for (const QVariant& rv : rows) {
        const QVariantMap row = rv.toMap();
        if (row.value(QStringLiteral("separator")).toBool()) {
            t.separatorRow(row.value(QStringLiteral("label")).toString());
            continue;
        }
        const QString fmt = row.value(QStringLiteral("fmt")).toString();
        QStringList cells{ row.value(QStringLiteral("label")).toString() };
        for (const QVariant& v : row.value(QStringLiteral("values")).toList()) {
            const double val = v.toDouble();
            if (fmt == QLatin1String("pct1"))      cells << d.pct(val, 1);
            else if (fmt == QLatin1String("pct2")) cells << d.pct(val, 2);
            else if (fmt == QLatin1String("num"))  cells << d.num(val, 1);
            else                                    cells << d.eur(val);
        }
        t.dataRow(cells, row.value(QStringLiteral("bold")).toBool(),
                    row.value(QStringLiteral("group")).toBool(),
                    row.value(QStringLiteral("groupEnd")).toBool());
    }
}

// ---------------------------------------------------------------- simulation
QString simulationCell(const Doc& d, double v, const QString& fmt)
{
    if (fmt == QLatin1String("pct1"))  return d.pct(v, 1);
    if (fmt == QLatin1String("years")) return d.num(v, 0) + QStringLiteral(" años");
    return d.eur(v);
}

// Below this column width (pt, at the table's 7.6 pt font) a Simulación
// combination column stops being readable — the widest cells are amounts
// like "1.200.000 €". Years with more columns than fit at this width get
// split into several side-by-side tables instead of squeezing every column
// into one (see simulationYearBlock()).
constexpr qreal kMinSimColWidth = 70;
// Row height passed to every Simulación Table (both header and data rows use
// it, see the Table constructor call below) — kept as a named constant so
// simulationYearBlock() can estimate a whole table's height up front.
constexpr qreal kSimRowHeight = 18;
// y increment of Doc::sectionTitle(): 8 (top gap) + 26 (title text + rule).
constexpr qreal kSectionTitleHeight = 34;

// Draws one year's worth of Simulación data: a single table merging every
// Facturación Total group (igual / + un % configurable) — unlike
// SimulacionView.qml, which shows one table per group, here the columns of
// the same combination (plazo/interés/aportación) from each group are placed
// next to each other, with a leading "Facturación" row identifying which
// group's Facturación Total each column used. Only the columns still visible
// in the UI are included ('combinaciones'/each row's "values" already
// exclude the ones collapsed with the "eye" filter, see
// Engine::exportSimulationPdf). If every column doesn't fit at a readable
// width in a single table, it's split into as many side-by-side tables as
// needed (each still labeled "Año N"), one after another — the page itself
// isn't forced to break per year: tables simply keep flowing down the
// document, breaking pages only when they run out of room (see
// Doc::ensureSpace(), used internally by sectionTitle()/Table::header()).
void simulationYearBlock(Doc& d, const QVariantList& rows, const QStringList& combinaciones, int year)
{
    const qreal wLabel = 190;
    const qreal wTotal = d.pageWidth() - 2 * kMargin;
    const int totalCols = std::max(1, int(combinaciones.size()));

    const int maxColsPerTable = std::max(1, int((wTotal - wLabel) / kMinSimColWidth));
    const int numTables = (totalCols + maxColsPerTable - 1) / maxColsPerTable;
    // Reparto uniforme entre las tablas de este año (en vez de llenar cada
    // una al máximo y dejar el resto en la última), para que no quede una
    // tabla final con muy pocas columnas.
    const int colsPerTable = (totalCols + numTables - 1) / numTables;

    // Altura de una tabla completa de este año (título de sección + cabecera
    // + todas sus filas): igual para todas las sub-tablas, solo cambian sus
    // columnas. Reservarla entera de una vez permite saltar de página ANTES
    // de empezar a dibujarla si no cabe en lo que queda de la actual, en vez
    // de dejar que se parta a mitad de fila.
    const qreal tableHeight = kSectionTitleHeight + (kSimRowHeight + 2) + rows.size() * kSimRowHeight;

    int parte = 1;
    for (int start = 0; start < totalCols; start += colsPerTable, ++parte) {
        const int n = std::min(colsPerTable, totalCols - start);
        const qreal wVal = (wTotal - wLabel) / n;

        QVector<Col> cols{ { QStringLiteral("Concepto"), wLabel, Qt::AlignLeft } };
        for (int i = start; i < start + n; ++i)
            cols.append({ combinaciones.at(i), wVal });

        // Si la tabla completa no cabe en el resto de la página, se salta de
        // página entera aquí. Si ni siquiera cabe en una página en blanco
        // (caso extremo con muchísimas filas), no hay nada que hacer: se deja
        // que Table la parta como último recurso, igual que antes.
        d.ensureSpace(tableHeight);

        d.sectionTitle(numTables > 1
            ? QStringLiteral("Año %1 (%2/%3)").arg(year).arg(parte).arg(numTables)
            : QStringLiteral("Año %1").arg(year));

        Table t(d, cols, true, kSimRowHeight, 7.6);
        for (const QVariant& rv : rows) {
            const QVariantMap row = rv.toMap();
            const QString fmt = row.value(QStringLiteral("fmt")).toString();
            const bool merged = row.value(QStringLiteral("merged")).toBool();
            const QVariantList values = row.value(QStringLiteral("values")).toList();
            QStringList cells{ row.value(QStringLiteral("label")).toString() };
            if (merged) {
                // Un único valor para todas las columnas de esta sub-tabla:
                // el de la primera (ver Engine::simulationForYear).
                cells << simulationCell(d, values.value(start).toDouble(), fmt);
            } else {
                for (int i = start; i < start + n; ++i)
                    cells << simulationCell(d, values.value(i).toDouble(), fmt);
            }
            t.dataRow(cells, row.value(QStringLiteral("bold")).toBool(), false, false, merged);
        }
    }
}

} // namespace

// ---------------------------------------------------------------- API
namespace pdf {

bool writeReport(QIODevice* dev, const sim::Inputs& in, const sim::Results& r)
{
    Doc d(dev);

    coverPage(d, in, r);
    baseDataSheet(d, in, r);
    financingSheet(d, in, r);
    projectionSheet(d, r);
    taxesSheet(d, in, r);
    analysisSheet(d, in, r);
    staffSheet(d, in, r);

    return d.p.end();
}

bool writeComparison(QIODevice* dev, const QVariantList& pages,
                      const QStringList& scenarioNames)
{
    Doc d(dev);
    for (const QVariant& pv : pages) {
        const QVariantMap page = pv.toMap();
        comparisonPage(d, page.value(QStringLiteral("rows")).toList(), scenarioNames,
                           page.value(QStringLiteral("year")).toInt());
    }
    return d.p.end();
}

bool writeSimulation(QIODevice* dev, const QVariantList& years,
                      const QStringList& combinacionLabels)
{
    Doc d(dev);
    d.newPage(QPageLayout::Landscape, QStringLiteral("Simulación"));
    if (d.pageNum == 1) {   // no prior cover page: this page does get a header/footer
        d.header();
        d.footer();
    }
    d.sheetTitle(QStringLiteral("Simulación"));

    // Los años se van apilando uno tras otro (varios por página si caben),
    // en vez de forzar una página nueva por año: ver simulationYearBlock().
    for (const QVariant& yv : years) {
        const QVariantMap ypage = yv.toMap();
        simulationYearBlock(d, ypage.value(QStringLiteral("rows")).toList(), combinacionLabels,
                                ypage.value(QStringLiteral("year")).toInt());
    }
    return d.p.end();
}

} // namespace pdf
