// -*- mode: c++; coding: utf-8 -*-
/// @file test.hh
/// @brief Minimal test library.

// (c) Daniel Llorens - 2012, 2014-2016
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
#if __cpp_lib_source_location >= 201907L
#include <source_location>
#endif
#include "ra/format.hh"
#include "ra/operators.hh"
#include "ra/io.hh"

namespace ra {

template <> constexpr bool is_scalar_def<std::string> = true;

#if __cpp_lib_source_location >= 201907L
using source_location = std::source_location;
#else
struct source_location { constexpr static source_location current() { return source_location {}; } };
#endif

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

    static std::string format_location(source_location const loc)
    {
#if __cpp_lib_source_location >= 201907L
        return format(" ", loc.file_name(), ":", loc.line(), ",", loc.column());
#else
        return "";
#endif
    }

    template <class ... A> TestRecorder & info(A && ... a)
    {
        bool empty = (info_str=="");
        info_str += esc_pink;
        info_str += (empty ? "" : "; ");
        info_str += format(a ...);
        info_str += esc_plain;
        return *this;
    }
    TestRecorder & quiet(verbose_t v=QUIET) { verbose = v; return *this; }
    TestRecorder & noisy(verbose_t v=NOISY) { verbose = v; return *this; }
    TestRecorder & skip(bool s=true) { willskip = s; return *this; }

    template <class A, class B>
    void test(bool c, A && info_full, B && info_min,
              source_location const loc = source_location::current())
    {
        switch (verbose) {
        case QUIET: {
            if (!c) {
                o << esc_cyan << "[" << (willskip ? std::string("skipped") : format(total))
                  << format_location(loc) << "]" << esc_plain << " ... "
                  << esc_bold << esc_red << "FAILED" << esc_reset
                  << " " << info_full() << std::endl;
            }
        }; break;
        case NOISY: case ERRORS: {
            o << esc_cyan << "[" << (willskip ? std::string("skipped") : format(total))
              << format_location(loc) << "]" << esc_plain << " ... "
              << (c ? std::string(esc_green) + "ok" + esc_plain
                  : std::string(esc_bold) + esc_red + "FAILED" + esc_reset)
              << " " << ((verbose==NOISY || !c) ? info_full() : info_min()) << std::endl;
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

#define LAZYINFO(...) [&]() { return format(info_str, (info_str=="" ? "" : "; "), __VA_ARGS__); }

    template <class A>
    void test(bool c, A && info_full,
              source_location const loc = source_location::current())
    {
        test(c, info_full, info_full, loc);
    }
    void test(bool c,
              source_location const loc = source_location::current())
    {
        test(c, LAZYINFO(""), loc);
    }

// Comp = ... is non-deduced context, so can't replace test_eq() with a default argument here.
// where() is used to match shapes if either REF or A don't't have one.
    template <class R, class A, class Comp>
    bool test_comp(R && ref, A && a, Comp && comp,
                   source_location const loc = source_location::current())
    {
        bool c = every(ra::map(comp, ref, a));
        test(c, LAZYINFO("comp (ref: ", where(true, ref, a), ", got: ", where(false, ref, a), ")"), LAZYINFO(""),
             loc);
        return c;
    }
    template <class R, class A>
    bool test_eq(R && ref, A && a,
                 source_location const loc = source_location::current())
    {
        return test_comp(std::forward<R>(ref), std::forward<A>(a), [](auto && a, auto && b) { return every(a==b); },
                         loc);
    }
    template <class A, class B>
    bool test_lt(A && a, B && b,
                 source_location const loc = source_location::current())
    {
        bool c = every(a<b);
        test(c, LAZYINFO("comp (", where(true, a, b), " should be < ", where(false, a, b), ")"), LAZYINFO(""), loc);
        return c;
    }
    template <class A, class B>
    bool test_le(A && a, B && b,
                 source_location const loc = source_location::current())
    {
        bool c = every(a<=b);
        test(c, LAZYINFO("comp (", where(true, a, b), " should be <= ", where(false, a, b), ")"), LAZYINFO(""), loc);
        return c;
    }
    template <class R, class A>
    double test_rel_error(R && ref, A && a, double req_err, double level=0,
                          source_location const loc = source_location::current())
    {
        double e = (level<=0)
            ? amax(where(isnan(ref),
                         where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                         rel_error(ref, a)))
            : amax(where(isnan(ref),
                         where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                         abs(ref-a)/level));
        test(e<=req_err,
             LAZYINFO("rerr (", esc_yellow, "ref", esc_plain, ": ", ref, esc_yellow, ", got", esc_plain, ": ", a,
                      ") = ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")), ", req. ", req_err),
             LAZYINFO("rerr: ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")),
                      ", req. ", req_err),
             loc);
        return e;
    }
    template <class R, class A>
    double test_abs_error(R && ref, A && a, double req_err=0,
                          source_location const loc = source_location::current())
    {
        double e = amax(where(isnan(ref),
                              where(isnan(a), 0., std::numeric_limits<double>::infinity()),
                              abs(ref-a)));
        test(e<=req_err,
             LAZYINFO((verbose!=QUIET), "aerr (ref: ", ref, ", got: ", a, ") = ", format_error(e), ", req. ", req_err),
             LAZYINFO("aerr: ", format_error(e), ", req. ", req_err),
             loc);
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
            o << esc_reset << ", " <<  esc_bold << esc_red << " failed " << failed.size()
              << " (" << ra::noshape << format_array(failed) << ")";
        }
        o << esc_reset << std::endl;
        return failed.size();
    }
};

} // namespace ra
