cmake_minimum_required(VERSION 3.21)
project(FarmaciaSim VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Svg)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(FarmaciaSim
    src/main.cpp
    src/simcore.h
    src/simcore.cpp
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

qt_add_qml_module(FarmaciaSim
    URI FarmaciaSim
    VERSION 1.0
    # Nota: el proyecto debe estar en una ruta SIN acentos ni espacios
    # (qmlimportscanner falla con ellas en Windows). El escaneo de imports
    # es imprescindible para el build estático de WebAssembly.
    QML_FILES
        qml/Main.qml
        qml/NavPanel.qml
        qml/SimulacionSimpleView.qml
        qml/DatosBaseView.qml
        qml/FinanciacionView.qml
        qml/ProyeccionView.qml
        qml/ImpuestosView.qml
        qml/AnalisisView.qml
        qml/AmortView.qml
        qml/PersonalView.qml
        qml/MoneyField.qml
        qml/PctField.qml
        qml/NumField.qml
        qml/SectionTitle.qml
        qml/KeyboardAvoider.qml
        qml/Fmt.qml
        qml/Nav.qml
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
)

# El código de registro QML generado incluye los headers por su nombre base
target_include_directories(FarmaciaSim PRIVATE src)

target_link_libraries(FarmaciaSim PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Svg)

set_target_properties(FarmaciaSim PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

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
