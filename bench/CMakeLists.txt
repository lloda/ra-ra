# -*- mode: cmake -*-
# -*- coding: utf-8 -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.5)
project (ra-bench)
include_directories ("..")

SET (TARGETS bench-dot bench-from bench-gemm bench-gemv bench-optimize bench-pack bench-reduce-sqrm
  bench-stencil1 bench-stencil2 bench-stencil3 bench-sum-cols bench-sum-rows)

include ("../config/cc.cmake")

find_package (CBLAS)
if (CBLAS_FOUND)
  target_compile_definitions (bench-gemm PRIVATE "-DRA_USE_BLAS=1")
  target_include_directories (bench-gemm PRIVATE ${CBLAS_INCLUDE_DIRS})
  target_link_libraries (bench-gemm ${CBLAS_LIBRARIES})
endif ()
