#pragma once

#include <QObject>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include "simcore.h"

class AmortModel;

// QML facade: editable inputs + results of every sheet.
class Engine : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantMap inputs       READ inputs       NOTIFY recalculated)
    Q_PROPERTY(QVariantMap baseData     READ baseData     NOTIFY recalculated)
    Q_PROPERTY(QVariantMap financing    READ financing    NOTIFY recalculated)
    Q_PROPERTY(QVariantMap staff        READ staff        NOTIFY recalculated)
    Q_PROPERTY(QVariantList projection  READ projection   NOTIFY recalculated)
    Q_PROPERTY(QVariantMap taxes        READ taxes        NOTIFY recalculated)
    Q_PROPERTY(QVariantMap analysis     READ analysis     NOTIFY recalculated)
    Q_PROPERTY(AmortModel* bank         READ bank         CONSTANT)
    Q_PROPERTY(AmortModel* cooperative  READ cooperative  CONSTANT)
    Q_PROPERTY(AmortModel* properties   READ properties   CONSTANT)
    Q_PROPERTY(QString dataPath         READ dataPath     CONSTANT)
    Q_PROPERTY(QVariantList comparisonScenarios READ comparisonScenarios NOTIFY comparisonScenariosChanged)

public:
    explicit Engine(QObject* parent = nullptr);

    // Changes an input by its key (e.g. "prescriptionSales", "ipcOptimistic") and recalculates.
    Q_INVOKABLE void set(const QString& key, double value);
    Q_INVOKABLE void resetToDefaults();

    // Recalculates the projection with some inputs substituted (e.g. from the
    // "Simulación simple" sheet), without touching the engine's shared state:
    // it doesn't affect the other sheets nor does it get saved to disk.
    Q_INVOKABLE QVariantMap simulateQuick(const QVariantMap& overrides) const;

    // Generates the PDF report. Desktop: saves it to Documents, opens it and
    // returns the path. WASM: the browser downloads it. Empty on failure.
    Q_INVOKABLE QString exportPdf();

    // Generates a PDF with just the scenario comparison table, always with
    // the "full view" (includes the "Financiación" group). If 'year' is -1,
    // includes all 10 years, one per page; otherwise just the given year
    // (0-9). Same save/download behavior as exportPdf().
    Q_INVOKABLE QString exportComparisonPdf(int year);

    // Freezes the current projection (m_projection) as a new scenario saved
    // for the "Comparación" sheet.
    Q_INVOKABLE void addComparisonScenario();
    Q_INVOKABLE void removeComparisonScenario(int index);

    // Pivots the saved scenarios: one row per concept, one column
    // (values[i]) per scenario, with that concept's value for the given year.
    Q_INVOKABLE QVariantList comparisonForYear(int year) const;

    // "Financiación" group of the full view: fixed values (don't vary by
    // year) of each saved scenario, pivoted the same way as comparisonForYear().
    Q_INVOKABLE QVariantList financingComparison() const;

    QVariantMap inputs()        const { return m_inputs; }
    QVariantMap baseData()      const { return m_baseData; }
    QVariantMap financing()     const { return m_financing; }
    QVariantMap staff()         const { return m_staff; }
    QVariantList projection()   const { return m_projection; }
    QVariantMap taxes()         const { return m_taxes; }
    QVariantMap analysis()      const { return m_analysis; }
    AmortModel* bank()          const { return m_bank; }
    AmortModel* cooperative()   const { return m_coop; }
    AmortModel* properties()    const { return m_properties; }
    QString dataPath()          const { return m_dataPath; }
    QVariantList comparisonScenarios() const { return m_comparisonScenarios; }

signals:
    void recalculated();
    void comparisonScenariosChanged();

private:
    void registerInputs();
    void recalc();
    void buildMaps();
    void resolveDataPath();  // next to the .exe; falls back to AppData if not writable
    void saveToDisk() const;
    void loadFromDisk();

    sim::Inputs  m_in;
    sim::Results m_r;

    QHash<QString, double*> m_dbl; // double inputs
    QHash<QString, int*>    m_int; // integer inputs (terms)

    AmortModel* m_bank = nullptr;
    AmortModel* m_coop = nullptr;
    AmortModel* m_properties = nullptr;

    QVariantMap  m_inputs, m_baseData, m_financing, m_staff, m_taxes, m_analysis;
    QVariantList m_projection;
    QString m_dataPath;

    // Frozen scenarios for comparison: each entry is
    // { "id": qint64, "name": QString, "projection": QVariantList (copy of m_projection),
    //   "financing": QVariantMap (contributedCash, pharmacyBankFinancing, premisesBankFinancing,
    //   initialOrder, propertiesFinancing) }.
    QVariantList m_comparisonScenarios;
};
