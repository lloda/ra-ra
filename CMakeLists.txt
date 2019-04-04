# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018-2019

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
