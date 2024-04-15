# -*- coding: utf-8; mode: Python -*-
# Top SConstruct for ra-ra

# (c) Daniel Llorens - 2016-2024
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

import os, atexit
from os.path import join, abspath
from SCons.Script import GetBuildFailures
import imp

# Make sure to disable (-no-sanitize) for benchmarks.
AddOption('--no-sanitize', default=True, action='store_false', dest='sanitize', help='Disable sanitizer flags.')
AddOption('--sanitize', default=True, action='store_true', dest='sanitize', help='Enable sanitizer flags.')
AddOption('--blas', default=False, action='store_true', dest='use_blas', help='Try to use BLAS (benchmarks only).')
top = {'skip_summary': True, 'sanitize': GetOption('sanitize'), 'use_blas': GetOption('use_blas')}
Export('top');

ra = imp.load_source('ra', 'config/ra.py')

SConscript('test/SConstruct', 'top')
SConscript('bench/SConstruct', 'top')
SConscript('examples/SConstruct', 'top')
SConscript('box/SConstruct', 'top')
# SConscript('docs/SConstruct', 'top') # TODO run from docs, otherwise makeinfo writes empty files (??)

Default(['test', 'bench', 'examples']) # exclude box

atexit.register(lambda: ra.print_summary(GetBuildFailures, 'ra-ra'))
