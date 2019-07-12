# -*- coding: utf-8; mode: cmake -*-
# (c) Daniel Llorens - 2018

cmake_minimum_required (VERSION 3.5)
project (ra-box)
include_directories ("..")

SET (TARGETS view-flat tuple-construct end iterator-as-ravel)

include ("../config/cc.cmake")
