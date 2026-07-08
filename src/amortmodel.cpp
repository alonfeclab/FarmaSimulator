#include "amortmodel.h"

AmortModel::AmortModel(const QString& titulo, QObject* parent)
    : QAbstractTableModel(parent), m_titulo(titulo)
{
}

void AmortModel::setResult(const sim::AmortResult& r)
{
    beginResetModel();
    m_r = r;
    // Mostrar hasta la fila donde el saldo final llega a 0 (préstamo
    // liquidado), inclusive; el resto se oculta. Se recalcula en cada
    // reset, así que al cambiar el plazo se muestran/ocultan solas.
    m_visibleRows = int(m_r.rows.size());
    for (size_t i = 0; i < m_r.rows.size(); ++i) {
        if (m_r.rows[i].saldoFin < 0.005) { // llega a 0 (tolerancia flotante)
            m_visibleRows = int(i) + 1;
            break;
        }
    }
    endResetModel();
    emit infoChanged();
}

QVariantMap AmortModel::info() const
{
    auto eur = [this](double v, int dec = 0) {
        return m_loc.toString(v, 'f', dec) + QStringLiteral(" €");
    };
    QVariantMap m;
    m["principal"]      = eur(m_r.principal);
    m["tasaAnual"]      = m_loc.toString(m_r.tasaAnual * 100.0, 'f', 1) + " %";
    m["plazoAnios"]     = QString::number(m_r.plazoAnios) + " años";
    m["pagoMensual"]    = eur(m_r.pagoMensual, 2);
    m["pagoAnual"]      = eur(m_r.pagoAnual, 2);
    m["numPagos"]       = QString::number(m_r.numPagos);
    m["totalIntereses"] = eur(m_r.totalIntereses);
    m["costeTotal"]     = eur(m_r.costeTotal);
    return m;
}

int AmortModel::rowCount(const QModelIndex&) const
{
    return m_visibleRows;
}

int AmortModel::columnCount(const QModelIndex&) const
{
    return 7; // Nº, Fecha, Saldo inicial, Cuota, Capital, Interés, Saldo final
}

QVariant AmortModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() >= int(m_r.rows.size()))
        return {};
    const auto& r = m_r.rows[size_t(idx.row())];

    if (role == Qt::DisplayRole) {
        auto eur = [this](double v) {
            return m_loc.toString(v, 'f', 0) + QStringLiteral(" €");
        };
        switch (idx.column()) {
        case 0: return r.num;
        case 1: return QStringLiteral("%1/%2")
                    .arg(r.mes, 2, 10, QLatin1Char('0')).arg(r.anio);
        case 2: return eur(r.saldoIni);
        case 3: return eur(r.cuota);
        case 4: return eur(r.capital);
        case 5: return eur(r.interes);
        case 6: return eur(r.saldoFin);
        }
    }
    return {};
}

QVariant AmortModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (o == Qt::Horizontal) {
        static const char* h[] = { "Nº", "Fecha", "Saldo inicial", "Cuota",
                                   "Capital", "Interés", "Saldo final" };
        if (section >= 0 && section < 7)
            return QString::fromUtf8(h[section]);
    }
    return section + 1;
}

QHash<int, QByteArray> AmortModel::roleNames() const
{
    return { { Qt::DisplayRole, "display" } };
}
