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

// Writes the "Simulación" sheet to 'dev': every entry of 'years' (QVariantMap
// with "year" (1-10) and "rows", a single table already merging every
// Facturación Total group — see Engine::exportSimulationPdf — shaped like
// { label, values[], fmt, bold } per row, with a leading "Facturación" row
// identifying which group's Facturación Total each column used, and with the
// columns collapsed in the UI already left out) is laid out one after
// another, as many per page as fit — not forced one page per year. If a
// year's columns don't fit at a readable width in a single table, that year
// is split into several side-by-side tables instead. Each individual table
// is kept whole: if it doesn't fit in what's left of the current page, it
// starts on the next page instead of being split mid-row (unless it's taller
// than a full page, in which case it still spills over as a last resort).
// 'combinacionLabels' names the remaining columns, in order (see
// Engine::exportSimulationPdf). Returns false on failure.
bool writeSimulation(QIODevice* dev, const QVariantList& years,
                      const QStringList& combinacionLabels);

} // namespace pdf
