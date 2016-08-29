
// (c) Daniel Llorens - 2012, 2014-2016

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
#include "ra/operators.H"

char const * esc_bold = "\x1b[01m";
char const * esc_unbold = "\x1b[0m";
char const * esc_red = "\x1b[31m";
char const * esc_green = "\x1b[32m";
char const * esc_cyan = "\x1b[36m";
char const * esc_yellow = "\x1b[33m";
char const * esc_blue = "\x1b[34m";
char const * esc_white = "\x1b[97m"; // an AIXTERM sequence
char const * esc_plain = "\x1b[39m";
char const * esc_reset = "\x1b[39m\x1b[0m"; // plain + unbold

template <class ... A> void section(A const & ... a)
{
    std::cout << "\n" << esc_bold << format(a ...) << esc_unbold << std::endl;
}

struct TestRecorder
{
    enum verbose_t { QUIET, // as NOISY if failed, else no output
                     ERRORS, // as NOISY if failed, else info and fp errors (default)
                     NOISY }; // full output of info, test arguments, fp errors
    std::ostream & o;
    int total = 0, skipped = 0;
    std::vector<int> failed;
    std::string info_str;
    verbose_t verbose_default, verbose;
    bool willskip;

    TestRecorder(std::ostream & o_=std::cout, verbose_t verbose_default_=ERRORS)
        : o(o_), verbose_default(verbose_default_), verbose(verbose_default_), willskip(false) {}

    template <class ... A> void section(A const & ... a)
    {
        o << "\n" << esc_bold << format(a ...) << esc_unbold << std::endl;
    }

    static std::string format_error(double e)
    {
        return format(esc_yellow, std::setprecision(2), e, esc_plain);
    }

    template <class ... A> TestRecorder & info(A && ... a)
    {
        bool empty = (info_str=="");
        info_str += esc_white;
        info_str += (empty ? "" : "; ");
        info_str += format(a ...);
        info_str += esc_plain;
        return *this;
    }
    TestRecorder & quiet(verbose_t v=QUIET) { verbose = v; return *this; }
    TestRecorder & noisy(verbose_t v=NOISY) { verbose = v; return *this; }
    TestRecorder & skip(bool s=true) { willskip = s; return *this; }

    template <class A, class B>
    void test(bool c, A && info_full, B && info_errors)
    {
        switch (verbose) {
        case QUIET: {
            if (!c) {
                o << esc_cyan << "[" << (willskip ? std::string("skipped") : format(total)) << "] " << esc_plain
                  << esc_bold << esc_red << "FAILED" << esc_reset
                  << "... " << info_full() << std::endl;
            }
        }; break;
        case NOISY: case ERRORS: {
            o << esc_cyan << "[" << (willskip ? std::string("skipped") : format(total)) << "] " << esc_plain
              << (c ? std::string(esc_green)+"ok"+esc_plain
                  : std::string(esc_bold)+esc_red+"FAILED"+esc_reset)
              << "... " << (verbose==NOISY ? info_full() : info_errors()) << std::endl;
        }; break;
        default: assert(0);
        }
        info_str = "";
        verbose = verbose_default;
        if (!willskip) {
            if (!c) {
                failed.push_back(total);
            }
            ++total;
        } else {
            ++skipped;
        }
        willskip = false;
    }
    template <class A> void test(bool c, A && info_full) { test(c, info_full, info_full); }
    void test(bool c) { test(c, [this]() { return this->info_str; }); }

#define LAZYINFO(...) [&]() { return format(info_str, (this->info_str=="" ? "" : "; "), __VA_ARGS__); }

// Comp = ... is non-deduced context, so can't replace test_eq() with a default argument here.
// where() is used to match shapes if either ref or a don't't have one.
    template <class R, class A, class Comp>
    bool test_comp(R && ref, A && a, Comp && comp)
    {
        bool c = every(ra::map(comp, ref, a));
        test(c, LAZYINFO("comp(ref: ", where(true, ref, a), ", got: ", where(false, ref, a), ")"), LAZYINFO(""));
        return c;
    }
    template <class R, class A>
    bool test_eq(R && ref, A && a)
    {
        return test_comp(std::forward<R>(ref), std::forward<A>(a), [](auto && a, auto && b) { return every(a==b); });
    }
    template <class A, class B>
    bool test_le(A && a, B && b)
    {
        bool c = every(a<=b);
        test(c, LAZYINFO("comp(", where(true, a, b), " should be <= ", where(false, a, b), ")"), LAZYINFO(""));
        return c;
    }
    template <class R, class A>
    double test_rel_error(R && ref, A && a, double req_err=0)
    {
        double e = amax(where(isnan(ref),
                              where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                              rel_error(ref, a)));
        test(e<=req_err,
             LAZYINFO("rel_error(ref: ", ref, ", got: ", a, ") = ", format_error(e), ", req. ", req_err),
             LAZYINFO("rel_error: ", format_error(e), ", req. ", req_err));
        return e;
    }
    template <class R, class A>
    double test_abs_error(R && ref, A && a, double req_err=0)
    {
        double e = amax(where(isnan(ref),
                              where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                              abs(ref-a)));
        test(e<=req_err,
             LAZYINFO((verbose!=QUIET), "abs_error(ref: ", ref, ", got: ", a, ") = ", format_error(e), ", req. ", req_err),
             LAZYINFO("abs_error: ", format_error(e), ", req. ", req_err));
        return e;
    }

#undef LAZYINFO

    int summary() const
    {
        o << "Of " << total << " tests " << esc_bold << esc_green << "passed " << (total-failed.size());
        if (skipped>0) {
            o << esc_reset << ", " <<  esc_bold << esc_yellow << "skipped " << skipped;
        }
        if (!failed.empty()) {
            o << esc_reset << ", " <<  esc_bold << esc_red << " failed " << failed.size() << " (" << rawp(failed) << ")";
        }
        o << esc_plain << esc_unbold << std::endl;
        return failed.size();
    }
};

#endif // RA_TEST_H
