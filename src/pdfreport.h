// pdfreport.h — Informe PDF maquetado con todas las hojas de la simulación.
#pragma once

#include "simcore.h"

class QIODevice;

namespace pdf {

// Escribe el informe completo (PDF) en 'dev'. Devuelve false si falla.
bool escribirInforme(QIODevice* dev, const sim::Inputs& in, const sim::Results& r);

} // namespace pdf
