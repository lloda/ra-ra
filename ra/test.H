
// (c) Daniel Llorens - 2012, 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_TEST_H
#define RA_TEST_H

/// @file test.H
/// @brief Minimal test library.

#include <string>
#include <iostream>
#include <iomanip>
#include <limits>
#include "ra/format.H"
#include "ra/real.H"
// @TODO For where() unfortunately I can't declare the scalar version in an earlier header. Also ra::map()
#include "ra/ra-operators.H"

char const * esc_bold = "\x1b[01m";
char const * esc_unbold = "\x1b[0m";
char const * esc_red = "\x1b[31m";
char const * esc_green = "\x1b[32m";
char const * esc_cyan = "\x1b[36m";
char const * esc_yellow = "\x1b[33m";
char const * esc_blue = "\x1b[34m";
char const * esc_plain = "\x1b[39m";

template <class ... A> void section(A const & ... a)
{
    std::cout << "\n\n" << esc_bold << format(a ...) << esc_unbold << std::endl;
}

struct TestRecorder
{
    std::ostream & o;
    int total = 0;
    std::vector<int> failed;
    std::string info_string;
    bool isquiet_default, isquiet;
    bool willskip;

    TestRecorder(std::ostream & o_=std::cout, bool isquiet_default_=false)
        : o(o_), isquiet_default(isquiet_default_), isquiet(isquiet_default_), willskip(false) {}

    static std::string format_error(double e)
    {
        return format(esc_yellow, std::setprecision(2), e, esc_plain);
    }

    template <class ... A> TestRecorder & info(A && ... a)
    {
        info_string += (info_string=="" ? "" : "; ") + format(a ...);
        return *this;
    }
    TestRecorder & quiet(bool q=true)
    {
        isquiet = q;
        return *this;
    }
    TestRecorder & skip(bool s=true)
    {
        willskip = s;
        return *this;
    }

    void test(bool c)
    {
        if (isquiet) {
            if (!c) {
                o << esc_cyan << "[" << total << "] " << esc_plain << esc_bold << esc_red << "FAILED" << esc_plain << esc_unbold
                  << "... " << info_string << std::endl;
            }
        } else {
            o << esc_cyan << "[" << total << "] " << esc_plain
              << (c ? std::string(esc_green)+"ok"+esc_plain
                  : std::string(esc_bold)+esc_red+"FAILED"+esc_plain+esc_unbold)
              << "... " << info_string << std::endl;
        }
        info_string = "";
        isquiet = isquiet_default;
        if (!willskip) {
            ++total;
            if (!c) {
                failed.push_back(total);
            }
        }
        willskip = false;
    }
    void operator+=(bool c)
    {
        test(c);
    }
// Comp = ... is non-deduced context, so can't replace test_eq() with a default argument here.
    template <class W, class A, class Comp>
    bool test_comp(W && wanted, A && got, Comp && comp)
    {
        bool c = every(ra::map(comp, wanted, got));
// where() to match shapes if either doesn't have one
        info("comp(wanted: ", where(true, wanted, got), ", got: ", got, ")").test(c);
        return c;
    }
    template <class W, class A>
    bool test_eq(W && wanted, A && got)
    {
        return test_comp(std::forward<W>(wanted), std::forward<A>(got),
                         [](auto && a, auto && b) { return every(a==b); });
    }
    template <class W, class A>
    double test_rel_error(W && wanted, A && got, double req_err=0)
    {
        double e = amax(where(isnan(wanted),
                              where(isnan(got), 0., std::numeric_limits<double>::infinity()),
                              rel_error(wanted, got)));
        info("rel_error(wanted: ", wanted, ", got: ", got, ") = ", format_error(e), ", req. ", req_err)
            .test(e<=req_err);
        return e;
    }
    template <class W, class A>
    double test_abs_error(W && wanted, A && got, double req_err=0)
    {
        double e = amax(where(isnan(wanted),
                              where(isnan(got), 0., std::numeric_limits<double>::infinity()),
                              abs(wanted-got)));
        info("abs_error(wanted: ", wanted, ", got: ", got, ") = ", format_error(e), ", req. ", req_err)
            .test(e<=req_err);
        return e;
    }
    int summary() const
    {
        if (failed.empty()) {
            o << esc_bold << esc_green << total << " tests passed";
        } else {
            o << esc_bold << esc_red << failed.size() << " tests of " << total << " failed (" << rawp(failed) << ")";
        }
        o << esc_plain << esc_unbold << std::flush;
        return failed.size();
    }
};

#endif // RA_TEST_H
