cmake_minimum_required(VERSION 3.21)
project(FarmaciaSim VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Svg Test)

qt_standard_project_setup(REQUIRES 6.5)

# Motor de cálculo puro (sin Qt), separado en una librería para poder
# enlazarlo también desde los tests unitarios sin arrastrar el módulo QML.
add_library(farmaciasim_core STATIC
    src/simcore.h
    src/simcore.cpp
)
target_include_directories(farmaciasim_core PUBLIC src)

qt_add_executable(FarmaciaSim
    src/main.cpp
    src/engine.h
    src/engine.cpp
    src/amortmodel.h
    src/amortmodel.cpp
    src/pdfreport.h
    src/pdfreport.cpp
)

# Debe declararse ANTES de qt_add_qml_module
set_source_files_properties(qml/Fmt.qml qml/Nav.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Sin esto, el alias de recurso de cada ícono queda como "qml/icons/xxx.svg"
# (ruta relativa a la raíz del proyecto), pero el QML los referencia como
# "qrc:/qt/qml/FarmaciaSim/icons/xxx.svg" (sin el "qml/" intermedio), por lo
# que QQuickImage no los encuentra en tiempo de ejecución.
foreach(_icono datos_base financiacion proyeccion impuestos analisis banco cooperativa propiedades personal simple)
    set_source_files_properties(qml/icons/${_icono}.svg PROPERTIES
        QT_RESOURCE_ALIAS icons/${_icono}.svg
    )
endforeach()

set_source_files_properties(app_icon.ico PROPERTIES
    QT_RESOURCE_ALIAS app_icon.ico
)

qt_add_qml_module(FarmaciaSim
    URI FarmaciaSim
    VERSION 1.0
    # Nota: el proyecto debe estar en una ruta SIN acentos ni espacios
    # (qmlimportscanner falla con ellas en Windows). El escaneo de imports
    # es imprescindible para el build estático de WebAssembly.
    QML_FILES
        qml/Main.qml
        qml/NavPanel.qml
        qml/Fmt.qml
        qml/Nav.qml
        qml/views/SimulacionSimpleView.qml
        qml/views/DatosBaseView.qml
        qml/views/FinanciacionView.qml
        qml/views/ProyeccionView.qml
        qml/views/ImpuestosView.qml
        qml/views/AnalisisView.qml
        qml/views/AmortView.qml
        qml/views/PersonalView.qml
        qml/components/MoneyField.qml
        qml/components/PctField.qml
        qml/components/NumField.qml
        qml/components/SectionTitle.qml
        qml/components/KeyboardAvoider.qml
        qml/components/Card.qml
        qml/components/YearTable.qml
    RESOURCES
        qml/icons/datos_base.svg
        qml/icons/financiacion.svg
        qml/icons/proyeccion.svg
        qml/icons/impuestos.svg
        qml/icons/analisis.svg
        qml/icons/banco.svg
        qml/icons/cooperativa.svg
        qml/icons/propiedades.svg
        qml/icons/personal.svg
        qml/icons/simple.svg
        app_icon.ico
)

# El código de registro QML generado incluye los headers por su nombre base
target_include_directories(FarmaciaSim PRIVATE src)

target_link_libraries(FarmaciaSim PRIVATE farmaciasim_core Qt6::Quick Qt6::QuickControls2 Qt6::Svg)

set_target_properties(FarmaciaSim PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Ícono del ejecutable: Windows lo usa para la barra de tareas, el acceso
# directo y el Explorador de archivos.
if(WIN32)
    target_sources(FarmaciaSim PRIVATE app_icon.rc)
endif()

# En Windows, los plugins de Qt (imageformats/qsvg.dll, platforms, módulos QML, etc.)
# no se copian junto al ejecutable a menos que se ejecute windeployqt. Sin esto,
# los íconos SVG y otros recursos fallan en tiempo de ejecución con
# "QQuickImage: Cannot open".
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt6 windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FarmaciaSim POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --qmldir "${CMAKE_SOURCE_DIR}/qml" "$<TARGET_FILE:FarmaciaSim>"
            COMMENT "Desplegando dependencias de Qt (windeployqt)"
        )
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
