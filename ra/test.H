
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
// @TODO Only for where(); unfortunately I can't declare the scalar version in an earlier header.
#include "ra/ra-operators.H"

char const * esc_bold = "\033[01m";
char const * esc_unbold = "\033[0m";
char const * esc_red = "\033[31m";
char const * esc_green = "\033[32m";
char const * esc_yellow = "\033[33m";
char const * esc_blue = "\033[34m";
char const * esc_plain = "\033[39m";

template <class ... A> void section(A const & ... a)
{
    std::cout << "\n\n" << esc_bold << format(a ...) << esc_unbold << std::endl;
}

struct TestRecorder
{
    std::ostream & o;
    int total, failed;
    std::string info_string;
    bool isquiet_default, isquiet;

    TestRecorder(std::ostream & o_=std::cout, bool isquiet_default_=false)
        : o(o_), total(0), failed(0), isquiet_default(isquiet_default_),
          isquiet(isquiet_default_) {}

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

    void test(bool c)
    {
        ++total;
        failed += !c;
        if (isquiet) {
            if (!c) {
                o << std::string(esc_bold)+esc_red+"FAILED"+esc_plain+esc_unbold
                  << "... " << info_string << std::endl;
            }
        } else {
            o << (c ? std::string(esc_green)+"ok"+esc_plain
                  : std::string(esc_bold)+esc_red+"FAILED"+esc_plain+esc_unbold)
              << "... " << info_string << std::endl;
        }
        info_string = "";
        isquiet = isquiet_default;
    }
    void operator+=(bool c)
    {
        test(c);
    }
    template <class W, class A>
    bool test_equal(W && wanted, A && got)
    {
        bool c = every(wanted==got);
// where() to match shapes if either doesn't have one
        info("equal(wanted: ", where(true, wanted, got), ", got: ", got, ")")
            .test(c);
        return c;
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
        if (failed>0) {
            o << esc_bold << esc_red << failed << " tests of " << total << " failed";
        } else {
            o << esc_bold << esc_green << total << " tests passed";
        }
        o << esc_plain << esc_unbold << std::flush;
        return failed;
    }
};

#endif // RA_TEST_H
