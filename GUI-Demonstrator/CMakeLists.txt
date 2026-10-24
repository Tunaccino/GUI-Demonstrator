cmake_minimum_required(VERSION 3.16)

project(Controller LANGUAGES C CXX)

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

# Svg rendert Logo und Wortmarke scharf in jeder Groesse, SerialPort spricht
# mit den Boards. Auf dem Pi: sudo apt install qt6-svg-dev qt6-serialport-dev
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg SerialPort)

# Protokoll zur Firmware: interface.h und checksum16.c aus pd-demonstrator-interface.
# Vorgabe ist das Repo neben diesem, sonst beim Konfigurieren angeben:
#   cmake -S . -B build -DSERIAL_INTERFACE_DIR=/pfad/zu/pd-demonstrator-interface
set(SERIAL_INTERFACE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../Demonstrator/pd-demonstrator-interface"
    CACHE PATH "Verzeichnis von pd-demonstrator-interface")
if(NOT EXISTS "${SERIAL_INTERFACE_DIR}/interface.h")
    message(FATAL_ERROR "interface.h nicht gefunden in ${SERIAL_INTERFACE_DIR}, "
                        "bitte -DSERIAL_INTERFACE_DIR=... angeben")
endif()

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
    boardconnection.cpp
    boardconnection.h
    ${SERIAL_INTERFACE_DIR}/checksum16.c
)
target_include_directories(Controller PRIVATE "${SERIAL_INTERFACE_DIR}")

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

target_link_libraries(Controller PRIVATE Qt6::Widgets Qt6::Svg Qt6::SerialPort)

# Unter Windows/macOS kein Konsolenfenster im Hintergrund
set_target_properties(Controller PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
