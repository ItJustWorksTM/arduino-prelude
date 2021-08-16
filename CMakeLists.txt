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
cmake_policy (SET CMP0091 NEW)

project (arduino-prelude VERSION 1.0.0)

add_executable (arduino-prelude)
target_compile_features (arduino-prelude PUBLIC cxx_std_20)
target_sources (arduino-prelude PRIVATE arduino-prelude.cpp)
target_compile_definitions (arduino-prelude PRIVATE "ARDUINO_PRELUDE_VERSION=\"${PROJECT_VERSION}\"")

if (NOT MSVC)
  target_compile_options (arduino-prelude PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-Werror" "-Wcast-align")
  if ("${ARDPRE_CXXRT_LINKING}" STREQUAL "STATIC" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_options (arduino-prelude PRIVATE -static-libstdc++ -static-libgcc)
    #FIXME handle Clang/libc++
  endif ()
else ()
  target_compile_options (arduino-prelude PRIVATE "/W4" "/permissive-" "/wd4244" "/wd4459" "/WX")
  if ("${ARDPRE_CXXRT_LINKING}" STREQUAL "STATIC")
    set_property (TARGET arduino-prelude PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif ()
endif ()

if (DEFINED SLIBCLANG_DIR)
  find_package (Threads REQUIRED)
  target_include_directories (arduino-prelude PRIVATE "${SLIBCLANG_DIR}/include")
  target_link_directories (arduino-prelude PRIVATE "${SLIBCLANG_DIR}/lib")
  target_link_libraries (arduino-prelude PUBLIC Threads::Threads)
  if (WIN32)
    target_compile_definitions (arduino-prelude PUBLIC _CINDEX_LIB_)
    target_link_libraries (arduino-prelude PUBLIC clang_static_bundled Version)
  else ()
    target_link_libraries (arduino-prelude PUBLIC clang_bundled dl z)
  endif ()

  if (NOT MSVC)
    if (APPLE)
      target_link_options (arduino-prelude PUBLIC -Wl,-dead_strip)
    else ()
      target_link_options (arduino-prelude PUBLIC -s -Wl,--gc-sections)
    endif ()
    target_compile_options (arduino-prelude PUBLIC -Os -ffunction-sections -fdata-sections)
  endif ()
else ()
  find_package (LLVM 12 REQUIRED CONFIG)
  message (STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message (STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR} (${LLVM_INCLUDE_DIRS})")

  separate_arguments (LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
  llvm_map_components_to_libnames (LLVM_LIBS support core)

  target_compile_definitions (arduino-prelude PRIVATE ${LLVM_DEFINITIONS_LIST})
  target_include_directories (arduino-prelude PRIVATE ${LLVM_INCLUDE_DIRS})

  target_link_directories (arduino-prelude PRIVATE ${LLVM_LIBRARY_DIRS})
  target_link_libraries (arduino-prelude PUBLIC ${LLVM_LIBS} clang)
endif ()

install (TARGETS arduino-prelude
    RUNTIME DESTINATION .
)

include (CPack)
