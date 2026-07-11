qt_add_executable(farmaciasim_tests
    test_simcore.cpp
)

target_link_libraries(farmaciasim_tests PRIVATE Qt6::Test farmaciasim_core)

add_test(NAME farmaciasim_tests COMMAND farmaciasim_tests)

# ------------------------------------------------------------- Tests del PDF
qt_add_executable(farmaciasim_pdf_tests
    test_pdfreport.cpp
)

target_link_libraries(farmaciasim_pdf_tests PRIVATE Qt6::Test farmaciasim_pdf)

add_test(NAME farmaciasim_pdf_tests COMMAND farmaciasim_pdf_tests)

# ------------------------------------------------------------- Tests QML
qt_add_executable(farmaciasim_qml_tests
    qml_test_main.cpp
)

target_link_libraries(farmaciasim_qml_tests PRIVATE farmaciasim_ui farmaciasim_uiplugin Qt6::QuickTest)

# Directorio de salida propio: Engine escribe farmaciasim_datos.json junto al
# ejecutable, y no debe coincidir con el de FarmaciaSim.exe (evita pisar los
# datos reales de la app al correr los tests).
set_target_properties(farmaciasim_qml_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml-tests-bin
)

add_test(NAME farmaciasim_qml_tests
    COMMAND farmaciasim_qml_tests -input ${CMAKE_CURRENT_SOURCE_DIR}/qml
)
