#pragma once

#include <QObject>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include "simcore.h"

class AmortModel;

// Fachada QML: entradas editables + resultados de todas las hojas.
class Engine : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantMap inputs       READ inputs       NOTIFY recalculated)
    Q_PROPERTY(QVariantMap datosBase    READ datosBase    NOTIFY recalculated)
    Q_PROPERTY(QVariantMap financiacion READ financiacion NOTIFY recalculated)
    Q_PROPERTY(QVariantMap personal     READ personal     NOTIFY recalculated)
    Q_PROPERTY(QVariantList proyeccion  READ proyeccion   NOTIFY recalculated)
    Q_PROPERTY(QVariantMap impuestos    READ impuestos    NOTIFY recalculated)
    Q_PROPERTY(QVariantMap analisis     READ analisis     NOTIFY recalculated)
    Q_PROPERTY(AmortModel* banco        READ banco        CONSTANT)
    Q_PROPERTY(AmortModel* cooperativa  READ cooperativa  CONSTANT)
    Q_PROPERTY(AmortModel* propiedades  READ propiedades  CONSTANT)
    Q_PROPERTY(QString rutaDatos        READ rutaDatos    CONSTANT)

public:
    explicit Engine(QObject* parent = nullptr);

    // Cambia una entrada por su clave (p. ej. "ventaReceta", "ipc0") y recalcula.
    Q_INVOKABLE void set(const QString& key, double value);
    Q_INVOKABLE void restaurarValoresIniciales();

    // Genera el informe PDF. Escritorio: lo guarda en Documentos, lo abre y
    // devuelve la ruta. WASM: lo descarga el navegador. Vacío si falla.
    Q_INVOKABLE QString exportarPdf();

    QVariantMap inputs()        const { return m_inputs; }
    QVariantMap datosBase()     const { return m_datosBase; }
    QVariantMap financiacion()  const { return m_financiacion; }
    QVariantMap personal()      const { return m_personal; }
    QVariantList proyeccion()   const { return m_proyeccion; }
    QVariantMap impuestos()     const { return m_impuestos; }
    QVariantMap analisis()      const { return m_analisis; }
    AmortModel* banco()         const { return m_banco; }
    AmortModel* cooperativa()   const { return m_coop; }
    AmortModel* propiedades()   const { return m_prop; }
    QString rutaDatos()         const { return m_rutaDatos; }

signals:
    void recalculated();

private:
    void registerInputs();
    void recalc();
    void buildMaps();
    void resolverRutaDatos();  // junto al .exe; si no se puede escribir, AppData
    void guardarEnDisco() const;
    void cargarDeDisco();

    sim::Inputs  m_in;
    sim::Results m_r;

    QHash<QString, double*> m_dbl; // entradas double
    QHash<QString, int*>    m_int; // entradas enteras (plazos)

    AmortModel* m_banco = nullptr;
    AmortModel* m_coop  = nullptr;
    AmortModel* m_prop  = nullptr;

    QVariantMap  m_inputs, m_datosBase, m_financiacion, m_personal, m_impuestos, m_analisis;
    QVariantList m_proyeccion;
    QString m_rutaDatos;
};
