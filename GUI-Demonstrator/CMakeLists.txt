cmake_minimum_required(VERSION 3.16)

project(Controller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AUTOMOC erzeugt den Meta-Object-Code fuer Klassen mit Q_OBJECT,
# AUTOUIC macht aus Controller.ui die ui_Controller.h.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Die von AUTOUIC erzeugte ui_Controller.h liegt im Build-Ordner und enthaelt
# #include "plotview.h" (wegen der Widget-Promotion). Ohne diese Zeile findet
# der Compiler den Header nicht.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(Controller
    main.cpp
    controller.cpp
    controller.h
    Controller.ui
    plotview.cpp
    plotview.h
    demorun.cpp
    demorun.h
)

target_link_libraries(Controller PRIVATE Qt6::Widgets)

# Unter Windows/macOS kein Konsolenfenster im Hintergrund
set_target_properties(Controller PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
