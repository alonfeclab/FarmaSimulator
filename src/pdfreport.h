// pdfreport.h — PDF report laid out with all the simulation sheets.
#pragma once

#include "simcore.h"

#include <QString>
#include <QStringList>
#include <QVariantList>

class QIODevice;

namespace pdf {

// Writes the full report (PDF) to 'dev'. Returns false on failure.
bool writeReport(QIODevice* dev, const sim::Inputs& in, const sim::Results& r);

// Writes only the scenario comparison table (the "Comparación" sheet) to
// 'dev': one page per entry of 'pages' (QVariantMap with "year" (1-10) and
// "rows", the latter already with the "Financiación" group interleaved if
// applicable — see Engine::exportComparisonPdf). Returns false on failure.
bool writeComparison(QIODevice* dev, const QVariantList& pages,
                      const QStringList& scenarioNames);

// Writes the "Simulación" sheet to 'dev': one page per entry of 'years'
// (QVariantMap with "year" (1-10) and "groups", the latter shaped like
// Engine::simulationForYear()'s return value, but with the columns collapsed
// in the UI already removed from every row's "values"). 'combinacionLabels'
// names the remaining columns, in order (see Engine::exportSimulationPdf).
// Returns false on failure.
bool writeSimulation(QIODevice* dev, const QVariantList& years,
                      const QStringList& combinacionLabels);

} // namespace pdf
