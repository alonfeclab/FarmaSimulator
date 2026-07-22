#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
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
    Q_PROPERTY(QVariantMap schedule     READ schedule     NOTIFY recalculated)
    Q_PROPERTY(QVariantList projection  READ projection   NOTIFY recalculated)
    Q_PROPERTY(QVariantMap taxes        READ taxes        NOTIFY recalculated)
    Q_PROPERTY(QVariantMap analysis     READ analysis     NOTIFY recalculated)
    Q_PROPERTY(AmortModel* bank         READ bank         CONSTANT)
    Q_PROPERTY(AmortModel* cooperative  READ cooperative  CONSTANT)
    Q_PROPERTY(AmortModel* family       READ family       CONSTANT)
    Q_PROPERTY(AmortModel* properties   READ properties   CONSTANT)
    Q_PROPERTY(QString dataPath         READ dataPath     CONSTANT)
    Q_PROPERTY(int maxStaffPerRole      READ maxStaffPerRole CONSTANT)
    Q_PROPERTY(QVariantList comparisonScenarios READ comparisonScenarios NOTIFY comparisonScenariosChanged)

public:
    explicit Engine(QObject* parent = nullptr);

    // Changes an input by its key (e.g. "prescriptionSales", "ipcOptimistic") and recalculates.
    Q_INVOKABLE void set(const QString& key, double value);
    Q_INVOKABLE void resetToDefaults();

    // Restores just the given keys (e.g. the ones in one Configuración
    // group) to their factory value, leaving every other input untouched.
    Q_INVOKABLE void resetKeysToDefaults(const QStringList& keys);

    // Generates the PDF report. Desktop: saves it to Documents, opens it and
    // returns the path. WASM: the browser downloads it. Empty on failure.
    Q_INVOKABLE QString exportPdf();

    // Generates a PDF with just the scenario comparison table, always with
    // the "full view" (includes the "Financiación" group). If 'year' is -1,
    // includes all 10 years, one per page; otherwise just the given year
    // (0-9). Same save/download behavior as exportPdf().
    Q_INVOKABLE QString exportComparisonPdf(int year);

    // Generates a PDF with the "Simulación" sheet: one page per year (1-10),
    // each with a single table merging every Facturación Total group: for
    // every combination (plazo/interés/aportación) still visible in at least
    // one group, one column per group where that combination isn't hidden,
    // grouped together and preceded by a "Facturación" row identifying which
    // group's Facturación Total each column used. 'hiddenColumnsPerGroup' has
    // one entry per group (same order as simulationForYear()'s return value),
    // each the combination (column) indices collapsed with the "eye" filter
    // in that group's table (SimulacionView.qml, grupoCol.collapsedColumns)
    // — those columns are left out of the PDF entirely, not just visually
    // collapsed. Same save/download behavior as exportPdf().
    Q_INVOKABLE QString exportSimulationPdf(const QVariantList& hiddenColumnsPerGroup);

    // Freezes the current projection (m_projection) as a new scenario saved
    // for the "Comparación" sheet.
    Q_INVOKABLE void addComparisonScenario();
    Q_INVOKABLE void removeComparisonScenario(int index);

    // Restores every input saved with the scenario (Datos base, Financiación,
    // Personal, Configuración...) into the engine's live state, then
    // recalculates and persists. Returns false without changing anything if
    // the scenario predates the "inputs" snapshot (nothing to restore).
    Q_INVOKABLE bool applyComparisonScenario(int index);

    // Pivots the saved scenarios: one row per concept, one column
    // (values[i]) per scenario, with that concept's value for the given year.
    Q_INVOKABLE QVariantList comparisonForYear(int year) const;

    // "Financiación" group of the full view: fixed values (don't vary by
    // year) of each saved scenario, pivoted the same way as comparisonForYear().
    Q_INVOKABLE QVariantList financingComparison() const;

    // "Simulación" sheet: agrupa por Facturación Total (igual / +X%, 2
    // grupos; X = simulationRevenueIncreasePct, editable en Configuración)
    // las 8 combinaciones (2×2×2) de aportación inicial y plazo/interés de
    // hipoteca (mobiliaria/inmobiliaria, ambos ligados: sin escenarios mixtos
    // con tipos distintos), partiendo del resto de los inputs actuales. La
    // aportación inicial usa la "Liquidez aportada" actual (Financiación:
    // contributedCash) y esa misma cifra +50.000 €, no valores fijos.
    // Devuelve una lista de 2 grupos: { "facturacion": double, "rows":
    // [ {label, values[8], fmt, bold} × 8 ] }, con las columnas ordenadas por
    // aportación (la actual primero) y el valor del año dado (0-9) en cada
    // fila. Pura: no modifica m_in ni m_r.
    Q_INVOKABLE QVariantList simulationForYear(int year) const;

    QVariantMap inputs()        const { return m_inputs; }
    QVariantMap baseData()      const { return m_baseData; }
    QVariantMap financing()     const { return m_financing; }
    QVariantMap staff()         const { return m_staff; }
    QVariantMap schedule()      const { return m_schedule; }
    QVariantList projection()   const { return m_projection; }
    QVariantMap taxes()         const { return m_taxes; }
    QVariantMap analysis()      const { return m_analysis; }
    AmortModel* bank()          const { return m_bank; }
    AmortModel* cooperative()   const { return m_coop; }
    AmortModel* family()        const { return m_family; }
    AmortModel* properties()    const { return m_properties; }
    QString dataPath()          const { return m_dataPath; }
    int maxStaffPerRole()       const { return sim::kMaxStaffPerRole; }
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
    QVariantMap inputsSnapshot() const; // full copy of every m_dbl/m_int input, by key

    sim::Inputs  m_in;
    sim::Results m_r;

    QHash<QString, double*> m_dbl; // double inputs
    QHash<QString, int*>    m_int; // integer inputs (terms)

    AmortModel* m_bank = nullptr;
    AmortModel* m_coop = nullptr;
    AmortModel* m_family = nullptr;
    AmortModel* m_properties = nullptr;

    QVariantMap  m_inputs, m_baseData, m_financing, m_staff, m_schedule, m_taxes, m_analysis;
    QVariantList m_projection;
    QString m_dataPath;

    // Frozen scenarios for comparison: each entry is
    // { "id": qint64, "name": QString, "projection": QVariantList (copy of m_projection),
    //   "financing": QVariantMap (contributedCash, pharmacyBankFinancing, premisesBankFinancing,
    //   initialOrder, propertiesFinancing),
    //   "inputs": QVariantMap (full snapshot of every m_dbl/m_int entry at freeze
    //   time, keyed exactly like registerInputs(); used to restore the scenario
    //   via applyComparisonScenario(). Absent on scenarios saved before this
    //   field existed.) }.
    QVariantList m_comparisonScenarios;
};
