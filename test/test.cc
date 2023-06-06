// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for TestRecorder.

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
constexpr auto QNAN = std::numeric_limits<double>::quiet_NaN();
constexpr auto PINF = std::numeric_limits<double>::infinity();

int main()
{

    TestRecorder tr(std::cout);
    tr.section("amax_strict"); // cf amax() in reduction.cc.
    {
        tr.test(isnan(TestRecorder::amax_strict(ra::Small<double, 2>(QNAN))));
        tr.test(isnan(TestRecorder::amax_strict(ra::Small<double, 2>(1, QNAN))));
        tr.test(isnan(TestRecorder::amax_strict(ra::Small<double, 2>(QNAN, 1))));
        tr.test_eq(9, TestRecorder::amax_strict(ra::Small<double, 2>{1, 9}));
        tr.test_eq(9, TestRecorder::amax_strict(ra::Small<double, 2>{9, 1}));
    }
    tr.section("nan in numeric tests");
    {
        std::ostream devnull(nullptr);
        TestRecorder ut(devnull);
        tr.test(0==ut.summary());
        ut.test_rel_error(0, QNAN, 1e-15);
        tr.test(1==ut.summary());
        ut.test_abs_error(0, QNAN, 1e-15);
        tr.test(2==ut.summary());
    }
    tr.section("inf in numeric tests");
    {
        auto test = [&tr](bool fail, auto ref, auto a, auto err)
        {
            tr.expectfail(fail).test_abs_error(ref, a, err);
            tr.expectfail(fail).test_rel_error(ref, a, err);
        };
        test(false, QNAN, QNAN, 0.);
        test(false, PINF, PINF, 0.);
        test(false, -PINF, -PINF, 0.);
        test(true, QNAN, -PINF, 0.);
        test(true, QNAN, PINF, 0.);
        test(true, -PINF, QNAN, 0.);
        test(true, PINF, QNAN, 0.);
        test(true, PINF, -PINF, 0.);
        test(true, -PINF, PINF, 0.);
        test(true, 3., PINF, 0.);
        test(true, 3., -PINF, 0.);
        test(true, 3., QNAN, 0.);
        test(true, PINF, 3., 0.);
        test(true, -PINF, 3., 0.);
        test(true, QNAN, 3., 0.);
    }
    return tr.summary();
}
