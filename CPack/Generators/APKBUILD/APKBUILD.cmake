#
#  APKBUILD.cmake
#  Copyright 2022 ItJustWorksTM
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

cmake_minimum_required (VERSION 3.16)
message (DEBUG "CPack APKBUILD Packager v0.1")

if (NOT DEFINED APKBUILD_PACKAGE_NAME)
  set (APKBUILD_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
endif ()
if (NOT DEFINED APKBUILD_PACKAGE_VERSION)
  set (APKBUILD_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
endif ()
if (NOT DEFINED APKBUILD_PACKAGE_RELEASE)
  set (APKBUILD_PACKAGE_RELEASE 0)
endif ()
if (NOT DEFINED APKBUILD_PACKAGE_DESCRIPTION)
  set (APKBUILD_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
endif ()
string (REPLACE "\n" "\\n" APKBUILD_PACKAGE_DESCRIPTION "${APKBUILD_PACKAGE_DESCRIPTION}")
if (NOT DEFINED APKBUILD_PACKAGE_HOMEPAGE)
  set (APKBUILD_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
endif ()
if (NOT DEFINED APKBUILD_PACKAGE_ARCHITECTURE)
  set (APKBUILD_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif ()
if (NOT DEFINED APKBUILD_PACKAGE_LICENSE)
  set (APKBUILD_PACKAGE_LICENSE "unknown")
endif()
if (NOT DEFINED APKBUILD_PACKAGE_DEPENDS)
  set (APKBUILD_PACKAGE_DEPENDS "")
endif()

set (SRCDIR ".")
set (BUILDDIR "$srcdir/build")
configure_file ("${CMAKE_CURRENT_LIST_DIR}/APKBUILD.in" "${CPACK_PACKAGE_DIRECTORY}/APKBUILD" @ONLY)

if (NOT DEFINED APKBUILD_GENERATE_PACKAGE)
  set (APKBUILD_GENERATE_PACKAGE Yes)
endif ()
if (NOT APKBUILD_GENERATE_PACKAGE)
  return ()
endif ()


list (GET CPACK_BUILD_SOURCE_DIRS 0 CPACK_SOURCE_DIR)
list (GET CPACK_BUILD_SOURCE_DIRS 1 CPACK_BUILD_DIR)
file (MAKE_DIRECTORY "${CPACK_BUILD_DIR}/cpack-abuild")
set (SRCDIR "${CPACK_SOURCE_DIR}")
set (BUILDDIR "${CPACK_BUILD_DIR}")
configure_file ("${CMAKE_CURRENT_LIST_DIR}/APKBUILD.in" "${CPACK_BUILD_DIR}/cpack-abuild/APKBUILD" @ONLY)

find_program (ID_EXECUTABLE id)
if (NOT ID_EXECUTABLE)
  message (FATAL_ERROR "Unable to detect current user id (root check)")
endif ()

execute_process (COMMAND "${ID_EXECUTABLE}" -u OUTPUT_VARIABLE uid)

if (uid EQUAL 0)
  set (abuild_maybe_root -F)
else ()
  set (abuild_maybe_root)
endif ()

find_program (ABUILD_EXECUTABLE abuild REQUIRED)
execute_process (
    COMMAND "${ABUILD_EXECUTABLE}" -P "${CPACK_TEMPORARY_DIRECTORY}" ${abuild_maybe_root} rootpkg
    WORKING_DIRECTORY "${CPACK_BUILD_DIR}/cpack-abuild"
)

set (CPACK_EXTERNAL_BUILT_PACKAGES)
file (GLOB_RECURSE package_files "${CPACK_TEMPORARY_DIRECTORY}/*.apk")
foreach (package_file IN LISTS package_files)
  file (COPY "${package_file}" DESTINATION "${CPACK_PACKAGE_DIRECTORY}")
  get_filename_component (package_filename "${package_file}" NAME)
  list (APPEND CPACK_EXTERNAL_BUILT_PACKAGES "${CPACK_PACKAGE_DIRECTORY}/${package_filename}")
endforeach ()
