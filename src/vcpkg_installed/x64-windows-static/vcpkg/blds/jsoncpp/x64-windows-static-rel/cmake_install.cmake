# Install script for directory: C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/src/3918c327b1-034a82149a.clean

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/pkgs/jsoncpp_x64-windows-static")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp/jsoncppConfig.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp/jsoncppConfig.cmake"
         "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/CMakeFiles/Export/8728d72a6b86aa6ce8d1b9aa67305bd2/jsoncppConfig.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp/jsoncppConfig-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp/jsoncppConfig.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp" TYPE FILE FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/CMakeFiles/Export/8728d72a6b86aa6ce8d1b9aa67305bd2/jsoncppConfig.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp" TYPE FILE FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/CMakeFiles/Export/8728d72a6b86aa6ce8d1b9aa67305bd2/jsoncppConfig-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/jsoncpp" TYPE FILE FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/jsoncppConfigVersion.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/src/cmake_install.cmake")
  include("C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/include/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/x64-windows-static/vcpkg/blds/jsoncpp/x64-windows-static-rel/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
