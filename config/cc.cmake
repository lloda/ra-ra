# -*- coding: utf-8; mode: cmake -*-
# (c) Daniel Llorens - 2018-2019
# Common include section for subdirs

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config/")

set (BASE_CXXFLAGS "-std=c++2b -Wall -Werror -fdiagnostics-color=always -Wno-unknown-pragmas \
-finput-charset=UTF-8 -fextended-identifiers -Wno-error=strict-overflow \
-Werror=zero-as-null-pointer-constant")
set (CMAKE_CXX_FLAGS "${BASE_CXXFLAGS} $ENV{CXXFLAGS}")

foreach (target ${TARGETS})
  add_executable (${target} "${target}.cc")
  add_test (${target} ${target})
endforeach ()

enable_testing ()

message ("* C++ compiler is: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
