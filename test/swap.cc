// -*- mode: c++; coding: utf-8 -*-
// ra/test - Test for swap(containers).

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr;
    auto test =
        [&](auto & a0, auto & a1, auto & b0, auto & b1)
        {
            auto ap = a1.data();
            auto bp = b1.data();
            tr.test_eq(a0, a1);
            tr.test_eq(b0, b1);
            swap(a1, b1);
            tr.test_eq(a0, b1);
            tr.test_eq(b0, a1);
            tr.test_eq(ra::scalar(ap), ra::scalar(b1.data()));
            tr.test_eq(ra::scalar(bp), ra::scalar(a1.data()));
            swap(b1, a1);
            tr.test_eq(a0, a1);
            tr.test_eq(b0, b1);
            tr.test_eq(ra::scalar(ap), ra::scalar(a1.data()));
            tr.test_eq(ra::scalar(bp), ra::scalar(b1.data()));
        };
    tr.section("swap ct and rt");
    {
        ra::Small<int, 2, 3> a0 = 1 + ra::_0 - ra::_1;
        ra::Big<int, 2> a1 ({2, 3}, 1 + ra::_0 - ra::_1);

        ra::Small<int, 3, 2> b0 = 1 - ra::_0 + ra::_1;
        ra::Big<int> b1 ({3, 2}, 1 - ra::_0 + ra::_1);

        test(a0, a1, b0, b1);
    }
    tr.section("swap ct and ct");
    {
        ra::Small<int, 2, 3> a0 = 1 + ra::_0 - ra::_1;
        ra::Big<int, 2> a1 ({2, 3}, 1 + ra::_0 - ra::_1);

        ra::Small<int, 3, 2> b0 = 1 - ra::_0 + ra::_1;
        ra::Big<int, 2> b1 ({3, 2}, 1 - ra::_0 + ra::_1);

        test(a0, a1, b0, b1);
    }
    tr.section("swap rt and rt");
    {
        ra::Small<int, 2, 3> a0 = 1 + ra::_0 - ra::_1;
        ra::Big<int> a1 ({2, 3}, 1 + ra::_0 - ra::_1);
        ra::Small<int, 2, 3, 4> b0 = 1 - ra::_0 + ra::_1 + ra::_2;
        ra::Big<int> b1 ({2, 3, 4}, 1 - ra::_0 + ra::_1 + ra::_2);

        test(a0, a1, b0, b1);
    }

    return tr.summary();
}
