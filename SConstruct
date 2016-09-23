# -*- mode: Python -*-
# -*- coding: utf-8 -*-

# (c) Daniel Llorens - 2016

# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

# Top SConstruct for ra-ra
# @todo Shared pieces with examples/SConstruct and test/SConstruct

import atexit
from colorama import Fore, Back, Style

top = {'skip_summary': True}; Export('top');
SConscript('test/SConstruct', 'top')
SConscript('examples/SConstruct', 'top')

env = Environment(BUILDERS = {'INFOBuilder' : Builder(action = 'makeinfo < $SOURCES > $TARGET',
                                                     suffix = '.info',
                                                     src_suffix = '.texi')})
env.INFOBuilder(target = 'ra-ra.info', source = 'ra-ra.texi')

def print_summary():
    from SCons.Script import GetBuildFailures
    test_item_tally = 0
    test_tally = 0
    build_tally = 0

    print '\n' + Style.BRIGHT + 'Summary for ek' + Style.RESET_ALL + '\n--------'
    for bf in GetBuildFailures():
        if str(bf.node).endswith('.check') and (bf.status > 0):
            print (Style.BRIGHT + Fore.RED + '%s ' + Style.RESET_ALL + Fore.RESET + ' failed (%d)') \
                % (bf.node, bf.status)
            test_item_tally += bf.status
            test_tally += 1
        else:
            print (Style.BRIGHT + Fore.YELLOW + '%s ' + Style.RESET_ALL + Fore.RESET + ' failed (%s)') \
                % (bf.node, bf.errstr)
            build_tally += 1

    print '%d targets failed to build.' % build_tally
    print '%d tests failed with %d total failures.' % (test_tally, test_item_tally)

atexit.register(print_summary)
