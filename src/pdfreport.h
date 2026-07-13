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
// 'dev', para el año indicado (1-10). 'filas' ya incluye, si procede, el
// grupo "Financiación" intercalado (ver Engine::exportarPdfComparacion).
// Devuelve false si falla.
bool escribirComparacion(QIODevice* dev, const QVariantList& filas,
                          const QStringList& nombresEscenarios, int anio);

} // namespace pdf
