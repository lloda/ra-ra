// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for early().

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"

using std::cout, std::endl, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("early with rank 0");
    {
        {
            ra::Big<int, 0> a = 99;
            tr.test(ra::any(a==99));
            tr.test(ra::every(a==99));
        }
        {
            ra::Big<int> a = 99;
            tr.test_eq(0, a.rank());
            tr.test(ra::any(a==99));
            tr.test(ra::every(a==99));
        }
    }
    tr.section("early with static lens");
    {
        ra::Small<int, 2, 3> a = ra::_0*2 + ra::_1*10;
        tr.test(!ra::any(odd(a)));
        tr.test(ra::every(!odd(a)));
    }
    tr.section("early with static lens. Regression with ununrolled expr");
    {
        ra::Small<int, 2, 3> a = { { 1, 1, 1 }, { 1, 0, 1} };
        tr.test(!every(1==a));
        tr.test(any(0==a));
        auto b = ra::transpose(a, ra::int_list<1, 0>{});
        tr.test(!every(1==b));
        tr.test(any(0==b));
    }
    tr.section("early with static lens is constexpr");
    {
        constexpr ra::Small<int, 2, 3> a = ra::_0*2 + ra::_1*10;
        constexpr bool not_any = !ra::any(odd(a));
        constexpr bool every_not = ra::every(!odd(a));
        tr.test(not_any);
        tr.test(every_not);
    }
    tr.section("early with static rank");
    {
        ra::Big<int, 2> a({2, 3}, ra::_0*2 + ra::_1*10);
        tr.test(!ra::any(odd(a)));
        tr.test(ra::every(!odd(a)));
    }
    tr.section("early with var rank");
    {
        ra::Big<int> a = ra::iota(9)*2;
        tr.test(!ra::any(odd(a)));
        tr.test(ra::every(!odd(a)));
    }
    return tr.summary();
}
