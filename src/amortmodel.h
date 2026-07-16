#pragma once

#include <QAbstractTableModel>
#include <QLocale>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include "simcore.h"

// Table model for an amortization schedule (300 rows).
class AmortModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by Engine")
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QVariantMap info READ info NOTIFY infoChanged)
    Q_PROPERTY(double principal READ principal NOTIFY infoChanged)

public:
    explicit AmortModel(const QString& title, QObject* parent = nullptr);

    void setResult(const sim::AmortResult& r);

    QString title() const { return m_title; }
    QVariantMap info() const;
    double principal() const { return m_r.principal; }

    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void infoChanged();

private:
    QString m_title;
    sim::AmortResult m_r;
    int m_visibleRows = 0; // rows with a non-negative starting balance (the rest are hidden)
    QLocale m_loc { QLocale::Spanish, QLocale::Spain };
};
