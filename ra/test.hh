// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Test library.

// (c) Daniel Llorens - 2012-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <string>
#include <limits>
#include <iomanip>
#include <version>
#include <iostream>
#include <ctime>
#include "ra.hh"

namespace ra {

struct TestRecorder
{
// ra::amax ignore nans in the way fmax etc. do, and we don't want that here.
    template <class A>
    inline static auto amax_strict(A && a)
    {
        using std::max;
        using T = value_t<A>;
        constexpr auto QNAN = std::numeric_limits<double>::quiet_NaN();
        T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
        return early(map([&c](auto && a) { if (c<a) { c = a; }; return std::make_tuple(isnan(a), QNAN*a); },
                         std::forward<A>(a)), c);
        return c;
    }

    enum verbose_t { QUIET, // as NOISY if failed, else no output
                     ERRORS, // as NOISY if failed, else info and fp errors (default)
                     NOISY }; // full output of info, test arguments, fp errors
    std::ostream & o;
    int total=0, skipped=0, passed_good=0, passed_bad=0, failed_good=0, failed_bad=0;
    std::vector<int> bad;
    std::string info_str;
    verbose_t verbose_default, verbose;
    bool willskip;
    bool willexpectfail;

    TestRecorder(std::ostream & o_=std::cout, verbose_t verbose_default_=ERRORS)
        : o(o_), verbose_default(verbose_default_), verbose(verbose_default_),
          willskip(false), willexpectfail(false) {}

    template <class ... A> void
    section(A const & ... a)
    {
        o << "\n" << esc_bold << format(a ...) << esc_unbold << std::endl;
    }

    static std::string
    format_error(double e)
    {
        return format(esc_yellow, std::setprecision(2), e, esc_reset);
    }

    template <class ... A> TestRecorder &
    info(A && ... a)
    {
        bool empty = (info_str=="");
        info_str += esc_pink;
        info_str += (empty ? "" : "; ") + format(a ...) + esc_reset;
        return *this;
    }
    TestRecorder & quiet(verbose_t v=QUIET) { verbose = v; return *this; }
    TestRecorder & noisy(verbose_t v=NOISY) { verbose = v; return *this; }
    TestRecorder & skip(bool s=true) { willskip = s; return *this; }
    TestRecorder & expectfail(bool s=true) { willexpectfail = s; return *this; }

    template <class A, class B>
    void
    test(bool c, A && info_full, B && info_min,
         std::source_location const loc = std::source_location::current())
    {
        switch (verbose) {
        case QUIET: {
            if (!c) {
                o << format(esc_cyan, "[", total, ":", loc, "]", esc_reset, " ...",
                            esc_bold, esc_red, " FAILED", esc_reset,
                            esc_yellow, (willskip ? " skipped" : ""), (willexpectfail ? " expected" : ""), esc_reset,
                            " ", info_full())
                  << std::endl;
            }
        }; break;
        case NOISY: case ERRORS: {
            o << format(esc_cyan, "[", total, ":", loc, "]", esc_reset, " ...")
              << (c ? std::string(esc_green) + " ok" + esc_reset
                  : std::string(esc_bold) + esc_red + " FAILED" + esc_reset)
              << esc_yellow << (willskip ? " skipped" : "")
              << (willexpectfail ? (c ? " not expected" : " expected") : "") << esc_reset
              << " " << ((verbose==NOISY || c==willexpectfail) ? info_full() : info_min())
              << std::endl;
        }; break;
        default: assert(0);
        }
        info_str = "";
        verbose = verbose_default;
        if (!willskip) {
            ++(willexpectfail? (c ? passed_bad : failed_good) : (c ? passed_good : failed_bad));
            if (c==willexpectfail) {
                bad.push_back(total);
            }
        } else {
            ++skipped;
        }
        ++total;
        willskip = false;
        willexpectfail = false;
    }

#define LAZYINFO(...) [&]() { return format(info_str, (info_str=="" ? "" : "; "), __VA_ARGS__); }

    template <class A>
    void
    test(bool c, A && info_full,
         std::source_location const loc = std::source_location::current())
    {
        test(c, info_full, info_full, loc);
    }
    void
    test(bool c,
         std::source_location const loc = std::source_location::current())
    {
        test(c, LAZYINFO(""), loc);
    }

// Comp = ... is non-deduced context, so can't replace test_eq() with a default argument here.
// where() is used to match shapes if either REF or A don't't have one.
    template <class A, class B, class Comp>
    bool
    test_comp(A && a, B && b, Comp && comp, char const * msg,
              std::source_location const loc = std::source_location::current())
    {
        if (agree_op(comp, a, b)) {
            bool c = every(ra::map(comp, a, b));
            test(c, LAZYINFO(where(false, a, b), " (", msg, " ", where(true, a, b), ")"),
                 LAZYINFO(""), loc);
            return c;
        } else {
            test(false,
                 LAZYINFO("Mismatched args [", ra::noshape, ra::shape(a), "] [", ra::noshape, ra::shape(b), "]"),
                 LAZYINFO("Shape mismatch"),
                 loc);
            return false;
        }
    }
    template <class R, class A>
    bool
    test_eq(R && ref, A && a,
            std::source_location const loc = std::source_location::current())
    {
        return test_comp(std::forward<R>(ref), std::forward<A>(a), [](auto && a, auto && b) { return every(a==b); },
                         "should be ==", loc);
    }
    template <class A, class B>
    bool
    test_lt(A && a, B && b,
            std::source_location const loc = std::source_location::current())
    {
        return test_comp(std::forward<A>(a), std::forward<B>(b), [](auto && a, auto && b) { return every(a<b); },
                         "should be <", loc);
    }
    template <class A, class B>
    bool
    test_le(A && a, B && b,
            std::source_location const loc = std::source_location::current())
    {
        return test_comp(std::forward<A>(a), std::forward<B>(b), [](auto && a, auto && b) { return every(a<=b); },
                         "should be <=", loc);
    }
// These two are included so that the first argument can remain the reference.
    template <class A, class B>
    bool
    test_gt(A && a, B && b,
            std::source_location const loc = std::source_location::current())
    {
        return test_comp(std::forward<A>(a), std::forward<B>(b), [](auto && a, auto && b) { return every(a>b); },
                         "should be >", loc);
    }
    template <class A, class B>
    bool
    test_ge(A && a, B && b,
            std::source_location const loc = std::source_location::current())
    {
        return test_comp(std::forward<A>(a), std::forward<B>(b), [](auto && a, auto && b) { return every(a>=b); },
                         "should be >=", loc);
    }
    template <class R, class A>
    double
    test_rel_error(R && ref, A && a, double req, double level=0,
                   std::source_location const loc = std::source_location::current())
    {
        double e = (level<=0)
            ? amax_strict(where(isnan(ref),
                                where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                                rel_error(ref, a)))
            : amax_strict(where(isnan(ref),
                                where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                                abs(ref-a)/level));
        test(e<=req,
             LAZYINFO("rerr (", esc_yellow, "ref", esc_reset, ": ", ref, esc_yellow, ", got", esc_reset, ": ", a,
                      ") = ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")), ", req. ", req),
             LAZYINFO("rerr: ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")),
                      ", req. ", req),
             loc);
        return e;
    }
    template <class R, class A>
    double
    test_abs_error(R && ref, A && a, double req=0,
                   std::source_location const loc = std::source_location::current())
    {
        double e = amax_strict(where(isnan(ref),
                                     where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                                     abs(ref-a)));
        test(e<=req,
             LAZYINFO("aerr (ref: ", ref, ", got: ", a, ") = ", format_error(e), ", req. ", req),
             LAZYINFO("aerr: ", format_error(e), ", req. ", req),
             loc);
        return e;
    }

#undef LAZYINFO

    int
    summary() const
    {
        std::time_t t = std::time(nullptr);
        tm * tmp = std::localtime(&t);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmp);
        o << "--------------\nTests end " << buf << ". ";
        o << format("Of ", total, " tests passed ", (passed_good+passed_bad),
                    " (", passed_bad, " unexpected), failed ", (failed_good+failed_bad),
                    " (", failed_bad, " unexpected), skipped ", skipped, ".\n");
        if (bad.size()>0) {
            o << format(bad.size(), " bad tests: [", esc_bold, esc_red, ra::noshape, format_array(bad),
                        esc_reset, "].\n");
        }
        return bad.size();
    }
};

} // namespace ra
