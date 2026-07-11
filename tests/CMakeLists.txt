qt_add_executable(farmaciasim_tests
    test_simcore.cpp
)

target_link_libraries(farmaciasim_tests PRIVATE Qt6::Test farmaciasim_core)

add_test(NAME farmaciasim_tests COMMAND farmaciasim_tests)
