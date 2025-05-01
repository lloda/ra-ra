# -*- coding: utf-8; mode: cmake -*-
# Cmake file for ra/box

# (c) Daniel Llorens - 2018-2024
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

cmake_minimum_required (VERSION 3.10)
project (ra-box)
include_directories ("..")

SET (TARGETS view-flat tuple-construct end iterator-as-ravel)

include ("../config/cc.cmake")
