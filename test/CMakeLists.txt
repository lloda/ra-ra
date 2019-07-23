# -*- coding: utf-8; mode: cmake -*-
# (c) Daniel Llorens - 2018, 2019

cmake_minimum_required (VERSION 3.5)
project (ra-test)
include_directories ("..")

SET (TARGETS bench big-0 big-1 check-runtime compatibility concrete dual explode-0 frame-old
  frame-new from io iterator-small mem-fn const nested-0 operators optimize owned ownership ply old
  ra-0 ra-10 ra-1 ra-2 ra-3 ra-4 ra-5 ra-6 ra-7 ra-8 ra-9 ra-11 ra-dual reduction reshape
  return-expr small-0 small-1 stl-compat tensorindex tuplelist types wedge where wrank list9 early
  macros)

include ("../config/cc.cmake")
