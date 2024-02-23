# Install script for directory: C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/pkgs/fmt_x64-windows")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/fmt" TYPE FILE FILES
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/fmt-config.cmake"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/fmt-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/fmt/fmt-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/fmt/fmt-targets.cmake"
         "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/CMakeFiles/Export/2a5b8b7bf6498a831bcdfe56de7404e6/fmt-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/fmt/fmt-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/fmt/fmt-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/fmt" TYPE FILE FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/CMakeFiles/Export/2a5b8b7bf6498a831bcdfe56de7404e6/fmt-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/fmt" TYPE FILE FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/CMakeFiles/Export/2a5b8b7bf6498a831bcdfe56de7404e6/fmt-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/fmt.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/bin/fmt.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE OPTIONAL FILES
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/$<TARGET_PDB_FILE:fmt"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/fmt-header-only>"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/fmt" TYPE FILE FILES
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/chrono.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/color.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/compile.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/core.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/format.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/format-inl.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/locale.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/os.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/ostream.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/posix.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/printf.h"
    "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/src/2b3b374f0b-8ea71ee8ea.clean/include/fmt/ranges.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/fmt.pc")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/Program Files (x86)/Steam/steamapps/common/Quake 2/rerelease/mymod/src/vcpkg_installed/vcpkg/blds/fmt/x64-windows-rel/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
