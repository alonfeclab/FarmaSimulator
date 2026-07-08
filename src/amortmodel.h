#pragma once

#include <QAbstractTableModel>
#include <QLocale>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include "simcore.h"

// Modelo de tabla para un cuadro de amortización (300 filas).
class AmortModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea Engine")
    Q_PROPERTY(QString titulo READ titulo CONSTANT)
    Q_PROPERTY(QVariantMap info READ info NOTIFY infoChanged)

public:
    explicit AmortModel(const QString& titulo, QObject* parent = nullptr);

    void setResult(const sim::AmortResult& r);

    QString titulo() const { return m_titulo; }
    QVariantMap info() const;

    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void infoChanged();

private:
    QString m_titulo;
    sim::AmortResult m_r;
    int m_visibleRows = 0; // filas con saldo inicial >= 0 (el resto se oculta)
    QLocale m_loc { QLocale::Spanish, QLocale::Spain };
};
