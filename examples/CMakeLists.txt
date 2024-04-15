# -*- coding: utf-8; mode: cmake -*-
# Cmake file for ra/examples

# (c) Daniel Llorens - 2016-2024
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

cmake_minimum_required (VERSION 3.5)
project (ra-examples)
include_directories ("..")

SET (TARGETS agreement cast deriv explode-collapse indirect laplace2d laplace3d
  maxwell nested newton outer rangexpr read-me slicing small useret view where-pick
  throw)

include ("../config/cc.cmake")
