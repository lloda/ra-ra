// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Runtime checks.

// (c) Daniel Llorens - 2019-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifdef RA_DO_CHECK
  #undef RA_DO_CHECK
  #define RA_DO_CHECK 1 // kind of the point here
#endif

#include <ranges>
#include <iostream>
#include "ra/bootstrap.hh"


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

#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "]", ##__VA_ARGS__); }
// -------------------------------------

#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, std::string, ra::TestRecorder;
using ra::mp::int_c, ra::mp::int_list;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("bad cell rank");
    {
        bool yes = false;
        ra::Big<int> a0 = 0;
        std::string msg;
        try {
            std::cout << ra::iter<1>(a0) << std::endl;
        } catch (ra_error & e) {
            msg = e.what();
            yes = true;
        }
        tr.info(msg).test(yes);
    }

// ------------------------------
// see test/compatibility.cc
// ------------------------------

    tr.section("bad indices to ptr().at()");
    {
        int x = 0;
        try {
            int p[] = {10, 20, 30};
            tr.test_eq(p[2], ra::ptr(p).at(std::array {2}));
            x = 1;
            tr.test_eq(p[2], ra::ptr(p, ra::mp::int_c<2>{}).at(std::array {2}));
            x = 2;
        } catch (ra_error & e) {
            x = x+10;
        }
        tr.info("ptr.at()").test_eq(11, x);
        x = 0;
        try {
            int p[] = {10, 20, 30};
            tr.test_eq(p[2], ra::ptr(p, 2).at(std::array {2}));
            x = 1;
        } catch (ra_error & e) {
            x = x+10;
        }
        tr.info("ptr.at()").test_eq(10, x);
    }

// ------------------------------
// see test/frame-new.cc
// ------------------------------

    tr.section("dynamic (implicit) match");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 4, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        int x = 0;
        try {
            tr.test_eq(ra::DIM_ANY, EXPR.len_s(1));
            x = 1;
        } catch (ra_error & e) {
        }
        tr.test_eq(0, x);
#undef EXPR
    }
// If the size of an expr is static, dynamic checks may still need to be run if any of the terms of the expr has dynamic size. This is checked in match.hh: check_expr_s().
    tr.section("dynamic terms in match, static rank");
    {
        int error = 0;
        std::string s;
        try {
            ra::Small<int, 2> a {2, 3};
            ra::Big<int, 1> b({1}, 77);
            tr.test_eq(1, ra::check_expr_s<decltype(a+b)>());
            a = b;
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("dynamic size checks on static size expr (", s, ")").test_eq(1, error);
    }
    tr.section("dynamic terms in match, dynamic rank. See frame-new.cc");
    {
        auto f2 = [](ra::Big<int, 2> const & a) { cout << ra::start(shape(a)) << endl; };
        int error = 0;
        std::string s;
        try {
            ra::Big<int> a {};
// flag the error when casting rank-0 to rank-2 array. FIXME check that copying is still possible.
            f2(a);
            error = 0;
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("dynamic size checks on static size expr (", s, ")").test_eq(1, error);
    }

// ------------------------------
// see test/big-0.cc
// ------------------------------

    tr.section("check rank errors with dynamic rank (I)");
    {
        int x = 0;
        try {
            ra::Big<int> a = 0;
            cout << a.len(0) << endl;
            x = 1;
        } catch (ra_error & e) {
            x = 2;
        }
        tr.info("caught error").test_eq(2, x);
    }
    tr.section("check rank errors with dynamic rank (II)");
    {
        int x = 0;
// initialization is to rank 1, size 0.
        try {
            ra::Big<int> a;
            a = ra::Big<int, 1> { 1 };
            x = 1;
        } catch (ra_error & e) {
            x = 2;
        }
        tr.info("uninitialized dynamic rank").test_eq(2, x);
    }

// ------------------------------
// see test/frame-old.cc
// ------------------------------

#define EXPR ra::expr(plus2double_print, a.iter(), b.iter())
    tr.section("frame matching should-be-error cases. See frame-old.cc");
    {
        ra::Unique<double, 1> a({3}, 10);
        ra::Unique<double, 1> b({4}, 1);
        auto plus2double_print = [](double a, double b) { cout << (a - b) << " "; };
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
    tr.section("frame matching should-be-error cases - dynamic rank. See frame-old.cc");
    {
        ra::Unique<double> a({3}, 10);
        ra::Unique<double> b({4}, 1);
        auto plus2double_print = [](double a, double b) { cout << (a - b) << " "; };
        int error = 0;
        string s;
        try {
            std::cout << "A: " << a.iter().len(0) << endl;
            std::cout << "B: " << b.iter().len(0) << endl;
            tr.info("dynamic test is needed").test_eq(1, ra::check_expr_s<decltype(EXPR)>());
            ply_ravel(EXPR);
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("caught error L" STRINGIZE(__LINE__) ": ", s).test_eq(1, error);
    }
#undef EXPR
    return tr.summary();
}
