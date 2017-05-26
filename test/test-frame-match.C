
// (c) Daniel Llorens - 2013-2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-frame-matching.C
/// @brief Specific frame-matching tests, previously in test-ra-0.C.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/large.H"

using std::cout; using std::endl; using std::flush;
using real = double;

int main()
{
    TestRecorder tr;

    section("frame matching - TensorIndex/Scalar");
    {
// driver is highest rank, which is ra::_0 (1).
        auto e = ra::_0+1;
        static_assert(e.rank_s()==1, "bad driver"); // TODO CWG1684 should allow .rank() (gcc bug #66297)
// but TensorIndex cannot be a driver.
        static_assert(!(decltype(e)::VALID_DRIVER), "bad driver check");
    }
    section("frame matching - Unique/TensorIndex");
    {
        ra::Unique<real, 2> c({3, 2}, ra::unspecified);
        ra::Unique<real, 2> a({3, 2}, ra::unspecified);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        std::iota(a.begin(), a.end(), 1);
        std::fill(c.begin(), c.end(), 0);
        ply_index(expr([](real & c, real a, int b) { c = a-(b+1); },
                          c.iter(), a.iter(), ra::_0));
        tr.test_eq(check, c);

        std::fill(c.begin(), c.end(), 0);
        ply_index(expr([](real & c, int a, real b) { c = b-(a+1); },
                          c.iter(), ra::_0, a.iter()));
        tr.test_eq(check, c);
    }
    section("frame matching - Unique/TensorIndex - TensorIndex can't be driving arg");
    {
        ra::Unique<real, 2> c({3, 2}, ra::unspecified);
        ra::Unique<real, 2> a({3, 2}, ra::unspecified);
        real check[3][2] = { {0, 0}, {2, 2}, {4, 4} };

        std::iota(a.begin(), a.end(), 1);
        std::fill(c.begin(), c.end(), 0);
        ply_index(expr([](real a, int b, real & c) { c = a-(b+1); },
                          a.iter(), ra::_1, c.iter()));
        tr.test_eq(check, c);

        std::fill(c.begin(), c.end(), 0);
        ply_index(expr([](int a, real b, real & c) { c = b-(a+1); },
                          ra::_1, a.iter(), c.iter()));
        tr.test_eq(check, c);
    }
#define TEST(plier)                                         \
    std::fill(c.begin(), c.end(), 0);                       \
    plier(expr([](real & c, real a, real b) { c = a-b; },   \
               c.iter(), a.iter(), b.iter()));              \
    tr.test_eq(check, c);                                   \
                                                            \
    std::fill(c.begin(), c.end(), 0);                       \
    plier(expr([](real & c, real a, real b) { c = b-a; },   \
               c.iter(), b.iter(), a.iter()));              \
    tr.test_eq(check, c);

    section("frame matching - Unique/Unique");
    {
        ra::Unique<real, 2> c({3, 2}, ra::unspecified);
        ra::Unique<real, 2> a({3, 2}, ra::unspecified);
        ra::Unique<real, 1> b({3}, ra::unspecified);
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        TEST(ply_ravel);
        TEST(ply_index);
    }
    section("frame matching - Unique/Small");
    {
        ra::Unique<real, 2> c({3, 2}, ra::unspecified);
        ra::Unique<real, 2> a({3, 2}, ra::unspecified);
        ra::Small<real, 3> b;
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        TEST(ply_ravel);
        TEST(ply_index);
    }
    section("frame matching - Small/Small");
    {
        ra::Small<real, 3, 2> c;
        ra::Small<real, 3, 2> a;
        ra::Small<real, 3> b;
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        TEST(ply_ravel);
        TEST(ply_index);
    }
#undef TEST
    section("frame match is good only for full expr, so test on ply, not construction");
    {
        ra::Unique<real, 2> a({2, 2}, 0.);
        ra::Unique<real, 1> b {1., 2.};
// note that b-c has no driver, but all that matters is that the full expression does.
        auto e = expr([](real & a, real bc) { a = bc; },
                      a.iter(), expr([](real b, real c) { return b-c; },  b.iter(), ra::_1));
        static_assert(e.A==0, "bad driver selection");
        ply_index(e);
        tr.test_eq(1, a(0, 0));
        tr.test_eq(0, a(0, 1));
        tr.test_eq(2, a(1, 0));
        tr.test_eq(1, a(1, 1));
    }
    section("frame matching should-be-error cases [untested]");
// TODO Check that this is an error.
    // {
    //     ra::Unique<real, 1> a {3};
    //     ra::Unique<real, 1> b {4};
    //     std::iota(a.begin(), a.end(), 10);
    //     std::iota(b.begin(), b.end(), 1);
    //     cout << "a: " << a << endl;
    //     cout << "b: " << b << endl;
    //     auto plus2real_print = [](real a, real b) { cout << (a - b) << " "; };
    //     ply_ravel(ra::expr(plus2real_print, a.iter(), b.iter()));
    // }
// TODO Check that this is an error.
// TODO This also requires that ra::expr handles dynamic rank.
    // {
    //     ra::Unique<real> a {3};
    //     ra::Unique<real> b {4};
    //     std::iota(a.begin(), a.end(), 10);
    //     std::iota(b.begin(), b.end(), 1);
    //     cout << "a: " << a << endl;
    //     cout << "b: " << b << endl;
    //     auto plus2real_print = [](real a, real b) { cout << (a - b) << " "; };
    //     ply_ravel(ra::expr(plus2real_print, a.iter(), b.iter()));
    // }

    return tr.summary();
}
