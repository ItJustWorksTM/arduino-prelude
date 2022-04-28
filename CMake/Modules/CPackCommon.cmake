#
#  CPackCommon.cmake
#  Copyright 2021-2022 ItJustWorksTM
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

include_guard ()

set (CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if (DEFINED CMAKE_OSX_ARCHITECTURES)
  list (LENGTH CMAKE_OSX_ARCHITECTURES osx_arch_count)
  if (osx_arch_count GREATER 1)
    set (CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-universal")
  endif ()
endif ()

set (CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set (CPACK_PACKAGE_VENDOR "ItJustWorksTM")
set (CPACK_PACKAGE_CONTACT "${CPACK_PACKAGE_VENDOR} <itjustworkstm@aerostun.dev>")
set (CMAKE_PROJECT_DESCRIPTION "SMCE's replacement for Arduino's ctags-based preprocessor")
set (CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/ItJustWorksTM/arduino-prelude")
set (CPACK_PACKAGE_DESCRIPTION "LLVM-based prelude generator for Arduino sketches.")
