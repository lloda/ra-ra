// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Dual numbers.

// (c) Daniel Llorens - 2013, 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <cassert>
#include <numeric>
#include <iostream>
#include <algorithm>
#include "ra/test.hh"
#include "ra/dual.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;
using complex = std::complex<double>;
using ra::dual, ra::Dual;
using ra::sqr, ra::fma;

#define DEFINE_CASE(N,  F, DF)                                          \
    struct JOIN(case, N)                                                \
    {                                                                   \
        template <class X> static auto f(X x) { return (F); }           \
        template <class X> static auto df(X x) { return (DF); }         \
    };

DEFINE_CASE(0, x*cos(x)/sqrt(x),
            cos(x)/(2.*sqrt(x))-sqrt(x)*sin(x))
DEFINE_CASE(1, x,
            1.)
DEFINE_CASE(2, 3.*exp(x*x)/x+8.*exp(2.*x)/x,
            -3.*exp(x*x)/(x*x)+6.*exp(x*x)+16.*exp(2.*x)/x-8.*exp(2.*x)/(x*x))
DEFINE_CASE(3, cos(pow(exp(x), 4.5)),
            -4.5*exp(4.5*x)*sin(exp(4.5*x)))
DEFINE_CASE(4, 1./(x*x),
            -2.*x/sqr(x*x))
DEFINE_CASE(5, 1./(2.-x*x),
            +2.*x/sqr(2.-x*x))
DEFINE_CASE(6, sinh(x)/cosh(x),
            1./sqr(cosh(x)))
DEFINE_CASE(7, fma(x, x, 3.*x),
            2.*x+3.)
#undef DEFINE_CASE

// repeat case2 using assignment ops.
struct case8
{
    template <class X> static auto f(X x)
    {
        auto y = 3.*exp(x*x);
        y /= x;
        y += 8.*exp(2.*x)/x;
        return y;
    }
    template <class X> static auto df(X x)
    {
        auto lo = x;
        lo *= lo;
        auto dy = -3.*exp(x*x)/lo;
        dy += +6.*exp(x*x);
        dy += +16.*exp(2.*x)/x;
        dy -= 8.*exp(2.*x)/lo;
        return dy;
    }
};

template <class Case, class X>
void test1(TestRecorder & tr, std::string info, X && x, real const rspec=2e-15)
{
    for (unsigned int i=0; i!=x.size(); ++i) {
        tr.info(info, " ", i, " f vs Dual").test_rel(Case::f(x[i]), Case::f(dual(x[i], 1.)).re, rspec);
        tr.info(info, " ", i, " df vs Dual").test_rel(Case::df(x[i]), Case::f(dual(x[i], 1.)).du, rspec);
    }
}

int main()
{
    TestRecorder tr(std::cout);

    tr.test_eq(0., Dual<real>{3}.du);
    tr.test_eq(0., dual(3.).du);

    tr.section("tests with real");
    {
        std::vector<real> x(10);
        std::iota(x.begin(), x.end(), 1);
        for (real & xi: x) { xi *= .1; }

        test1<case0>(tr, "case0", x);
        test1<case1>(tr, "case1", x);
        test1<case2>(tr, "case2", x);
        test1<case3>(tr, "case3", x, 3e-14);
        test1<case4>(tr, "case4", x, 1e-15);
        test1<case5>(tr, "case5", x, 1e-15);
        test1<case6>(tr, "case6", x, 1e-15);
        test1<case7>(tr, "case7", x);
        test1<case8>(tr, "case8", x);
    }
    tr.section("demo with complex");
    {
        Dual<complex> x { complex(3, 1), 1. };
        cout << x << endl;
        cout << exp(x) << endl;
        cout << (x*x) << endl;
    }
    tr.section("real -> dual<complex> conversion");
    {
        Dual<complex> x { 1., 1. };
        x = 0.;
        tr.test_eq(0., x.re);
        tr.test_eq(0., x.du);
    }
    tr.section("tests with complex");
    {
        std::vector<complex> x(10);
        std::iota(x.begin(), x.end(), 1);
        for (complex & xi: x) { xi = xi*.1 + complex(0, 1); }

        test1<case0>(tr, "case0", x);
        test1<case1>(tr, "case1", x);
        test1<case2>(tr, "case2", x);
        test1<case3>(tr, "case3", x, 5e-14);
        test1<case4>(tr, "case4", x, 1e-15);
        test1<case5>(tr, "case5", x, 1e-15);
        test1<case6>(tr, "case6", x, 1.2e-15);
        test1<case7>(tr, "case7", x);
        test1<case8>(tr, "case8", x);
    }
    return tr.summary();
}
