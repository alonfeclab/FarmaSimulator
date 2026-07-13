// pdfreport.h — Informe PDF maquetado con todas las hojas de la simulación.
#pragma once

#include "simcore.h"

#include <QString>
#include <QStringList>
#include <QVariantList>

class QIODevice;

namespace pdf {

// Escribe el informe completo (PDF) en 'dev'. Devuelve false si falla.
bool escribirInforme(QIODevice* dev, const sim::Inputs& in, const sim::Results& r);

// Escribe solo la tabla comparativa de escenarios (hoja "Comparación") en
// 'dev': una página por cada entrada de 'paginas' (QVariantMap con "anio"
// (1-10) y "filas", esta última ya con el grupo "Financiación" intercalado
// si procede — ver Engine::exportarPdfComparacion). Devuelve false si falla.
bool escribirComparacion(QIODevice* dev, const QVariantList& paginas,
                          const QStringList& nombresEscenarios);

} // namespace pdf
