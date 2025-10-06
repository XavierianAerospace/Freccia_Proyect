cmake_minimum_required(VERSION 3.16)
project(GUI_FRECCIA LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/charts/GUI_FRECCIA")

find_package(Qt6 REQUIRED COMPONENTS
    Charts
    Core
    Gui
    Widgets
    SerialPort
    DataVisualization
    3DCore
    3DExtras
    3DRender
    3DInput
    3DLogic
    3DAnimation
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

qt_standard_project_setup()

qt_add_executable(GUI_FRECCIA
  Source_Files/main.cpp
  Source_Files/widget.cpp
  Source_Files/SensorManager.cpp
  Source_Files/SerialReader.cpp
  Source_Files/Graph3DWindow.cpp
  Source_Files/InicioWindow.cpp

  Source_Files/data/DataCleaner.cpp
  Source_Files/data/FileHelper.cpp

  Header_Files/widget.h
  Header_Files/SensorManager.h
  Header_Files/SerialReader.h
  Header_Files/Graph3DWindow.h
  Header_Files/InicioWindow.h

  Header_Files/data/DataCleaner.h
  Header_Files/data/FileHelper.h
  Header_Files/data/SensorData.h
)

configure_file(
    ${CMAKE_SOURCE_DIR}/assets/logo_xae.png
    ${CMAKE_BINARY_DIR}/assets/logo_xae.png
    COPYONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/assets/Inicio.gif
    ${CMAKE_BINARY_DIR}/assets/Inicio.gif
    COPYONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/assets/Menu.png
    ${CMAKE_BINARY_DIR}/assets/Menu.png
    COPYONLY
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets/model)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Header_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/Header_Files/data
)

set_target_properties(GUI_FRECCIA PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/apple/Info.cmake.plist"
)

target_link_libraries(GUI_FRECCIA PUBLIC
    Qt::Charts
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::SerialPort
    Qt::DataVisualization
    Qt::3DCore
    Qt::3DExtras
    Qt::3DRender
    Qt::3DInput
    Qt::3DLogic
    Qt::3DAnimation
)

install(TARGETS GUI_FRECCIA
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Copiar recursivamente la carpeta PyWindow al directorio de build
file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/Source_Files/PyWindow
    DESTINATION ${CMAKE_BINARY_DIR}/Source_Files
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source_Files/PyWindow
    DESTINATION "${INSTALL_EXAMPLEDIR}/Source_Files"
    USE_SOURCE_PERMISSIONS
)

file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/Source_Py/
    DESTINATION ${CMAKE_BINARY_DIR}/Source_Py
)

install(DIRECTORY 
    ${CMAKE_CURRENT_SOURCE_DIR}/Source_Py/
    DESTINATION "${INSTALL_EXAMPLEDIR}/Source_Py"
    USE_SOURCE_PERMISSIONS
)