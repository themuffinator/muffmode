# Install script for directory: C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/jsoncpp/src/3918c327b1-034a82149a.clean/src/lib_json

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/pkgs/jsoncpp_x64-windows/debug")
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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/jsoncpp/x64-windows-dbg/lib/jsoncpp.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/jsoncpp/x64-windows-dbg/bin/jsoncpp.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/objects-Debug/jsoncpp_object" TYPE FILE FILES
    "json_reader.cpp.obj"
    "json_value.cpp.obj"
    "json_writer.cpp.obj"
    FILES_FROM_DIR "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/jsoncpp/x64-windows-dbg/src/lib_json/CMakeFiles/jsoncpp_object.dir/./")
endif()

