# Install script for directory: C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/muffmode/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/src/1.9.5-13b47286ba.clean/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/muffmode/src/vcpkg_installed/x64-windows-static/vcpkg/pkgs/jsoncpp_x64-windows-static/debug")
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
  set(CMAKE_CROSSCOMPILING "OFF")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/muffmode/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-dbg/src/lib_json/cmake_install.cmake")

endif()

