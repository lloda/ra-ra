// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Using Dual<> with ra:: arrays & expressions.

// (c) Daniel Llorens - 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <cassert>
#include <iostream>
#include <algorithm>
#include "ra/test.hh"
#include "ra/dual.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;
using complex = std::complex<double>;
using ra::dual, ra::Dual, ra::sqr;

// not needed to put Dual<> in containers, but needed to use Dual<>s by themselves as expr terms.
template <class T> constexpr bool ra::is_scalar_def<Dual<T>> = true;

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
#undef DEFINE_CASE

template <class Case, class X>
void
test1(TestRecorder & tr, std::string const & info, X && x, real const rspec=2e-15)
{
    tr.info(info, ": f vs Dual")
        .test_rel(ra::map([](auto && x) { return Case::f(x); }, x),
                  ra::map([](auto && x) { return Case::f(dual(x, 1.)).re; }, x),
                  rspec);
    tr.info(info, ": df vs Dual")
        .test_rel(ra::map([](auto && x) { return Case::df(x); }, x),
                  ra::map([](auto && x) { return Case::f(dual(x, 1.)).du; }, x),
                  rspec);
}

template <class Case, class D>
void
test2(TestRecorder & tr, std::string const & info, D && d, real const rspec=2e-15)
{
    tr.info(info, ": f vs Dual")
        .test_rel(ra::map([](auto && d) { return Case::f(d.re); }, d),
                  ra::map([](auto && d) { return Case::f(d).re; }, d),
                  rspec);
    tr.info(info, ": df vs Dual")
        .test_rel(ra::map([](auto && d) { return Case::df(d.re); }, d),
                  ra::map([](auto && d) { return Case::f(d).du; }, d),
                  rspec);
}

int main()
{
    TestRecorder tr(std::cout);

    tr.test_eq(0., Dual<real>{3}.du);
    tr.test_eq(0., dual(3.).du);

#define TESTER(testn, x)                        \
    {                                           \
        testn<case0>(tr, "case0", x);           \
        testn<case1>(tr, "case1", x);           \
        testn<case2>(tr, "case2", x);           \
        testn<case3>(tr, "case3", x, 5e-14);    \
        testn<case4>(tr, "case4", x);           \
        testn<case5>(tr, "case5", x);           \
        testn<case6>(tr, "case6", x);           \
    }
    tr.section("args are arrays of real");
    TESTER(test1, ra::Big<real>({10}, (ra::_0 + 1) * .1));
    tr.section("args are arrays of complex");
    TESTER(test1, ra::Big<complex>({10}, (ra::_0 + 1) * .1 + complex(0, 1)));
    tr.section("args are arrays of Dual<real>");
    TESTER(test2, ra::Big<Dual<real>>({10}, map([](auto x) { return dual(x, 1.); }, (ra::_0 + 1) * .1)));
    tr.section("requires is_scalar registration");
    TESTER(test2, Dual<real>(1., 1.));
#undef TESTER
    tr.section("using ra:: operators on arrays of Dual<real>");
    {
        auto test3 = [](TestRecorder & tr, std::string const & info, auto && d, real const rspec=2e-15)
            {
                tr.info(info, ": f vs Dual")
                .test_rel(ra::map([](auto && d) { return cos(d.re); }, d),
                                ra::map([](auto && d) { return d.re; }, cos(d)),
                                rspec);
                tr.info(info, ": df vs Dual")
                .test_rel(ra::map([](auto && d) { return -sin(d.re); }, d),
                                ra::map([](auto && d) { return d.du; }, cos(d)),
                                rspec);
            };
        test3(tr, "Dual<real>",
              ra::Big<Dual<real>>({10}, map([](auto x) { return dual(x, 1.); }, (ra::_0 + 1) * .1)));
    }
    tr.section("TODO define ra:: operators for .re and .du, as real_part(), imag_part() do");
    {

    }
    return tr.summary();
}
