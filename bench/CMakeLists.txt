# -*- coding: utf-8; mode: cmake -*-
# Cmake file for ra/bench

# (c) Daniel Llorens - 2018-2024
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

cmake_minimum_required (VERSION 3.10)
project (ra-bench)
include_directories ("..")

SET (TARGETS bench-dot bench-from bench-gemm bench-gemv bench-optimize bench-pack bench-reduce-sqrm
  bench-stencil1 bench-stencil2 bench-stencil3 bench-sum-cols bench-sum-rows bench-tensorindex
  bench-at bench-iterator)

include ("../config/cc.cmake")

find_package (CBLAS)
if (CBLAS_FOUND)
  target_compile_definitions (bench-gemm PRIVATE "-DRA_USE_BLAS=1")
  target_include_directories (bench-gemm PRIVATE ${CBLAS_INCLUDE_DIRS})
  target_link_libraries (bench-gemm ${CBLAS_LIBRARIES})
endif ()
