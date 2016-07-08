
// (c) Daniel Llorens - 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-dual.C
/// @brief Using Dual<> with ra:: arrays & expressions.

#include <iostream>
#include <algorithm>
#include <cassert>
#include "ra/format.H"
#include "ra/dual.H"
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/ra-operators.H"
#include "ra/ra-io.H"

using std::cout; using std::endl; using std::flush;
using real = double;
using complex = std::complex<double>;

namespace ra {

// Register our type as a scalar with ra:: . This isn't needed to have
// containers of Dual<>, only to use Dual<>s by themselves as expr terms.
template <class T>
struct is_scalar_def<Dual<T> >
{
    static bool const value = true;
};

} // namespace ra

#define DEFINE_CASE(N,  F, DF)                                          \
    struct JOIN(case, N)                                                \
    {                                                                   \
        template <class X> static auto f(X x) { return (F); }           \
        template <class X> static auto df(X x) { return (DF); }         \
    };

DEFINE_CASE(0, x*cos(x)/sqrt(x),
            cos(x)/(2.*sqrt(x))-sqrt(x)*sin(x))
DEFINE_CASE(1, x,
            1)
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
void test1(TestRecorder & tr, std::string const & info, X && x, real const rspec=2e-15)
{
    tr.info(info, ": f vs Dual")
        .test_rel_error(ra::map([](auto && x) { return Case::f(x); }, x),
                        ra::map([](auto && x) { return Case::f(dual(x, 1.)).re; }, x),
                        rspec);
    tr.info(info, ": df vs Dual")
        .test_rel_error(ra::map([](auto && x) { return Case::df(x); }, x),
                        ra::map([](auto && x) { return Case::f(dual(x, 1.)).du; }, x),
                        rspec);
}

template <class Case, class D>
void test2(TestRecorder & tr, std::string const & info, D && d, real const rspec=2e-15)
{
    tr.info(info, ": f vs Dual")
        .test_rel_error(ra::map([](auto && d) { return Case::f(d.re); }, d),
                        ra::map([](auto && d) { return Case::f(d).re; }, d),
                        rspec);
    tr.info(info, ": df vs Dual")
        .test_rel_error(ra::map([](auto && d) { return Case::df(d.re); }, d),
                        ra::map([](auto && d) { return Case::f(d).du; }, d),
                        rspec);
}

int main()
{
    TestRecorder tr(std::cout);

    assert(Dual<real>{3}.du==0.);
    assert(dual(3.).du==0.);

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
    section("args are arrays of real");
    TESTER(test1, ra::Owned<real>({10}, (ra::_0 + 1) * .1));
    section("args are arrays of complex");
    TESTER(test1, ra::Owned<complex>({10}, (ra::_0 + 1) * .1 + complex(0, 1)));
    section("args are arrays of Dual<real>");
    TESTER(test2, ra::Owned<Dual<real> >({10}, map([](auto x) { return dual(x, 1.); }, (ra::_0 + 1) * .1)));
    section("requires is_scalar registration");
    TESTER(test2, Dual<real>(1., 1.));
#undef TESTER
    section("using ra:: operators on arrays of Dual<real>");
    {
        auto test3 = [](TestRecorder & tr, std::string const & info, auto && d, real const rspec=2e-15)
            {
                tr.info(info, ": f vs Dual")
                .test_rel_error(ra::map([](auto && d) { return cos(d.re); }, d),
                                ra::map([](auto && d) { return d.re; }, cos(d)),
                                rspec);
                tr.info(info, ": df vs Dual")
                .test_rel_error(ra::map([](auto && d) { return -sin(d.re); }, d),
                                ra::map([](auto && d) { return d.du; }, cos(d)),
                                rspec);
            };
        test3(tr, "Dual<real>",
              ra::Owned<Dual<real> >({10}, map([](auto x) { return dual(x, 1.); }, (ra::_0 + 1) * .1)));
    }
    section("@TODO define ra:: operators for .re and .du, as real_part(), imag_part() do");
    {

    }
    return tr.summary();
}
