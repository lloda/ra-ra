# -*- coding: utf-8; mode: Python -*-
# (c) Daniel Llorens - 2016, 2017-2019

# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

# Utilities for SConstructs

import os, string

# These are colorama names, but the dependence is a bother.
class Fore: RED = '\x1b[31m'; YELLOW ='\x1b[33m'; RESET = '\x1b[39m'
class Style: BRIGHT = '\x1b[1m'; RESET_ALL = '\x1b[0m';

from os.path import join, abspath, split
from subprocess import call

# SANITIZE = []
SANITIZE = ['-fsanitize=address']

CXXFLAGS = ['-std=c++20', '-Wall', '-Werror', '-Wlogical-op',
            '-fdiagnostics-color=always', '-Wno-unknown-pragmas',
            '-Wno-error=strict-overflow', '-Werror=zero-as-null-pointer-constant',
            # '-finput-charset=UTF-8', '-fextended-identifiers',
            #'-Wconversion',
            # '-funsafe-math-optimizations', # TODO Test with this.
        ] + SANITIZE

LINKFLAGS = SANITIZE

def blas_flags(Configure, env, arch):
    env_blas = env.Clone()
    if arch.find('apple-darwin') >= 0:

        # after OS X 10.14 giving -framework Accelerate isn't enough.
        # cf https://github.com/shogun-toolbox/shogun/commit/6db834fb4ca9783b6e5adfde808d60ebfca0abc9
        # cf https://github.com/BVLC/caffe/blob/master/cmake/Modules/FindvecLib.cmake

        cblas_possible_paths = ['/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/Headers/']

        for ppath in cblas_possible_paths:
            env0 = env.Clone()
            env0.Append(CPPPATH = [ppath])
            conf = Configure(env0)
            success = conf.CheckCHeader('cblas.h')
            conf.Finish()
            if success:
                print("cblas.h found at %r" % ppath)
                env_blas.Append(LINKFLAGS=' -framework Accelerate ')
                env_blas.Append(CPPPATH=[ppath])
                break
        else:
            print("cblas.h couldn't be found. Crossing fingers.")
            env_blas.Append(LINKFLAGS=' -framework Accelerate ')
            env_blas.Append(CCFLAGS=' -framework Accelerate ')
    else:
        env_blas.Append(LIBS=['blas'])

    return env_blas

def ensure_ext(s, ext):
    "if s doesn't end with ext, append it."
    p = s.rfind(ext)
    return s if p+len(ext) == len(s) else s + ext

def remove_ext(s):
    "clip string from the last dot until the end."
    p = s.rfind('.')
    assert p>=0, 'source must have an extension'
    return s[0:p]

def path_parts(path):
    path, tail = split(path)
    return ([path] if path == os.sep else path_parts(path) if path else []) \
           + ([tail] if tail else [])

def take_from_environ(env, var, wrapper=(lambda x: x), default=None):
    if var in os.environ and os.environ[var]!='':
        env[var] = wrapper(os.environ[var])
    elif default is not None:
        env[var] = default

def get_value_wo_error(dictionary, key, default = ''):
    if key in dictionary:
        return dictionary[key]
    else:
        return default

def dict_index_list(dictionary, list_of_keys):
    return dict([ (k, get_value_wo_error(dictionary, k))
                  for k in list_of_keys ])

def to_test(env, variant_dir, source, args):
    """
    Run program with args to produce a check stamp. The name of the first source
    is used to generate the name of the stamp.
    """

    class tester:
        def __init__(self, args):
            self.args = args

        def __call__(self, target, source, env):
            print("-> running %s \n___________________" % str(self.args))
            r = os.spawnl(os.P_WAIT, self.args[0], self.args[0], *self.args[1:])
            print("^^^^^^^^^^^^^^^^^^^")
            print('r ', r)
            if not r:
                print("PASSED %s" % str(self.args))
                call(['touch', target[0].abspath])
            else:
                print("FAILED %s" % str(self.args))
            return r

    stamp = env.File(join(variant_dir, str(source[0])) + "".join(args[1:]) + '.check')
    return env.Command(stamp, source, tester(args))

# def to_source(env, targets, source):
#     main = source[0]
#     for target in targets: env.Notangle(target, remove_ext(main)+'.nw')
#     env.Noweave(remove_ext(main)+'.tex', remove_ext(main)+'.nw')
#     env.PDF(remove_ext(main), remove_ext(main)+'.tex')

def to_source_from_noweb(env, targets, source):
    main = source[0]
    env.Noweave(remove_ext(main) + '.tex', remove_ext(main) + '.nw')
    env.PDF(remove_ext(main), remove_ext(main) + '.tex')
    return [env.Notangle(target, remove_ext(main) + '.nw') for target in targets]

def to_test_ra(env_, variant_dir):
    def f(source, target='', cxxflags=[], cppdefines=[]):
        if len(cxxflags)==0 or len(cppdefines)==0:
            env = env_
        else:
            env = env_.Clone()
            env.Append(CXXFLAGS=cxxflags + ['-U' + k for k in cppdefines.keys()], CPPDEFINES=cppdefines)
        if len(target)==0:
            target = source
        obj = env.Object(target, [source + '.cc'])
        test = env.Program(target, obj)
        to_test(env, variant_dir, test, [test[0].abspath])
    return f

def print_summary(GetBuildFailures, tag):
    test_item_tally = 0
    test_tally = 0
    build_tally = 0

    print('\n' + Style.BRIGHT + 'Summary for ' + tag + Style.RESET_ALL + '\n--------')
    for bf in GetBuildFailures():
        if str(bf.node).endswith('.check') and (bf.status > 0):
            print((Style.BRIGHT + Fore.RED + '%s ' + Style.RESET_ALL + Fore.RESET + ' failed (%d)') \
                  % (bf.node, bf.status))
            test_item_tally += bf.status
            test_tally += 1
        else:
            print((Style.BRIGHT + Fore.YELLOW + '%s ' + Style.RESET_ALL + Fore.RESET + ' failed (%s)') \
                  % (bf.node, bf.errstr))
            build_tally += 1

    print('%d targets failed to build.' % build_tally)
    print('%d tests failed with %d total failures.' % (test_tally, test_item_tally))
