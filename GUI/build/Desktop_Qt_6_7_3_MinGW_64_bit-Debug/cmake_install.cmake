# Install script for directory: C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/GUI_FRECCIA")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/Qt/Tools/mingw1120_64/bin/objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA" TYPE EXECUTABLE FILES "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/build/Desktop_Qt_6_7_3_MinGW_64_bit-Debug/GUI_FRECCIA.exe")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA/GUI_FRECCIA.exe" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA/GUI_FRECCIA.exe")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "C:/Qt/Tools/mingw1120_64/bin/strip.exe" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA/GUI_FRECCIA.exe")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA/Source_Files" TYPE DIRECTORY FILES "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Files/PyWindow" USE_SOURCE_PERMISSIONS)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA/Source_Py" TYPE FILE OPTIONAL FILES
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/build.spec"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/client_viewer.py"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/client_viewer2.py"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/CoheteGUI.STL"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/FRECCIA_XAE.spec"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/Instrucciones.txt"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/stl_rotator_pyglet.py"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/charts/GUI_FRECCIA/Source_Py" TYPE DIRECTORY OPTIONAL FILES
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/dist"
    "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/Source_Py/Map"
    USE_SOURCE_PERMISSIONS)
endif()

if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
  file(WRITE "C:/Users/santi/Documents/GitHub/Freccia_Proyect/GUI/build/Desktop_Qt_6_7_3_MinGW_64_bit-Debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
