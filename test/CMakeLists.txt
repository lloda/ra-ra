# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.0)
project (ra-test)
include_directories ("..")

# FIXME add... test-end

SET (TARGETS test-bench test-big-0 test-big-1 test-check-runtime test-compatibility
  test-concrete test-dual test-explode-collapse test-frame-match test-from test-io
  test-iterator-small test-mem-fn test-nested test-operators test-optimize
  test-owned test-ownership test-ply test-ra-0 test-ra-10 test-ra-1 test-ra-2
  test-ra-3 test-ra-4 test-ra-6 test-ra-7 test-ra-8 test-ra-9 test-ra-dual
  test-reduction test-reshape test-return-expr test-small-0 test-small-1
  test-stl-compat test-tensorindex test-tuplelist test-types test-wedge-product
  test-where test-wrank)

include ("../config/cc.cmake")
