#include "amortmodel.h"

AmortModel::AmortModel(const QString& title, QObject* parent)
    : QAbstractTableModel(parent), m_title(title)
{
}

void AmortModel::setResult(const sim::AmortResult& r)
{
    beginResetModel();
    m_r = r;
    // Show up to (and including) the row where the ending balance reaches 0
    // (loan paid off); the rest is hidden. Recomputed on every reset, so
    // changing the term shows/hides rows automatically.
    m_visibleRows = int(m_r.rows.size());
    for (size_t i = 0; i < m_r.rows.size(); ++i) {
        if (m_r.rows[i].endingBalance < 0.005) { // reaches 0 (float tolerance)
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
    m["principal"]     = eur(m_r.principal);
    m["annualRate"]    = m_loc.toString(m_r.annualRate * 100.0, 'f', 1) + " %";
    m["termYears"]     = QString::number(m_r.termYears) + " años";
    m["monthlyPayment"]= eur(m_r.monthlyPayment, 2);
    m["annualPayment"] = eur(m_r.annualPayment, 2);
    m["numPayments"]   = QString::number(m_r.numPayments);
    m["totalInterest"] = eur(m_r.totalInterest);
    m["totalCost"]     = eur(m_r.totalCost);
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
        case 0: return r.paymentNum;
        case 1: return QStringLiteral("%1/%2")
                    .arg(r.month, 2, 10, QLatin1Char('0')).arg(r.year);
        case 2: return eur(r.startingBalance);
        case 3: return eur(r.payment);
        case 4: return eur(r.principalPaid);
        case 5: return eur(r.interest);
        case 6: return eur(r.endingBalance);
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
