# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.0)
project (ra-doc)

find_program (MAKEINFO_EXECUTABLE makeinfo)

set (man_in ${CMAKE_CURRENT_SOURCE_DIR}/ra-ra.texi)
set (man_out ${CMAKE_CURRENT_BINARY_DIR}/ra-ra.html)

add_custom_command (OUTPUT ${man_out}
  COMMAND ${MAKEINFO_EXECUTABLE} --html --no-split < ${man_in} > ${man_out}
  DEPENDS ${man_in}
  COMMENT "Creating HTML doc file ${info_out}"
  VERBATIM)

add_custom_target (doc ALL DEPENDS ${man_out})
