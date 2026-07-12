# FarmaciaSim

Réplica en C++ / Qt 6 (QML) del Excel **Simulación Farmacia_v2.xlsx**. Las 9 hojas
(incluida la hoja **Impuestos** con el IRPF por tramos de la escala 2026) están
implementadas como vistas, con las fórmulas replicadas en un motor de cálculo
C++ puro: al editar cualquier campo amarillo se recalcula todo (proyección, IRPF,
cuadros de amortización, TIR, etc.), igual que en el Excel.

## Estructura

```
FarmaciaSim/
├── CMakeLists.txt
├── src/
│   ├── simcore.h/.cpp      Motor de cálculo puro (sin Qt) con todas las fórmulas
│   ├── pdfreport.h/.cpp    Backend del informe PDF (Qt Gui, sin QML)
│   ├── engine.h/.cpp       Fachada QML (singleton Engine): entradas + resultados
│   ├── amortmodel.h/.cpp   Modelo de tabla para los cuadros de amortización
│   └── main.cpp
└── qml/
    ├── Main.qml            Ventana + navegación entre hojas
    ├── DatosBaseView.qml   ├── FinanciacionView.qml  ├── ProyeccionView.qml
    ├── AnalisisView.qml    ├── AmortView.qml (x3)    ├── PersonalView.qml
    ├── MoneyField / PctField / NumField    Campos editables (€ / % / número)
    └── Fmt.qml             Formateo es-ES ("1.234.567 €", "2,5 %")
```

## Compilar

Requiere Qt 6.5+ (probado contra 6.11) con los módulos Quick y QuickControls2,
CMake ≥ 3.21 y un compilador C++17.

**Qt Creator:** abrir `CMakeLists.txt`, configurar el kit de Qt 6.11 y ejecutar.

**Línea de comandos (Windows, MSVC):**
```bat
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
cmake --build build --config Release
build\Release\FarmaciaSim.exe
```

## Tests

Los tests unitarios (`tests/test_simcore.cpp`, con Qt Test) cubren el motor de
cálculo puro (`simcore.cpp`): fórmulas (`pmt`, `irr`, deducción de Reales Decretos),
invariantes estructurales de `compute()` y un test de regresión con valores de
referencia, para detectar cambios accidentales en las fórmulas replicadas del Excel.

```bat
cmake --build build --target farmaciasim_tests
ctest --test-dir build --output-on-failure
```

Los tests del informe (`tests/test_pdfreport.cpp`, con Qt Test) cubren
`pdfreport.cpp`: que `escribirInforme()` produzca un PDF válido y no reviente con
casos extremos (sin ventas ni financiación). Como es un backend puro (Qt Gui,
sin QML), vive en su propia librería `farmaciasim_pdf`.

```bat
cmake --build build --target farmaciasim_pdf_tests
ctest --test-dir build --output-on-failure
```

Los tests gráficos (`tests/qml/tst_*.qml`, con Qt Quick Test) cubren la capa QML:
formateo (`Fmt.qml`), navegación (`NavPanel.qml`, con clicks simulados) y campos
editables (`MoneyField.qml`, contra el singleton `Engine` real). Para poder hacer
`import FarmaciaSim` desde los tests, el módulo QML de la app vive en la librería
`farmaciasim_ui` (`CMakeLists.txt`), compartida entre `FarmaciaSim` y
`farmaciasim_qml_tests`.

```bat
cmake --build build --target farmaciasim_qml_tests
ctest --test-dir build --output-on-failure
```

El ejecutable de tests QML corre en su propio directorio de salida
(`tests/qml-tests-bin/`) para que el `farmaciasim_datos.json` que guarda `Engine`
al recalcular no coincida con el de `FarmaciaSim.exe`.

Las tres suites corren automáticamente en GitHub Actions en cada push/PR
([.github/workflows/tests.yml](.github/workflows/tests.yml)).

## Fidelidad con el Excel

- El motor (`simcore.cpp`) replica cada fórmula celda a celda; cada línea lleva un
  comentario con la celda de origen (p. ej. `// D43`).
- Verificado contra los valores cacheados del Excel: las ~4.600 celdas calculadas
  (incluidas las 300 filas de cada cuadro de amortización) coinciden con error
  máximo 2·10⁻¹⁰.
- Se replican incluso las peculiaridades del original: el PMT usa la misma
  aritmética en coma flotante que Excel y, tras liquidarse un préstamo, el cuadro
  sigue devengando interés sobre el saldo residual (deriva negativa a partir del
  mes 241 en el préstamo bancario), exactamente como la hoja.
- Los "Impuestos venta" del Análisis de Inversión son constantes en el Excel
  (no fórmula), por lo que aquí son campos editables.
- El botón «Restaurar valores» devuelve todas las entradas a los valores del Excel.
