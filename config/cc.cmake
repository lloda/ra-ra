# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018
# Common include section for subdirs

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config/")

set (BASE_CXXFLAGS "-std=c++1z -Wall -Werror -fdiagnostics-color=always -Wno-unknown-pragmas \
-finput-charset=UTF-8 -fextended-identifiers -Wno-error=strict-overflow \
-Werror=zero-as-null-pointer-constant")
set (CMAKE_CXX_FLAGS "${BASE_CXXFLAGS} $ENV{CXXFLAGS}")

foreach (target ${TARGETS})
  add_executable (${target} "${target}.C")
  add_test (${target} ${target})
endforeach ()

enable_testing ()
