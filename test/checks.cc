// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Runtime checks.

// (c) Daniel Llorens - 2019-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifdef RA_CHECK
  #undef RA_CHECK
#endif
#define RA_CHECK 1 // the point here

#include "ra/base.hh"
#include <exception>


// -------------------------------------
// bit from example/throw.cc which FIXME maybe a prepared option.

struct ra_error: public std::exception
{
    std::string s;
    template <class ... A> ra_error(A && ... a): s(ra::format(std::forward<A>(a) ...)) {}
    virtual char const * what() const throw () { return s.c_str(); }
};

#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) [[unlikely]] throw ra_error("ra::", std::source_location::current(), " (" RA_STRINGIZE(cond) ") " __VA_OPT__(,) __VA_ARGS__); }
// -------------------------------------

#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, std::string, ra::TestRecorder;
using ra::int_c, ra::ilist_t;

int main()
{
    cout << "******* " << RA_CHECK << "******" << endl;
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

    tr.section("choose_len");
    {
        using ra::MIS, ra::UNB, ra::ANY, ra::choose_len;
        tr.test_eq(ANY, choose_len(UNB, ANY));
        tr.test_eq(ANY, choose_len(ANY, UNB));
        tr.test_eq(0,   choose_len(0, UNB));
        tr.test_eq(0,   choose_len(UNB, 0));
        tr.test_eq(0,   choose_len(0, ANY));
        tr.test_eq(0,   choose_len(ANY, 0));
        tr.test_eq(MIS, choose_len(1, 0));
        tr.test_eq(0,   choose_len(0, 0));
    }


// ------------------------------
// see test/iota.cc
// ------------------------------

    tr.section("ot of range with iota");
    {
        std::string msg;
        try {
            cout << ra::iota(10).at(std::array {11}) << endl;
        } catch (ra_error & e) {
            msg = e.what();
        }
        tr.info(msg).test(0<msg.size());
    }


// ------------------------------
// see test/fromb.cc
// ------------------------------

    tr.section("out of range with iota subscripts");
    {
        std::string msg;
        try {
            ra::Small<int, 10> a = ra::_0;
            cout << a(ra::iota(ra::int_c<1>(), 10)) << endl;
        } catch (ra_error & e) {
            msg = e.what();
        }
        tr.info(msg).test(0<msg.size());
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
            tr.test_eq(p[2], ra::ptr((int *)p, ra::int_c<2>{}).at(std::array {2}));
            x = 2;
        } catch (ra_error & e) {
            x = x+10;
        }
        tr.info("ptr.at()").test_eq(11, x);
        x = 0;
        try {
            int p[] = {10, 20, 30};
            tr.test_eq(p[2], ra::ptr((int *)p, 2).at(std::array {2}));
            x = 1;
        } catch (ra_error & e) {
            x = x+10;
        }
        tr.info("ptr.at()").test_eq(10, x);
    }
    tr.section("bad length to ptr()");
    {
        int x = 0;
        try {
            int p[] = {10, 20, 30};
            tr.test_eq(-1, ra::ptr((int *)p, -1));
        } catch (ra_error & e) {
            x = 1;
        }
        tr.info("ptr len").test_eq(x, 1);
    }


// ------------------------------
// FIXME checking at Match isn't enough, and ply() doesn't check nicely
// ------------------------------
/*
    ra::Big<double> z(10, 0.);
    auto w = z(ra::insert<1>, ra::all);
// causes abort() in ply(). Checks are skipped bc there's only one positive rank arg
    w *= -1;
// causes abort() in ply(). Checks are skipped bc there's only one argument
    for_each([](auto & w) { w *= -1; }, -w) ;
// causes abort() in ply(). Checks are skipped bc no expr is built.
    for_each([](auto & w) { w *= -1; }, w) ;
    std::cout << w << std::endl;
*/


// ------------------------------
// see test/frame-new.cc
// ------------------------------

    tr.section("static match in dynamic case");
    {
        ra::Big<int> a({2, 3, 4}, 0);
        tr.test_eq(2, agree_s(a, 99));
    }
    tr.section("dynamic (implicit) match");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 4, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        tr.test_eq(1, agree_s(a, b));
        tr.test_eq(0, agree(a, b));
#define MAP map_([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        int x = 0;
        try {
            cout << "CHECK " << MAP.check() << endl;
            cout << MAP << endl; // check happens here
            x = 1;
        } catch (ra_error & e) {
        }
        tr.test_eq(0, x);
#undef MAP
    }
// If the size of an expression is static, dynamic checks may still need to be run if any of the terms of the expression has dynamic size. This is checked in Match::check_s().
    tr.section("static mismatch I");
    {
        ra::Small<int, 2, 2> a;
        ra::Small<int, 3, 2> b;
        cout << ra::Match(start(a), start(b)).len_s(0) << endl;
        static_assert(0==agree_s(a, b));
        static_assert(0==agree_s(b, a));
    }
    tr.section("static mismatch II");
    {
        ra::Small<int, 2> a;
        ra::Small<int, 2, 2> b;
        ra::Small<int, 2, 3> c;
        static_assert(2==agree_s(a, b));
        static_assert(2==agree_s(a, c));
        static_assert(2==agree_s(b, a));
        static_assert(2==agree_s(c, a));
        static_assert(0==agree_s(a, b+c));
        static_assert(0==agree_s(b+c, a));
        static_assert(0==agree_s(a+(b+c)));
        static_assert(0==agree_s((b+c)+a));
    }

    tr.section("static match");
    {
        ra::Small<int, 2, 2> a;
        ra::Small<int, 2> b;
        static_assert(2==agree_s(a, b));
        static_assert(2==agree_s(b, a));
    }
    tr.section("static match with dynamic term");
    {
        ra::Small<int> a;
        ra::Big<int, 1> b({1}, 77);
        tr.info("with rank ", ra::rank_s<decltype(a+b)>()).test_eq(2, agree_s(a, b));
        tr.info("with rank ", ra::rank_s<decltype(a+b)>()).test_eq(2, agree_s(b, a));
    }
    tr.section("dynamic terms in match, static rank");
    {
        int error = 0;
        std::string s;
        try {
            ra::Small<int, 2> a {2, 3};
            ra::Big<int, 1> b({1}, 77);
            static_assert(2==decltype(a+b)::len_s(0));
            tr.info("with rank ", ra::rank_s<decltype(a+b)>()).test_eq(1, decltype(a+b)::check_s());
            tr.test(!agree(a, b));
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
            cout << shape(a, 0) << endl;
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
    tr.section("iffy conversions");
// won't fail until runtime bc val[0] might have size 1, but FIXME this is very likely a coding error.
// FIXME shape doesn't show in the error message as it should.
    {
        ra::Big<double, 2> val = { { 7, 0, 0, 0.5, 1.5, 1, 1, 1 } };
        ra::Big<double, 2> bal = { { 7 }, { 0 } };
        double x = bal[0];
        tr.test_eq(7, x);
        int error = 0;
        string s;
        try {
            double x = val[0];
            cout << "x " << x << " (!)" << endl;
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("caught error L" RA_STRINGIZE(__LINE__) ": ", s).test_eq(1, error);
    }


// ------------------------------
// see test/frame-old.cc
// ------------------------------

#define MAP ra::map_(plus2double_print, a.iter(), b.iter())
    tr.section("frame matching should-be-error cases. See frame-old.cc");
    {
        ra::Unique<double, 1> a({3}, 10);
        ra::Unique<double, 1> b({4}, 1);
        auto plus2double_print = [](double a, double b) { cout << (a - b) << " "; };
        int error = 0;
        string s;
        try {
            tr.info("dynamic test is needed").test_eq(1, decltype(MAP)::check_s());
            tr.test(!agree(a, b));
            ply_ravel(MAP);
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("caught error L" RA_STRINGIZE(__LINE__) ": ", s).test_eq(1, error);
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
            tr.info("dynamic test is needed").test_eq(1, decltype(MAP)::check_s());
            tr.test(!agree(a, b));
            ply_ravel(MAP);
        } catch (ra_error & e) {
            error = 1;
            s = e.s;
        }
        tr.info("caught error L" RA_STRINGIZE(__LINE__) ": ", s).test_eq(1, error);
    }
#undef MAP

    return tr.summary();
}
