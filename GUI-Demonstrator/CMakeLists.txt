cmake_minimum_required(VERSION 3.16)

project(Controller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AUTOMOC erzeugt den Meta-Object-Code fuer Klassen mit Q_OBJECT,
# AUTOUIC macht aus Controller.ui die ui_Controller.h.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Die von AUTOUIC erzeugte ui_Controller.h liegt im Build-Ordner und enthaelt
# #include "plotview.h", "motordial.h" und "svglogo.h" (Widget-Promotion). Ohne
# diese Zeile findet der Compiler die Header nicht.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Svg rendert Logo und Wortmarke scharf in jeder Groesse.
# Auf dem Pi: sudo apt install qt6-svg-dev
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

qt_standard_project_setup()

qt_add_executable(Controller
    main.cpp
    controller.cpp
    controller.h
    Controller.ui
    plotview.cpp
    plotview.h
    knobinput.cpp
    knobinput.h
    motordial.cpp
    motordial.h
    svglogo.cpp
    svglogo.h
)

# Logos und Schriften werden in die Binary eingebaut. So findet das Programm
# sie unabhaengig davon, aus welchem Verzeichnis es gestartet wird.
qt_add_resources(Controller "brand"
    PREFIX "/"
    FILES
        logo/ATHENE_wordmark_weiss.svg
        logo/DARE-ROBOTICS_logo-horizontal_dunkel.svg
        logo/DARE-ROBOTICS_icon_weiss.svg
        fonts/Oswald-Bold.ttf
        fonts/Oswald-Light.ttf
)

target_link_libraries(Controller PRIVATE Qt6::Widgets Qt6::Svg)

# Unter Windows/macOS kein Konsolenfenster im Hintergrund
set_target_properties(Controller PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
