// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Reduce a bug in gcc 8.3.

// (c) Daniel Llorens - 2013-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    {
        double b[6] = { 1, 2, 3, 4, 5, 6 };
        ra::ViewBig<double *> ra { {6}, b };
        tr.test_eq(b, ra);

        ra::Unique<double> A({2, 3}, 0);
        tr.test_eq(ra::Small<ra::dim_t, 2> {2, 3}, shape(iter<-2>(A)));
        tr.test_eq(ra::Small<ra::dim_t, 1> {2}, shape(iter<-1>(A)));

        double pool[6] = { 1, 2, 3, 4, 5, 6 };
        ra::Unique<double> u({3, 2}, pool);
        tr.test(std::equal(pool, pool+6, u.begin()));

        ra::Unique<double> q(ra::scalar(44));
        tr.test_eq(1, q.size());

        double rpool[6] = { 1, 2, 3, 4, 5, 6 };
        ra::ViewBig<double *, 2> r { {{3, 1}, {2, 3}}, rpool };
        double rcheck[6] = { 1, 4, 2, 5, 3, 6 };
// kinda... long
        auto v = at_view(r, ra::Big<int>({0}, {}));
        tr.test(std::ranges::equal(std::ranges::subrange(rcheck, rcheck+6),
                                   std::ranges::subrange(v)));
    }
    return tr.summary();
}
