# -*- mode: Python -*-
# -*- coding: utf-8 -*-

# (c) Daniel Llorens - 2016

# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

# Top SConstruct for ra-ra
# todo Shared pieces with examples/SConstruct and test/SConstruct

import os, atexit
from colorama import Fore, Back, Style
from os.path import join, abspath
from SCons.Script import GetBuildFailures
import imp
ra = imp.load_source('ra', './ra.py')

top = {'skip_summary': True}; Export('top');
SConscript('test/SConstruct', 'top')
SConscript('bench/SConstruct', 'top')
SConscript('examples/SConstruct', 'top')
# SConscript('doc/SConstruct', 'top') # TODO run from doc, otherwise makeinfo writes empty files (??)

atexit.register(lambda: ra.print_summary(GetBuildFailures, 'ra-ra'))
