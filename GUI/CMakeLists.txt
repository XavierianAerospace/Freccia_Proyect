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
    Network
    OpenGLWidgets
)

find_package(OpenGL REQUIRED)

# FFmpeg configuration (Manual lookup if PkgConfig fails)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG IMPORTED_TARGET
        libavcodec
        libavformat
        libswscale
        libavutil
    )
    if(FFMPEG_FOUND)
        add_definitions(-DHAS_FFMPEG)
    endif()
endif()

if(NOT FFMPEG_FOUND)
    message(STATUS "FFmpeg not found via PkgConfig, attempting manual search...")

    # Common paths for MinGW/Windows and Linux
    set(FFMPEG_PATHS
        "C:/ffmpeg"
        "C:/ffmpeg/include"
        "/usr/local/include"
        "/usr/include"
        "/usr/include/x86_64-linux-gnu"
    )

    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h PATHS ${FFMPEG_PATHS})
    find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_PATHS})
    find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_PATHS})
    find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_PATHS})
    find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_PATHS})

    if(AVCODEC_INCLUDE_DIR AND AVCODEC_LIBRARY)
        set(FFMPEG_FOUND TRUE)
        add_definitions(-DHAS_FFMPEG)
        add_library(FFMPEG::FFMPEG INTERFACE IMPORTED)
        set_target_properties(FFMPEG::FFMPEG PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${AVCODEC_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${AVCODEC_LIBRARY};${AVFORMAT_LIBRARY};${SWSCALE_LIBRARY};${AVUTIL_LIBRARY}"
        )
    endif()
endif()

qt_standard_project_setup()

qt_add_executable(GUI_FRECCIA
  Source_Files/main.cpp
  Source_Files/widget.cpp
  Source_Files/SensorManager.cpp
  Source_Files/SerialReader.cpp
  Source_Files/Graph3DWindow.cpp
  Source_Files/InicioWindow.cpp
  Source_Files/WindowManager.cpp
  Source_Files/TopToolbar.cpp
  Source_Files/CameraWidget.cpp
  Source_Files/VideoManager.cpp
  Source_Files/VideoDecoder.cpp
  Source_Files/VideoSubsystem.cpp

  Source_Files/data/DataCleaner.cpp
  Source_Files/data/FileHelper.cpp
  Source_Files/data/DataTopic.cpp

  Header_Files/widget.h
  Header_Files/SensorManager.h
  Header_Files/SerialReader.h
  Header_Files/Graph3DWindow.h
  Header_Files/InicioWindow.h
  Header_Files/WindowManager.h
  Header_Files/TopToolbar.h
  Header_Files/CameraWidget.h
  Header_Files/VideoManager.h
  Header_Files/VideoDecoder.h
  Header_Files/VideoSubsystem.h

  Header_Files/data/DataCleaner.h
  Header_Files/data/FileHelper.h
  Header_Files/data/SensorData.h
  Header_Files/data/DataTopic.h
  Header_Files/data/RangeChecker.h
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
    Qt::Network
    Qt::OpenGLWidgets
    OpenGL::GL
)

if(FFMPEG_FOUND)
    if(TARGET PkgConfig::FFMPEG)
        target_link_libraries(GUI_FRECCIA PUBLIC PkgConfig::FFMPEG)
    else()
        target_link_libraries(GUI_FRECCIA PUBLIC FFMPEG::FFMPEG)
    endif()
endif()

install(TARGETS GUI_FRECCIA
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
