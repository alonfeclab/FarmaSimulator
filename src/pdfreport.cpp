// pdfreport.cpp — Genera el informe PDF con QPdfWriter + QPainter.
// Maquetación propia (no captura de pantalla): portada, cabecera/pie por
// página, tarjetas de cifras clave y tablas con zebra, en el mismo estilo
// visual (verde #14523f) que la aplicación.
#include "pdfreport.h"

#include <QBuffer>
#include <QDate>
#include <QFontMetricsF>
#include <QLocale>
#include <QPainter>
#include <QPdfWriter>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace {

// ---------------------------------------------------------------- estilo
const QColor kVerde       ("#14523f");
const QColor kVerdeMedio  ("#1a7a5e");
const QColor kVerdeSuave  ("#e3efe9");
const QColor kZebra       ("#f7faf8");
const QColor kTexto       ("#1e2b28");
const QColor kGris        ("#3c4a46");
const QColor kGrisClaro   ("#6b7a76");
const QColor kRojo        ("#a33b2e");
const QColor kBorde       ("#dde5e1");
const QColor kAmarillo    ("#ffe9a8");

constexpr int   kDpi    = 96;   // coordenadas ~píxeles de pantalla
constexpr qreal kMargen = 42;   // ≈ 11 mm

// ---------------------------------------------------------------- documento
struct Doc {
    QPdfWriter  wr;
    QPainter    p;
    QLocale     loc{QLocale::Spanish, QLocale::Spain};
    QString     familia = QStringLiteral("Segoe UI"); // fallback automático
    QString     seccion;
    QString     fecha;
    int         numPag = 0;
    qreal       y = 0;

    explicit Doc(QIODevice* dev) : wr(dev)
    {
        wr.setResolution(kDpi);
        wr.setPageSize(QPageSize(QPageSize::A4));
        wr.setPageMargins(QMarginsF(0, 0, 0, 0));
        wr.setTitle(QStringLiteral("Simulación Farmacia — Informe"));
        wr.setCreator(QStringLiteral("FarmaciaSim"));
        fecha = loc.toString(QDate::currentDate(), QStringLiteral("d 'de' MMMM 'de' yyyy"));
    }

    qreal ancho() const { return wr.width();  }
    qreal alto()  const { return wr.height(); }
    qreal xIzq()  const { return kMargen; }
    qreal xDer()  const { return ancho() - kMargen; }
    qreal yFin()  const { return alto() - kMargen - 14; } // hueco para el pie

    QFont f(qreal pt, bool bold = false) const
    {
        QFont fu(familia);
        fu.setPointSizeF(pt);
        fu.setBold(bold);
        return fu;
    }
    qreal altoTexto(const QFont& fu) const { return QFontMetricsF(fu, &wr).height(); }

    // ---- formato es-ES. Agrupación de miles propia ("1.234.567"): el CLDR
    // español de Qt omite el punto cuando el primer grupo tiene 1 dígito.
    QString num(double v, int dec = 0) const
    {
        QString s = QString::number(v, 'f', dec);
        const bool neg = s.startsWith(QLatin1Char('-'));
        if (neg) s.remove(0, 1);
        const int punto = s.indexOf(QLatin1Char('.'));
        QString ent  = punto < 0 ? s : s.left(punto);
        QString frac = punto < 0 ? QString() : s.mid(punto + 1);
        QString res;
        for (int i = 0; i < ent.size(); ++i) {
            if (i && (ent.size() - i) % 3 == 0) res += QLatin1Char('.');
            res += ent.at(i);
        }
        if (!frac.isEmpty()) res += QLatin1Char(',') + frac;
        // evitar "-0"
        if (neg && res.contains(QRegularExpression(QStringLiteral("[1-9]"))))
            res.prepend(QLatin1Char('-'));
        return res;
    }
    QString eur (double v) const { return num(v, 0) + QStringLiteral(" €"); }
    QString eur2(double v) const { return num(v, 2) + QStringLiteral(" €"); }
    QString pct (double v, int dec = 1) const
    { return num(v * 100.0, dec) + QStringLiteral(" %"); }

    // ---- cabecera y pie de página
    void cabecera()
    {
        p.setFont(f(7.5, true));
        p.setPen(kVerde);
        p.drawText(QRectF(xIzq(), kMargen - 24, ancho() - 2 * kMargen, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("SIMULACIÓN FARMACIA · INFORME ECONÓMICO-FINANCIERO"));
        p.setFont(f(7.5));
        p.setPen(kGrisClaro);
        p.drawText(QRectF(xIzq(), kMargen - 24, ancho() - 2 * kMargen, 14),
                   Qt::AlignRight | Qt::AlignVCenter, seccion);
        p.setPen(QPen(kVerde, 1.2));
        p.drawLine(QPointF(xIzq(), kMargen - 6), QPointF(xDer(), kMargen - 6));
    }

    void pie()
    {
        p.setFont(f(7));
        p.setPen(kGrisClaro);
        const QRectF r(xIzq(), alto() - kMargen + 6, ancho() - 2 * kMargen, 14);
        p.drawText(r, Qt::AlignLeft  | Qt::AlignVCenter, fecha);
        p.drawText(r, Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("Página %1").arg(numPag));
    }

    // Nueva sección (puede cambiar la orientación de la página).
    void paginaNueva(QPageLayout::Orientation o, const QString& s)
    {
        wr.setPageOrientation(o);
        if (numPag == 0)
            p.begin(&wr);
        else
            wr.newPage();
        ++numPag;
        seccion = s;
        if (numPag > 1) {   // la portada no lleva cabecera/pie
            cabecera();
            pie();
        }
        y = kMargen + 14;
    }

    // Salto de página dentro de la misma sección (misma orientación).
    void saltoPagina()
    {
        wr.newPage();
        ++numPag;
        cabecera();
        pie();
        y = kMargen + 14;
    }

    void asegurar(qreal h)
    {
        if (y + h > yFin())
            saltoPagina();
    }

    // ---- bloques de texto
    void tituloHoja(const QString& t)
    {
        asegurar(46);
        p.setFont(f(16, true));
        p.setPen(kVerde);
        p.drawText(QPointF(xIzq(), y + 20), t);
        y += 34;
    }

    void tituloSeccion(const QString& t)
    {
        asegurar(64);
        y += 8;
        p.setFont(f(9, true));
        p.setPen(kVerdeMedio);
        p.drawText(QPointF(xIzq(), y + 11), t.toUpper());
        p.setPen(QPen(kBorde, 1));
        p.drawLine(QPointF(xIzq(), y + 17), QPointF(xDer(), y + 17));
        y += 26;
    }

    void nota(const QString& t)
    {
        const QFont fu = f(7.5);
        const QRectF medida = QFontMetricsF(fu, &wr).boundingRect(
            QRectF(0, 0, ancho() - 2 * kMargen, 1000), Qt::TextWordWrap, t);
        asegurar(medida.height() + 8);
        p.setFont(fu);
        p.setPen(kGrisClaro);
        p.drawText(QRectF(xIzq(), y, ancho() - 2 * kMargen, medida.height() + 4),
                   Qt::TextWordWrap, t);
        y += medida.height() + 8;
    }
};

// ---------------------------------------------------------------- tablas
struct Col {
    QString titulo;
    qreal   w;
    Qt::Alignment align = Qt::AlignRight;
};

struct Tabla {
    Doc&          d;
    QVector<Col>  cols;
    bool          conCabecera;
    qreal         hFila;
    qreal         ptFuente;
    int           fila = 0;

    Tabla(Doc& doc, QVector<Col> c, bool cab = true,
          qreal alto = 19, qreal pt = 8.5)
        : d(doc), cols(std::move(c)), conCabecera(cab), hFila(alto), ptFuente(pt)
    {
        if (conCabecera)
            cabecera();
    }

    qreal anchoTotal() const
    {
        qreal w = 0;
        for (const auto& c : cols) w += c.w;
        return w;
    }

    void cabecera()
    {
        d.asegurar(hFila + 4);
        d.p.setPen(Qt::NoPen);
        d.p.setBrush(kVerde);
        d.p.drawRect(QRectF(d.xIzq(), d.y, anchoTotal(), hFila + 2));
        d.p.setFont(d.f(ptFuente, true));
        d.p.setPen(Qt::white);
        qreal x = d.xIzq();
        for (const auto& c : cols) {
            d.p.drawText(QRectF(x + 6, d.y, c.w - 12, hFila + 2),
                         c.align | Qt::AlignVCenter, c.titulo);
            x += c.w;
        }
        d.y += hFila + 2;
    }

    // 'destacada' = fila de totales (fondo verde suave, texto verde, negrita).
    void filaDatos(const QStringList& celdas, bool destacada = false)
    {
        if (d.y + hFila > d.yFin()) {
            d.saltoPagina();
            if (conCabecera)
                cabecera();
        }
        const QColor fondo = destacada ? kVerdeSuave
                            : (fila % 2 ? kZebra : QColor(Qt::white));
        d.p.setPen(Qt::NoPen);
        d.p.setBrush(fondo);
        d.p.drawRect(QRectF(d.xIzq(), d.y, anchoTotal(), hFila));

        d.p.setFont(d.f(ptFuente, destacada));
        qreal x = d.xIzq();
        for (int i = 0; i < cols.size() && i < celdas.size(); ++i) {
            const QString& t = celdas.at(i);
            QColor color = destacada ? kVerde : (i == 0 ? kGris : kTexto);
            if (i > 0 && t.startsWith(QLatin1Char('-')))
                color = kRojo;                      // importes negativos en rojo
            d.p.setPen(color);
            d.p.drawText(QRectF(x + 6, d.y, cols.at(i).w - 12, hFila),
                         cols.at(i).align | Qt::AlignVCenter, t);
            x += cols.at(i).w;
        }
        d.y += hFila;
        ++fila;
    }

    void cerrar()
    {
        d.p.setPen(QPen(kBorde, 1));
        d.p.setBrush(Qt::NoBrush);
        const qreal h = d.y;
        Q_UNUSED(h)
        d.y += 4;
    }
};

// Tabla de dos columnas etiqueta/valor (como las tarjetas de la app).
Tabla kv(Doc& d)
{
    const qreal w = d.ancho() - 2 * kMargen;
    return Tabla(d, { { QString(), w - 170, Qt::AlignLeft },
                      { QString(), 170,     Qt::AlignRight } },
                 /*cab*/ false, 19, 9);
}

// ---------------------------------------------------------------- portada
void portada(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    Q_UNUSED(in)
    d.paginaNueva(QPageLayout::Portrait, QStringLiteral("Portada"));

    // Banda superior verde con cruz de farmacia.
    d.p.setPen(Qt::NoPen);
    d.p.setBrush(kVerde);
    d.p.drawRect(QRectF(0, 0, d.ancho(), 390));

    d.p.setBrush(kVerdeMedio);
    const qreal cx = d.ancho() - 150, cy = 120, b = 26; // cruz
    d.p.drawRoundedRect(QRectF(cx - b * 1.5, cy - b / 2, b * 3, b), 6, 6);
    d.p.drawRoundedRect(QRectF(cx - b / 2, cy - b * 1.5, b, b * 3), 6, 6);

    d.p.setPen(QColor("#cfe8de"));
    d.p.setFont(d.f(11, true));
    d.p.drawText(QPointF(kMargen + 14, 150), QStringLiteral("FARMACIASIM"));

    d.p.setPen(Qt::white);
    d.p.setFont(d.f(27, true));
    d.p.drawText(QPointF(kMargen + 14, 196), QStringLiteral("Simulación Farmacia"));

    d.p.setPen(QColor("#cfe8de"));
    d.p.setFont(d.f(12.5));
    d.p.drawText(QPointF(kMargen + 14, 226),
                 QStringLiteral("Informe económico-financiero · Proyección a 10 años"));

    d.p.setPen(QColor("#9fc6b6"));
    d.p.setFont(d.f(9.5));
    d.p.drawText(QPointF(kMargen + 14, 252), d.fecha);

    // Tarjetas con las cifras clave.
    struct Metrica { QString etiqueta, valor; };
    const Metrica metricas[6] = {
        { QStringLiteral("Venta total (año base)"),          d.eur(r.datosBase.ventaTotal) },
        { QStringLiteral("Bº antes de imp. y amort."),       d.eur(r.datosBase.beneficioAntesImp) },
        { QStringLiteral("Inversión total"),                 d.eur(r.financiacion.totalInversion) },
        { QStringLiteral("Financiación total"),              d.eur(r.financiacion.totalFinanciacion) },
        { QStringLiteral("TIR a 10 años (esc. neutral)"),    d.pct(r.analisis.tir[1], 2) },
        { QStringLiteral("Salario neto mensual (año 1)"),    d.eur(r.personal.salarioNetoMensualAnio1) },
    };

    const qreal x0 = kMargen + 14, gap = 16;
    const qreal wCard = (d.ancho() - 2 * x0 - gap) / 2, hCard = 86;
    qreal yy = 450;
    for (int i = 0; i < 6; ++i) {
        const qreal x = x0 + (i % 2) * (wCard + gap);
        d.p.setPen(QPen(kBorde, 1.2));
        d.p.setBrush(Qt::white);
        d.p.drawRoundedRect(QRectF(x, yy, wCard, hCard), 10, 10);
        d.p.setPen(kGrisClaro);
        d.p.setFont(d.f(8.5));
        d.p.drawText(QPointF(x + 18, yy + 28), metricas[i].etiqueta);
        d.p.setPen(kVerde);
        d.p.setFont(d.f(17, true));
        d.p.drawText(QPointF(x + 18, yy + 62), metricas[i].valor);
        if (i % 2 == 1)
            yy += hCard + gap;
    }

    // Índice de contenidos.
    yy += 26;
    d.p.setPen(kVerde);
    d.p.setFont(d.f(10, true));
    d.p.drawText(QPointF(x0, yy), QStringLiteral("CONTENIDO"));
    yy += 8;
    const char* indice[] = {
        "1.  Datos base — PyG estimada del estudio",
        "2.  Estudio de financiación",
        "3.  Proyección a 10 años",
        "4.  Impuestos (IRPF por tramos)",
        "5.  Análisis de la inversión",
        "6.  Personal",
    };
    d.p.setFont(d.f(9.5));
    for (const char* linea : indice) {
        yy += 20;
        d.p.setPen(kGris);
        d.p.drawText(QPointF(x0 + 6, yy), QString::fromUtf8(linea));
    }

    d.p.setPen(kGrisClaro);
    d.p.setFont(d.f(7.5));
    d.p.drawText(QRectF(0, d.alto() - 60, d.ancho(), 20), Qt::AlignCenter,
                 QStringLiteral("Generado con FarmaciaSim · Importes en euros · %1").arg(d.fecha));
}

// ---------------------------------------------------------------- datos base
void hojaDatosBase(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.paginaNueva(QPageLayout::Portrait, QStringLiteral("1 · Datos base"));
    d.tituloHoja(QStringLiteral("1. Datos base del estudio"));

    const auto& D = r.datosBase;

    d.tituloSeccion(QStringLiteral("Ventas"));
    Tabla t = kv(d);
    t.filaDatos({ QStringLiteral("Venta receta"),               d.eur(in.ventaReceta) });
    t.filaDatos({ QStringLiteral("Venta libre"),                d.eur(in.ventaLibre) });
    t.filaDatos({ QStringLiteral("VENTA TOTAL"),                d.eur(D.ventaTotal) }, true);

    d.tituloSeccion(QStringLiteral("Margen comercial"));
    Tabla t1b = kv(d);
    t1b.filaDatos({ QStringLiteral("Coste mercancía"),          d.eur(D.costeMercancia) });
    t1b.filaDatos({ QStringLiteral("M. comercial bruto"),       d.eur(D.mComBruto) });
    t1b.filaDatos({ QStringLiteral("M. comercial bruto %"),     d.pct(in.margenPct) });
    t1b.filaDatos({ QStringLiteral("Reales decretos"),          d.eur(D.realesDecretos) });
    t1b.filaDatos({ QStringLiteral("M. COMERCIAL DESPUÉS RDs"), d.eur(D.mComDespuesRD) }, true);

    d.tituloSeccion(QStringLiteral("Gastos de personal"));
    Tabla t2 = kv(d);
    t2.filaDatos({ QStringLiteral("Gastos de personal"), d.eur(D.gastosPersonal) });
    t2.filaDatos({ QStringLiteral("Seguridad Social"),   d.eur(D.seguridadSocial) });
    t2.filaDatos({ QStringLiteral("Cuota autónomos (RETA, año 1)"),      d.eur(r.proyeccion.cuotaAutonomos[0]) });
    t2.filaDatos({ QStringLiteral("TOTAL GASTOS PERSONAL"),              d.eur(D.totalGastosPersonal) }, true);

    d.tituloSeccion(QStringLiteral("Otros gastos"));
    Tabla t3 = kv(d);
    t3.filaDatos({ QStringLiteral("Alquiler local"),             d.eur(in.alquilerLocal) });
    t3.filaDatos({ QStringLiteral("Suministros"),                d.eur(in.suministros) });
    t3.filaDatos({ QStringLiteral("Gastos asesoría"),            d.eur(in.asesoria) });
    t3.filaDatos({ QStringLiteral("Mantenimiento informático"),  d.eur(in.mantenimiento) });
    t3.filaDatos({ QStringLiteral("Robot"),                      d.eur(in.robot) });
    t3.filaDatos({ QStringLiteral("Seguros"),                    d.eur(in.seguros) });
    t3.filaDatos({ QStringLiteral("Otros gastos"),               d.eur(in.otrosGastos) });
    t3.filaDatos({ QStringLiteral("TOTAL OTROS GASTOS"),         d.eur(D.totalOtrosGastos) }, true);

    // Resultado final destacado (banda verde).
    d.asegurar(64);
    d.y += 12;
    d.p.setPen(Qt::NoPen);
    d.p.setBrush(kVerde);
    d.p.drawRoundedRect(QRectF(d.xIzq(), d.y, d.ancho() - 2 * kMargen, 44), 10, 10);
    d.p.setPen(Qt::white);
    d.p.setFont(d.f(11, true));
    d.p.drawText(QRectF(d.xIzq() + 18, d.y, 400, 44), Qt::AlignLeft | Qt::AlignVCenter,
                 QStringLiteral("Bº ANTES DE IMPUESTOS Y AMORTIZACIONES"));
    d.p.setPen(kAmarillo);
    d.p.setFont(d.f(15, true));
    d.p.drawText(QRectF(d.xIzq(), d.y, d.ancho() - 2 * kMargen - 18, 44),
                 Qt::AlignRight | Qt::AlignVCenter, d.eur(D.beneficioAntesImp));
    d.y += 56;
}

// ---------------------------------------------------------------- financiación
void hojaFinanciacion(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.paginaNueva(QPageLayout::Portrait, QStringLiteral("2 · Financiación"));
    d.tituloHoja(QStringLiteral("2. Estudio de financiación"));

    const auto& F = r.financiacion;

    d.tituloSeccion(QStringLiteral("Escenario de crecimiento"));
    {
        Tabla t = kv(d);
        t.filaDatos({ QStringLiteral("Escenario"),
                      in.escenarioCrecimiento >= 0.5 ? QStringLiteral("Optimista") : QStringLiteral("Realista") });
        if (in.escenarioCrecimiento >= 0.5)
            t.filaDatos({ QStringLiteral("IPC"), d.pct(in.ipcOptimista) });
    }

    d.tituloSeccion(QStringLiteral("Inversión operación"));
    Tabla t2 = kv(d);
    t2.filaDatos({ QStringLiteral("Coeficiente (sobre venta total)"),   d.num(in.coeficiente, 1) });
    t2.filaDatos({ QStringLiteral("Fondo de comercio"),                 d.eur(F.fondoComercio) });
    t2.filaDatos({ QStringLiteral("Local comercial"),                   d.eur(in.localComercial) });
    t2.filaDatos({ QStringLiteral("Existencias"),                       d.eur(in.existencias) });
    t2.filaDatos({ QStringLiteral("Honorarios (5% FdC + Local)"),       d.eur(F.honorarios) });
    t2.filaDatos({ QStringLiteral("IVA (21% honorarios)"),              d.eur(F.iva) });
    t2.filaDatos({ QStringLiteral("Impuesto ITP (8% local)"),           d.eur(F.impuestoITP) });
    t2.filaDatos({ QStringLiteral("AJD (1,5% FdC + existencias)"),      d.eur(F.ajd) });
    t2.filaDatos({ QStringLiteral("Impuestos (ITP + AJD)"),             d.eur(F.impuestos) });
    t2.filaDatos({ QStringLiteral("Gastos varios operación"),           d.eur(in.gastosVarios) });
    t2.filaDatos({ QStringLiteral("TOTAL INVERSIÓN"),                   d.eur(F.totalInversion) }, true);

    d.tituloSeccion(QStringLiteral("Tipos y plazos"));
    {
        const qreal wl = d.ancho() - 2 * kMargen - 3 * 110;
        Tabla t(d, { { QStringLiteral("Préstamo"), wl, Qt::AlignLeft },
                     { QStringLiteral("Tipo interés"), 110 },
                     { QStringLiteral("Plazo"), 110 },
                     { QStringLiteral("% financiación"), 110 } });
        t.filaDatos({ QStringLiteral("Banco (farmacia)"), d.pct(in.tipoBanco),
                      QStringLiteral("%1 años").arg(in.plazoBanco), d.pct(in.pctFinFarmacia, 0) });
        t.filaDatos({ QStringLiteral("Cooperativa"), d.pct(in.tipoCoop),
                      QStringLiteral("%1 años").arg(in.plazoCoop), QStringLiteral("—") });
        t.filaDatos({ QStringLiteral("Familiar"), d.pct(in.tipoFamiliar),
                      QStringLiteral("%1 años").arg(in.plazoFamiliar), QStringLiteral("—") });
        t.filaDatos({ QStringLiteral("Local"), d.pct(in.tipoBanco),
                      QStringLiteral("%1 años").arg(in.plazoBanco), d.pct(in.pctFinLocal, 0) });
        t.filaDatos({ QStringLiteral("Propiedades"), d.pct(in.tipoPropiedades),
                      QStringLiteral("%1 años").arg(in.plazoPropiedades), d.pct(in.pctFinPropiedades, 0) });
    }

    d.tituloSeccion(QStringLiteral("Financiación"));
    Tabla t3 = kv(d);
    t3.filaDatos({ QStringLiteral("Liquidez aportada"),                 d.eur(in.liquidezAportada) });
    t3.filaDatos({ QStringLiteral("Aportación familiar"),               d.eur(in.aportacionFamiliar) });
    t3.filaDatos({ QStringLiteral("Financiación bancaria farmacia"),    d.eur(F.finBancariaFarmacia) });
    t3.filaDatos({ QStringLiteral("Financiación bancaria local"),       d.eur(F.finBancariaLocal) });
    t3.filaDatos({ QStringLiteral("Financiación propiedades"),          d.eur(in.finPropiedades) });
    t3.filaDatos({ QStringLiteral("Exceso/defecto de aportación"),      d.eur(in.excesoAportacion) });
    t3.filaDatos({ QStringLiteral("Pedido inicial (cooperativa)"),      d.eur(in.pedidoInicial) });
    t3.filaDatos({ QStringLiteral("TOTAL FINANCIACIÓN"),                d.eur(F.totalFinanciacion) }, true);

    d.nota(QStringLiteral("Inicio de los préstamos: %1/%2.")
               .arg(in.inicioMes, 2, 10, QLatin1Char('0')).arg(in.inicioAnio));
}

// ---------------------------------------------------------------- proyección
QVector<Col> colsAnios(Doc& d, int n = 10)
{
    const qreal wVal = 81;
    QVector<Col> c{ { QStringLiteral("Concepto"),
                      d.ancho() - 2 * kMargen - n * wVal, Qt::AlignLeft } };
    for (int k = 1; k <= n; ++k)
        c.append({ QStringLiteral("Año %1").arg(k), wVal });
    return c;
}

void hojaProyeccion(Doc& d, const sim::Results& r)
{
    d.paginaNueva(QPageLayout::Landscape, QStringLiteral("3 · Proyección 10 años"));
    d.tituloHoja(QStringLiteral("3. Proyección a 10 años"));

    const auto& Y = r.proyeccion;
    struct Fila { QString label; const std::array<double,10>& v; bool eur; bool bold; };
    const Fila filas[] = {
        { QStringLiteral("Venta receta"),                     Y.ventaReceta,      true,  false },
        { QStringLiteral("Venta libre"),                      Y.ventaLibre,       true,  false },
        { QStringLiteral("VENTA TOTAL"),                      Y.ventaTotal,       true,  true  },
        { QStringLiteral("Coste mercancía (proveedores)"),    Y.costeMercancia,   true,  false },
        { QStringLiteral("M. comercial bruto"),               Y.mComBruto,        true,  false },
        { QStringLiteral("Reales decretos"),                  Y.realesDecretos,   true,  false },
        { QStringLiteral("M. COMERCIAL DESPUÉS DE RDs"),      Y.mComDespuesRD,    true,  true  },
        { QStringLiteral("Alquiler local comercial"),         Y.alquiler,         true,  false },
        { QStringLiteral("Gastos de personal + S.S."),        Y.gastosPersonal,   true,  false },
        { QStringLiteral("Cuota autónomos (RETA)"),           Y.cuotaAutonomos,   true,  false },
        { QStringLiteral("Otros gastos"),                     Y.otrosGastos,      true,  false },
        { QStringLiteral("Intereses de deudas"),              Y.intereses,        true,  false },
        { QStringLiteral("BENEFICIO FARMACIA"),               Y.beneficio,        true,  true  },
        { QStringLiteral("Pago impuestos"),                   Y.pagoImpuestos,    true,  false },
        { QStringLiteral("LIQUIDEZ DESPUÉS DE IMPUESTOS"),    Y.liquidez,         true,  true  },
        { QStringLiteral("Devolución capital al banco"),      Y.devCapitalBanco,  true,  false },
        { QStringLiteral("Devolución cooperativa"),           Y.devCooperativa,   true,  false },
        { QStringLiteral("SALARIO NETO ANUAL TITULAR"),       Y.salarioNetoAnual, true,  true  },
        { QStringLiteral("SALARIO NETO MENSUAL TITULAR"),     Y.salarioNetoMensual, true, true },
        { QStringLiteral("% gasto personal s/ facturación"),  Y.pctGastoPersonal, false, false },
    };

    Tabla t(d, colsAnios(d), true, 19, 7.6);
    for (const auto& fl : filas) {
        QStringList celdas{ fl.label };
        for (double v : fl.v)
            celdas << (fl.eur ? d.eur(v) : d.pct(v, 1));
        t.filaDatos(celdas, fl.bold);
    }
}

// ---------------------------------------------------------------- impuestos
void hojaImpuestos(Doc& d, const sim::Results& r)
{
    d.paginaNueva(QPageLayout::Landscape, QStringLiteral("4 · Impuestos (IRPF)"));
    d.tituloHoja(QStringLiteral("4. Impuestos — IRPF por tramos (escala 2026)"));

    const auto& I = r.impuestos;

    d.tituloSeccion(QStringLiteral("Base amortizable"));
    Tabla t0 = kv(d);
    t0.filaDatos({ QStringLiteral("Fondo de comercio"),        d.eur(I.fdc) });
    t0.filaDatos({ QStringLiteral("Honorarios"),               d.eur(I.honorarios) });
    t0.filaDatos({ QStringLiteral("AJD"),                      d.eur(I.ajd) });
    t0.filaDatos({ QStringLiteral("BASE AMORTIZABLE"),         d.eur(I.baseAmortizable) }, true);
    t0.filaDatos({ QStringLiteral("Coste del local"),          d.eur(I.costeLocal) });
    t0.filaDatos({ QStringLiteral("Deducción mínimo personal (19%)"), d.eur(I.deduccionMinimo) });

    d.tituloSeccion(QStringLiteral("Cálculo del IRPF (10 años)"));
    struct Fila { QString label; const std::array<double,10>& v; QString fmt; bool bold; };
    const Fila filas[] = {
        { QStringLiteral("Beneficio"),                 I.beneficio,     QStringLiteral("eur"),  false },
        { QStringLiteral("Amortización local"),        I.amortLocal,    QStringLiteral("eur"),  false },
        { QStringLiteral("% amort. FdC ajustado"),     I.pctAjustado,   QStringLiteral("pct2"), false },
        { QStringLiteral("Amortización FdC"),          I.amortFdC,      QStringLiteral("eur"),  false },
        { QStringLiteral("BASE IMPONIBLE"),            I.baseImponible, QStringLiteral("eur"),  true  },
        { QStringLiteral("Cuota según escala"),        I.cuotaEscala,   QStringLiteral("eur"),  false },
        { QStringLiteral("PAGO IMPUESTOS"),            I.pago,          QStringLiteral("eur"),  true  },
    };
    Tabla t(d, colsAnios(d), true, 19, 7.6);
    for (const auto& fl : filas) {
        QStringList celdas{ fl.label };
        for (double v : fl.v)
            celdas << (fl.fmt == QLatin1String("pct2") ? d.pct(v, 2) : d.eur(v));
        t.filaDatos(celdas, fl.bold);
    }

    d.tituloSeccion(QStringLiteral("Desglose por tramos de la escala"));
    static const char* tramoLabels[6] = {
        "Tramo 0 – 12.450 € (19%)",      "Tramo 12.450 – 20.200 € (24%)",
        "Tramo 20.200 – 35.200 € (30%)", "Tramo 35.200 – 60.000 € (37%)",
        "Tramo 60.000 – 300.000 € (45%)","Tramo > 300.000 € (47%)" };
    Tabla t2(d, colsAnios(d), true, 19, 7.6);
    for (int k = 0; k < 6; ++k) {
        QStringList celdas{ QString::fromUtf8(tramoLabels[k]) };
        for (double v : I.tramos[k])
            celdas << d.eur(v);
        t2.filaDatos(celdas);
    }
}

// ---------------------------------------------------------------- análisis
void hojaAnalisis(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    d.paginaNueva(QPageLayout::Landscape, QStringLiteral("5 · Análisis inversión"));
    d.tituloHoja(QStringLiteral("5. Análisis de la inversión"));

    const auto& A = r.analisis;
    const qreal wVal = 150;
    const qreal wLbl = d.ancho() - 2 * kMargen - 3 * wVal;

    d.tituloSeccion(QStringLiteral("Valor del patrimonio en el año 10"));
    {
        Tabla t(d, { { QStringLiteral("Concepto"), wLbl, Qt::AlignLeft },
                     { QStringLiteral("Pesimista"), wVal },
                     { QStringLiteral("Neutral"),   wVal },
                     { QStringLiteral("Optimista"), wVal } });
        auto f3 = [&](const QString& l, const std::array<double,3>& v,
                      const QString& fmt = QStringLiteral("eur"), bool bold = false) {
            QStringList c{ l };
            for (double x : v)
                c << (fmt == QLatin1String("pct2") ? d.pct(x, 2)
                    : fmt == QLatin1String("num")  ? d.num(x, 1) : d.eur(x));
            t.filaDatos(c, bold);
        };
        f3(QStringLiteral("Inversión inicial"),               A.inversionInicial);
        f3(QStringLiteral("Factor de venta"),                 { in.factorVenta[0], in.factorVenta[1], in.factorVenta[2] }, QStringLiteral("num"));
        f3(QStringLiteral("Valor venta FdC año 10"),          A.valorVentaFdC);
        f3(QStringLiteral("Valor venta local (incr. IPC)"),   A.valorVentaLocal);
        f3(QStringLiteral("Existencias (10% fact. año 10)"),  A.existencias10);
        f3(QStringLiteral("Fondo de comercio pendiente"),     A.fdcPendiente);
        f3(QStringLiteral("Impuestos venta"),                 { in.impuestosVenta[0], in.impuestosVenta[1], in.impuestosVenta[2] });
        f3(QStringLiteral("Deuda pendiente año 10"),          A.deuda);
        f3(QStringLiteral("PATRIMONIO BRUTO AÑO 10"),         A.patrimonioBruto, QStringLiteral("eur"), true);
        f3(QStringLiteral("PATRIMONIO NETO AÑO 10"),          A.patrimonioNeto,  QStringLiteral("eur"), true);
        f3(QStringLiteral("CAGR PATRIMONIO"),                 A.cagr, QStringLiteral("pct2"), true);
        f3(QStringLiteral("TIR INVERSIÓN TOTAL"),             A.tir,  QStringLiteral("pct2"), true);
    }
    d.nota(QStringLiteral("CAGR: revalorización del capital al vender la farmacia a los 10 años, sin contar el salario. "
                          "TIR: retorno total, incluye el salario neto cobrado cada año más el valor de venta."));

    d.tituloSeccion(QStringLiteral("Liquidez mensual"));
    {
        Tabla t(d, { { QStringLiteral("Concepto"), wLbl, Qt::AlignLeft },
                     { QStringLiteral("Año 1"),  wVal },
                     { QStringLiteral("Año 5"),  wVal },
                     { QStringLiteral("Año 10"), wVal } });
        auto f3 = [&](const QString& l, const std::array<double,3>& v, bool bold = false) {
            QStringList c{ l };
            for (double x : v) c << d.eur(x);
            t.filaDatos(c, bold);
        };
        f3(QStringLiteral("Liquidez mensual"),     A.liqMensual);
        f3(QStringLiteral("Devolución de capital"),A.devCapitalMensual);
        f3(QStringLiteral("Intereses"),            A.interesesMensual);
        f3(QStringLiteral("NETO TITULAR"),         A.netoTitular, true);
    }

    d.tituloSeccion(QStringLiteral("Simulación amortización del fondo de comercio"));
    {
        struct Fila { QString label; const std::array<double,10>& v; QString fmt; bool bold; };
        const Fila filas[] = {
            { QStringLiteral("Beneficio (antes amort.)"), A.benFarmacia,     QStringLiteral("eur"),  false },
            { QStringLiteral("% amort. FdC (óptimo)"),    A.pctAmortFdC,     QStringLiteral("pct2"), false },
            { QStringLiteral("Amort. fondo de comercio"), A.amortFdC,        QStringLiteral("eur"),  false },
            { QStringLiteral("Amort. local comercial"),   A.amortLocal,      QStringLiteral("eur"),  false },
            { QStringLiteral("BASE IMPONIBLE"),           A.baseImponible,   QStringLiteral("eur"),  true  },
            { QStringLiteral("FdC pendiente"),            A.fdcPendienteSim, QStringLiteral("eur"),  false },
        };
        Tabla t(d, colsAnios(d), true, 19, 7.6);
        for (const auto& fl : filas) {
            QStringList celdas{ fl.label };
            for (double v : fl.v)
                celdas << (fl.fmt == QLatin1String("pct2") ? d.pct(v, 2) : d.eur(v));
            t.filaDatos(celdas, fl.bold);
        }
    }
    d.nota(QStringLiteral("Amortización del local comercial: %1/año (%2 del coste).")
               .arg(d.eur(A.amortLocalAnual), d.pct(in.pctAmortLocal, 0)));
}

// ---------------------------------------------------------------- personal
void hojaPersonal(Doc& d, const sim::Inputs& in, const sim::Results& r)
{
    Q_UNUSED(in)
    d.paginaNueva(QPageLayout::Portrait, QStringLiteral("6 · Personal"));
    d.tituloHoja(QStringLiteral("6. Personal"));

    const auto& P = r.personal;
    const qreal wTotal = d.ancho() - 2 * kMargen;

    static const char* tiposDatos[3] = { "Farmacéutico", "Auxiliar de farmacia", "Técnico" };
    d.tituloSeccion(QStringLiteral("Datos salariales"));
    {
        const qreal wP = wTotal - 6 * 88;
        Tabla t(d, { { QStringLiteral("Puesto"),       wP, Qt::AlignLeft },
                     { QStringLiteral("Bruto FT"),     88 },
                     { QStringLiteral("Jornada"),      88 },
                     { QStringLiteral("% S.S."),       88 },
                     { QStringLiteral("Coste S.S."),   88 },
                     { QStringLiteral("Salario real"), 88 },
                     { QStringLiteral("Coste total"),  88 } });
        for (int k = 0; k < 3; ++k) {
            const auto& fila = P.datos[size_t(k)];
            t.filaDatos({ QString::fromUtf8(tiposDatos[k]), d.eur(fila.brutoFT),
                          d.num(fila.jornada, 2), d.pct(fila.pctSS, 0),
                          d.eur(fila.costeSS), d.eur(fila.salReal), d.eur(fila.costeTotal) });
        }
        t.filaDatos({ QStringLiteral("TOTAL"), QString(), QString(), QString(),
                      d.eur(P.totCosteSS), d.eur(P.totSalReal), d.eur(P.totCoste) }, true);
    }

    static const char* tiposPlant[4] = { "Propietario farmacéutico", "Farmacéutico empleado",
                                         "Auxiliar de farmacia", "Técnico especialista" };
    static const char* roles[4] = {
        "L-V 9:00–17:00 · 20% mostrador · 80% gestión/dirección",
        "L-V 13:00–21:00 · Turno de cierre",
        "Turnos escalonados · Apoyo en mostrador en hora punta",
        "Media jornada · Stock, pedidos, administración" };

    d.tituloSeccion(QStringLiteral("Plantilla recomendada"));
    {
        const qreal wP = wTotal - 6 * 88;
        Tabla t(d, { { QStringLiteral("Puesto"),        wP, Qt::AlignLeft },
                     { QStringLiteral("Jornada"),       88 },
                     { QStringLiteral("Personas"),      88 },
                     { QStringLiteral("Bruto real"),    88 },
                     { QStringLiteral("Coste S.S."),    88 },
                     { QStringLiteral("Coste/persona"), 88 },
                     { QStringLiteral("Coste total"),   88 } });
        for (int k = 0; k < 4; ++k) {
            const auto& fila = P.plantilla[size_t(k)];
            t.filaDatos({ QString::fromUtf8(tiposPlant[k]), d.num(fila.jornada, 2),
                          d.num(fila.personas, 0), d.eur(fila.brutoReal),
                          d.eur(fila.costeSS), d.eur(fila.costePersona), d.eur(fila.costeTotal) });
        }
        t.filaDatos({ QStringLiteral("TOTAL"), QString(), d.num(P.totPersonas, 0),
                      d.eur(P.totBrutoReal), d.eur(P.totSS), QString(),
                      d.eur(P.totPlantilla) }, true);
    }

    d.tituloSeccion(QStringLiteral("Organización orientativa"));
    for (int k = 0; k < 4; ++k)
        d.nota(QStringLiteral("%1 — %2").arg(QString::fromUtf8(tiposPlant[k]),
                                             QString::fromUtf8(roles[k])));

    d.y += 8;
    d.asegurar(52);
    d.p.setPen(Qt::NoPen);
    d.p.setBrush(kVerdeSuave);
    d.p.drawRoundedRect(QRectF(d.xIzq(), d.y, wTotal, 40), 10, 10);
    d.p.setPen(kVerde);
    d.p.setFont(d.f(10.5, true));
    d.p.drawText(QRectF(d.xIzq() + 16, d.y, wTotal - 32, 40), Qt::AlignLeft | Qt::AlignVCenter,
                 QStringLiteral("Salario neto mensual del titular (año 1)"));
    d.p.drawText(QRectF(d.xIzq() + 16, d.y, wTotal - 32, 40), Qt::AlignRight | Qt::AlignVCenter,
                 d.eur(P.salarioNetoMensualAnio1));
    d.y += 52;
}

} // namespace

// ---------------------------------------------------------------- API
namespace pdf {

bool escribirInforme(QIODevice* dev, const sim::Inputs& in, const sim::Results& r)
{
    Doc d(dev);

    portada(d, in, r);
    hojaDatosBase(d, in, r);
    hojaFinanciacion(d, in, r);
    hojaProyeccion(d, r);
    hojaImpuestos(d, r);
    hojaAnalisis(d, in, r);
    hojaPersonal(d, in, r);

    return d.p.end();
}

} // namespace pdf
