# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.0)
project (ra-examples)
include_directories ("..")

# FIXME add... test-end

SET (TARGETS agreement cast deriv explode-collapse indirect laplace2d laplace3d
  maxwell nested newton outer rangexpr readme slicing small useret view where)

include ("../config/cc.cmake")
