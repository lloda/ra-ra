# -*- coding: utf-8; mode: cmake -*-
# Common cmake section for subdirs

# (c) Daniel Llorens - 2018-2024
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config/")

set (SANITIZE 1
  CACHE BOOL "Build with sanitizers.")

set (BASE_CXXFLAGS "-std=c++2b -Wall -Werror -fdiagnostics-color=always -Wno-unknown-pragmas \
-finput-charset=UTF-8 -fextended-identifiers -Wno-error=strict-overflow \
-Werror=zero-as-null-pointer-constant")

if (SANITIZE)
  set (BASE_CXXFLAGS "${BASE_CXXFLAGS} -fsanitize=address,leak,undefined")
endif()

set (CMAKE_CXX_FLAGS "${BASE_CXXFLAGS} $ENV{CXXFLAGS}")

foreach (target ${TARGETS})
  add_executable (${target} "${target}.cc")
  add_test (${target} ${target})
endforeach ()

enable_testing ()

message ("* C++ compiler is: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message ("* C++ flags are: ${CMAKE_CXX_FLAGS}")
