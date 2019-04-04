# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.5)
project (ra-examples)
include_directories ("..")

SET (TARGETS agreement cast deriv explode-collapse indirect laplace2d laplace3d
  maxwell nested newton outer rangexpr readme slicing small useret view where-pick
  throw)

include ("../config/cc.cmake")
