# -*- mode: Python -*-
# -*- coding: utf-8 -*-

# (c) Daniel Llorens - 2016, 2017

# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

# Utilities for SConstructs

import os, string
from colorama import Fore, Back, Style
from os.path import join, abspath
from subprocess import call

def take_from_environ(env, var, wrapper=(lambda x: x), default=None):
    if var in os.environ and os.environ[var]!='':
        env[var] = wrapper(os.environ[var])
    elif default is not None:
        env[var] = default

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

    stamp = env.File(join(variant_dir, str(source[0])) + string.join(args[1:]) + '.check')
    return env.Command(stamp, source, tester(args))

def to_test_ra(env_, variant_dir):
    def f(source, target='', cxxflags=[], cppdefines=[]):
        if len(cxxflags)==0 or len(cppdefines)==0:
            env = env_
        else:
            env = env_.Clone()
            env.Append(CXXFLAGS=cxxflags + ['-U' + k for k in cppdefines.keys()], CPPDEFINES=cppdefines)
        if len(target)==0:
            target = source
        obj = env.Object(target, [source + '.C'])
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
