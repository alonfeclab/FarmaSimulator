#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QUrl>
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
    Q_PROPERTY(QVariantList simulationScenarios READ simulationScenarios NOTIFY simulationScenariosChanged)
    // Folder where exported PDFs are saved (desktop only). Empty means the
    // default: the user's Documents folder (see pdfSaveDirDefault).
    Q_PROPERTY(QString pdfSaveDir        READ pdfSaveDir        NOTIFY pdfSaveDirChanged)
    Q_PROPERTY(QString pdfSaveDirDefault READ pdfSaveDirDefault CONSTANT)
    // Tema claro/oscuro elegido en el panel lateral, persistido en disco.
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)

public:
    explicit Engine(QObject* parent = nullptr);

    // Changes an input by its key (e.g. "prescriptionSales", "ipcOptimistic") and recalculates.
    Q_INVOKABLE void set(const QString& key, double value);
    Q_INVOKABLE void resetToDefaults();

    // Sets the folder where exported PDFs are saved from now on, from the
    // native folder picker's selectedFolder (ignored on WASM, which always
    // downloads through the browser). Pass an empty/non-local url to restore
    // the default (Documents).
    Q_INVOKABLE void setPdfSaveDir(const QUrl& dir);

    // pdfSaveDir (or the default, if unset) as a file:// url, for the native
    // folder picker's initial currentFolder.
    Q_INVOKABLE QUrl pdfSaveDirUrl() const;

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
    // each with the same table simulationForYear() shows in the UI (column 0
    // "Actual" + one per saved scenario). Same save/download behavior as
    // exportPdf().
    Q_INVOKABLE QString exportSimulationPdf();

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

    // Freezes the current staged overrides as a new "Simulación" scenario
    // (a new column in SimulacionView.qml, added after "Actual"). 'overrides'
    // holds only the axes the user filled in (any subset of revenueEur/
    // termYears/ratePct/cashEur/marginPct) — an absent axis keeps following
    // the live main scenario forever (see simulationForYear()).
    Q_INVOKABLE void addSimulationScenario(const QVariantMap& overrides);
    Q_INVOKABLE void removeSimulationScenario(int index);

    // "Simulación" sheet: one row per concept, one column per scenario —
    // column 0 ("Actual") is always the live main scenario (m_in) untouched;
    // columns 1..N are m_simulationScenarios, each a copy of m_in with just
    // its overridden axes replaced (see addSimulationScenario()). Returns
    // { label, values[1+N], fmt, bold } × 10, with the value for the given
    // year (0-9) in each row. Pure: never modifies m_in or m_r, so every
    // non-overridden axis of every column tracks the main scenario live.
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
    QVariantList simulationScenarios() const { return m_simulationScenarios; }
    QString pdfSaveDir()        const { return m_pdfSaveDir; }
    QString pdfSaveDirDefault() const;
    bool darkTheme()            const { return m_darkTheme; }
    void setDarkTheme(bool dark);

signals:
    void recalculated();
    void comparisonScenariosChanged();
    void simulationScenariosChanged();
    void pdfSaveDirChanged();
    void darkThemeChanged();

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
    QString m_pdfSaveDir; // empty = default (Documents); see pdfSaveDirDefault()
    bool m_darkTheme = false;

    // Frozen scenarios for comparison: each entry is
    // { "id": qint64, "name": QString, "projection": QVariantList (copy of m_projection),
    //   "financing": QVariantMap (contributedCash, pharmacyBankFinancing, premisesBankFinancing,
    //   initialOrder, propertiesFinancing),
    //   "inputs": QVariantMap (full snapshot of every m_dbl/m_int entry at freeze
    //   time, keyed exactly like registerInputs(); used to restore the scenario
    //   via applyComparisonScenario(). Absent on scenarios saved before this
    //   field existed.) }.
    QVariantList m_comparisonScenarios;

    // Manually added scenarios for "Simulación": each entry is
    // { "overrides": QVariantMap (any subset of revenueEur/termYears/
    //   ratePct/cashEur/marginPct) }. See addSimulationScenario()/
    //   simulationForYear().
    QVariantList m_simulationScenarios;
};
