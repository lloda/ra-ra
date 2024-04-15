# -*- coding: utf-8; mode: cmake -*-
# Cmake file for ra-ra/docs

# (c) Daniel Llorens - 2018-2024
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

cmake_minimum_required (VERSION 3.5)
project (ra-docs)

if (NOT "$ENV{MAKEINFO}" STREQUAL "")
  set(MAKEINFO_EXECUTABLE "$ENV{MAKEINFO}" CACHE INTERNAL "Copied from environment variable")
else()
  find_program(MAKEINFO_EXECUTABLE makeinfo)
endif()
message ("Using makeinfo : ${MAKEINFO_EXECUTABLE}")

set (man_in ${CMAKE_CURRENT_SOURCE_DIR}/ra-ra.texi)
set (man_out ${CMAKE_CURRENT_BINARY_DIR}/index.html)

add_custom_command (OUTPUT ${man_out}
  COMMAND ${MAKEINFO_EXECUTABLE} --html --no-split ${man_in} -o ${man_out} -c HTML_MATH=mathjax
  DEPENDS ${man_in}
  COMMENT "Creating HTML file ${man_out}"
  VERBATIM)

add_custom_target (docs ALL DEPENDS ${man_out})
