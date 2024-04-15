# -*- coding: utf-8; mode: cmake -*-
# Top cmake file for ra-ra

# (c) Daniel Llorens - 2018-2019
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

# here or in any of the subdirectories
# CXXFLAGS=-O3 cmake .
# VERBOSE=1 make -j7
# VERBOSE=1 make -j7 test

cmake_minimum_required (VERSION 3.5)
project (ra-ra)

foreach (dir test bench examples docs)
  add_subdirectory (${dir})
endforeach ()

enable_testing ()
