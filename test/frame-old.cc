// -*- mode: c++; coding: utf-8 -*-
/// @file frame-old.cc
/// @brief Frame-matching tests for pre v10 Expr, previously in test/ra-0.cc.

// (c) Daniel Llorens - 2013-2014, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/format.hh"


// -------------------------------------
// bit from example/throw.cc which FIXME should be easier. Maybe an option in ra/macros.hh.

struct ra_error: public std::exception
{
    std::string s;
    template <class ... A> ra_error(A && ... a): s(ra::format(std::forward<A>(a) ...)) {}
    virtual char const * what() const throw ()
    {
        return s.c_str();
    }
};

#ifdef RA_ASSERT
#error RA_ASSERT is already defined!
#endif
#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "]", ##__VA_ARGS__); }
// -------------------------------------

#include "ra/complex.hh"
#include "ra/test.hh"
#include "ra/big.hh"

using std::cout, std::endl, std::flush, std::string, ra::TestRecorder;
using real = double;

int main()
{
    TestRecorder tr;

    tr.section("frame matching - TensorIndex/Scalar");
    {
// driver is highest rank, which is ra::_0 (1).
        constexpr auto e = ra::_0+1;
        static_assert(e.rank_s()==1, "bad rank_s");
        static_assert(e.rank()==1, "bad rank");
        static_assert(e.size_s(0)==ra::DIM_BAD, "bad size");
    }
    tr.section("frame matching - Unique/TensorIndex");
    {
        ra::Unique<real, 2> c({3, 2}, ra::none);
        ra::Unique<real, 2> a({3, 2}, ra::none);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        std::iota(a.begin(), a.end(), 1);
        std::fill(c.begin(), c.end(), 0);
        ply(expr([](real & c, real a, int b) { c = a-(b+1); },
                 c.iter(), a.iter(), ra::start(ra::_0)));
        tr.test_eq(check, c);

        std::fill(c.begin(), c.end(), 0);
        ply(expr([](real & c, int a, real b) { c = b-(a+1); },
                 c.iter(), ra::start(ra::_0), a.iter()));
        tr.test_eq(check, c);
    }
    tr.section("frame matching - Unique/TensorIndex - TensorIndex can't be driving arg");
    {
        ra::Unique<real, 2> c({3, 2}, ra::none);
        ra::Unique<real, 2> a({3, 2}, ra::none);
        real check[3][2] = { {0, 0}, {2, 2}, {4, 4} };

        std::iota(a.begin(), a.end(), 1);
        std::fill(c.begin(), c.end(), 0);
        ply(expr([](real a, int b, real & c) { c = a-(b+1); },
                 a.iter(), ra::start(ra::_1), c.iter()));
        tr.test_eq(check, c);

        std::fill(c.begin(), c.end(), 0);
        ply(expr([](int a, real b, real & c) { c = b-(a+1); },
                 ra::start(ra::_1), a.iter(), c.iter()));
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

    tr.section("frame matching - Unique/Unique");
    {
        ra::Unique<real, 2> c({3, 2}, ra::none);
        ra::Unique<real, 2> a({3, 2}, ra::none);
        ra::Unique<real, 1> b({3}, ra::none);
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        TEST(ply_ravel);
    }
    tr.section("frame matching - Unique/Small");
    {
        ra::Unique<real, 2> c({3, 2}, ra::none);
        ra::Unique<real, 2> a({3, 2}, ra::none);
        ra::Small<real, 3> b;
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        TEST(ply_ravel);
    }
    tr.section("frame matching - Small/Small");
    {
        ra::Small<real, 3, 2> c;
        ra::Small<real, 3, 2> a;
        ra::Small<real, 3> b;
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        real check[3][2] = { {0, 1}, {1, 2}, {2, 3} };

        TEST(ply_ravel);
    }
#undef TEST
    tr.section("frame match is good only for full expr, so test on ply, not construction");
    {
        ra::Unique<real, 2> a({2, 2}, 0.);
        ra::Unique<real, 1> b {1., 2.};
// note that b-c has no driver, but all that matters is that the full expression does.
        auto e = expr([](real & a, real bc) { a = bc; },
                      a.iter(), expr([](real b, real c) { return b-c; },  b.iter(), ra::start(ra::_1)));
        ply(e);
        tr.test_eq(1, a(0, 0));
        tr.test_eq(0, a(0, 1));
        tr.test_eq(2, a(1, 0));
        tr.test_eq(1, a(1, 1));
    }
#define EXPR ra::expr(plus2real_print, a.iter(), b.iter())
    tr.section("frame matching should-be-error cases");
    {
        ra::Unique<real, 1> a({3}, 10);
        ra::Unique<real, 1> b({4}, 1);
        auto plus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        int error = 0;
        string s;
        try {
            tr.info("dynamic test is needed").test_eq(1, ra::check_expr_s<decltype(EXPR)>());
            ply_ravel(EXPR);
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("caught error L" STRINGIZE(__LINE__) ": ", s).test_eq(1, error);
    }
    tr.section("frame matching should-be-error cases - dynamic rank");
    {
        ra::Unique<real> a({3}, 10);
        ra::Unique<real> b({4}, 1);
        auto plus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        int error = 0;
        string s;
        try {
            std::cout << "A: " << a.iter().size(0) << endl;
            std::cout << "B: " << b.iter().size(0) << endl;
            tr.info("dynamic test is needed").test_eq(1, ra::check_expr_s<decltype(EXPR)>());
            ply_ravel(EXPR);
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("caught error L" STRINGIZE(__LINE__) ": ", s).test_eq(1, error);
    }
#undef EXPR
    tr.section("unintiuitive behavior [ra33]");
    {
        ra::Big<int, 1> i = {0, 1, 2};
        ra::Big<double, 2> A({3, 2}, ra::_0 - ra::_1);
        ra::Big<double, 2> F({3, 2}, 0.);
        iter<-1>(F) = A(i); // A(i) returns a nested expression. FIXME Should it?
        tr.test_eq(A, F);
    }
    return tr.summary();
}
