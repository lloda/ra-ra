# -*- coding: utf-8; mode: cmake -*-
# Cmake file for ra/test

# (c) Daniel Llorens - 2018-2025
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

cmake_minimum_required (VERSION 3.10)
project (ra-test)
include_directories ("..")

SET (TARGETS at bench big-0 big-1 bug83 bug10 checks compatibility concrete const constexpr dual
  early explode-0 foreign frame-new frame-old fromb fromu headers io iota iterator-small len
  list9 macros mem-fn nested-0 operators optimize owned ownership ply ra-0 ra-1 ra-10 ra-11
  ra-12 ra-13 ra-14 ra-15 ra-2 ra-3 ra-4 ra-5 ra-6 ra-8 ra-9 ra-dual reduction
  reexported reshape return-expr self-assign sizeof small-0 small-1 stl-compat swap tensorindex
  tuples types vector-array view-ops wedge where wrank)

include ("../config/cc.cmake")
