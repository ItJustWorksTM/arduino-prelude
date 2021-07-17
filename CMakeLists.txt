#
#  CMakeLists.txt
#  Copyright 2021 ItJustWorksTM
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

cmake_minimum_required (VERSION 3.13.4)
cmake_policy (SET CMP0022 NEW)

project (ardpp VERSION 0.1.0)

add_executable (ardpp)
target_compile_features (ardpp PUBLIC cxx_std_17)
target_sources (ardpp PRIVATE ardpp.cpp)

if (NOT MSVC)
  target_compile_options (ardpp PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-Werror" "-Wcast-align")
  if ("${SMCE_CXXRT_LINKING}" STREQUAL "STATIC" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_options (ardpp PRIVATE -static-libstdc++ -static-libgcc)
    #FIXME handle Clang/libc++
  endif ()
else ()
  target_compile_options (ardpp PRIVATE "/W4" "/permissive-" "/wd4244" "/wd4459" "/WX")
endif ()

if (DEFINED SLIBCLANG_DIR)
  find_package (Threads REQUIRED)
  target_include_directories (ardpp PRIVATE "${SLIBCLANG_DIR}/include")
  target_link_directories (ardpp PRIVATE "${SLIBCLANG_DIR}/lib")
  target_link_libraries (ardpp PUBLIC clang_bundled dl z Threads::Threads)
  if (NOT MSVC)
    target_link_options (ardpp PUBLIC -s -Wl,--gc-sections)
    target_compile_options (ardpp PUBLIC -Os -ffunction-sections -fdata-sections)
  endif ()
else ()
  find_package (LLVM 12 REQUIRED CONFIG)
  message (STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message (STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR} (${LLVM_INCLUDE_DIRS})")

  separate_arguments (LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
  llvm_map_components_to_libnames (LLVM_LIBS support core)

  target_compile_definitions (ardpp PRIVATE ${LLVM_DEFINITIONS_LIST})
  target_include_directories (ardpp PRIVATE ${LLVM_INCLUDE_DIRS})

  target_link_directories (ardpp PRIVATE ${LLVM_LIBRARY_DIRS})
  target_link_libraries (ardpp PUBLIC ${LLVM_LIBS} clang)
endif ()
