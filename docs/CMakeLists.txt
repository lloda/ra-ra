# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.0)
project (ra-docs)

find_program (MAKEINFO_EXECUTABLE makeinfo)

set (man_in ${CMAKE_CURRENT_SOURCE_DIR}/ra-ra.texi)
set (man_out ${CMAKE_CURRENT_BINARY_DIR}/index.html)

add_custom_command (OUTPUT ${man_out}
  COMMAND ${MAKEINFO_EXECUTABLE} --html --no-split ${man_in} -o ${man_out}
  DEPENDS ${man_in}
  COMMENT "Creating HTML file ${man_out}"
  VERBATIM)

add_custom_target (docs ALL DEPENDS ${man_out})
