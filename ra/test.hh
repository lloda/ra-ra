// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Test library.

// (c) Daniel Llorens - 2012-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <string>
#include <iomanip>
#include <iostream>
#include <ctime>
#include "ra.hh"

namespace ra {

struct TestRecorder
{
    constexpr static double QNAN = std::numeric_limits<double>::quiet_NaN();
    constexpr static double PINF = std::numeric_limits<double>::infinity();

// ra::amax ignore nans in the way fmax etc. do, and we don't want that here.
    template <class A>
    inline static auto
    amax_strict(A && a)
    {
        using T = value_t<A>;
        T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
        return early(map([&c](auto && a) { if (c<a) { c=a; }; return isnan(a) ? std::make_optional(QNAN*a) : std::nullopt; },
                         std::forward<A>(a)),
                     c);
    }

    enum verbose_t { QUIET, // as NOISY if failed, else no output
                     ERRORS, // as NOISY if failed, else info and fp errors (default)
                     NOISY }; // full output of info, test arguments, fp errors
    std::ostream & o;
    int total=0, skipped=0, passed_good=0, passed_bad=0, failed_good=0, failed_bad=0;
    std::vector<int> bad;
    std::string info_str;
    verbose_t verbose_default, verbose;
    bool willskip = false;
    bool willexpectfail = false;
    bool willstrictshape = false;

    TestRecorder(std::ostream & o_=std::cout, verbose_t verbose_default_=ERRORS)
        : o(o_), verbose_default(verbose_default_), verbose(verbose_default_) {}

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
    TestRecorder & strictshape(bool s=true) { willstrictshape = s; return *this; }
    TestRecorder & expectfail(bool s=true) { willexpectfail = s; return *this; }

#define RA_CURRENT_LOC std::source_location const loc = std::source_location::current()
#define RA_LAZYINFO(...) [&] { return format(info_str, (info_str=="" ? "" : "; "), __VA_ARGS__); }

    template <class A, class B>
    void
    test(bool c, A && info_full, B && info_min, RA_CURRENT_LOC)
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
        default: std::abort();
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
        willstrictshape = willskip = willexpectfail = false;
    }

    template <class A>
    void
    test(bool c, A && info_full, RA_CURRENT_LOC)
    {
        test(c, info_full, info_full, loc);
    }
    void
    test(bool c, RA_CURRENT_LOC)
    {
        test(c, RA_LAZYINFO(""), loc);
    }

    template <class A, class B, class Comp>
    bool
    test_scomp(A && a, B && b, Comp && comp, char const * msg, RA_CURRENT_LOC)
    {
        bool c = comp(a, b);
        test(c, RA_LAZYINFO(b, " (", msg, " ", a, ")"), RA_LAZYINFO(""), loc);
        return c;
    }
    template <class R, class A>
    bool
    test_seq(R && ref, A && a, RA_CURRENT_LOC)
    {
        return test_scomp(ref, a, [](auto && a, auto && b) { return a==b; }, "should be strictly ==", loc);
    }

// Comp = ... is non-deduced context, so can't replace test_eq() with a default argument here.
// where() is used to match shapes if either REF or A don't't have one.
    template <class A, class B, class Comp>
    bool
    test_comp(A && a, B && b, Comp && comp, char const * msg, RA_CURRENT_LOC)
    {
        if (willstrictshape
            ? [&] {
                if constexpr (ra::rank_s<decltype(a)>()==ra::rank_s<decltype(b)>()
                              || ra::rank_s<decltype(a)>()==ANY || ra::rank_s<decltype(b)>()==ANY) {
                    return ra::rank(a)==ra::rank(b) && every(ra::start(ra::shape(a))==ra::shape(b));
                } else {
                    return false;
                } }()
            : agree_op(comp, a, b)) {

            bool c = every(ra::map(comp, a, b));
            test(c,
                 RA_LAZYINFO(where(false, a, b), " (", msg, " ", where(true, a, b), ")"),
                 RA_LAZYINFO(""),
                 loc);
            return c;
        } else {
            test(false,
                 RA_LAZYINFO("Mismatched args [", ra::noshape, ra::shape(a), "] [", ra::noshape, ra::shape(b), "]",
                             willstrictshape ? " (strict shape)" : ""),
                 RA_LAZYINFO("Shape mismatch", willstrictshape ? " (strict shape)" : ""),
                 loc);
            return false;
        }
    }
#define RA_TEST_COMP(NAME, OP)                                          \
    template <class R, class A>                                         \
    bool                                                                \
    JOIN(test_, NAME)(R && ref, A && a, RA_CURRENT_LOC)                 \
    {                                                                   \
        return test_comp(ra::start(ref), ra::start(a), [](auto && a, auto && b) { return every(a OP b); }, \
                         "should be " STRINGIZE(OP), loc);              \
    }
    RA_TEST_COMP(eq, ==)
    RA_TEST_COMP(lt, <)
    RA_TEST_COMP(le, <=)
    RA_TEST_COMP(gt, >)
    RA_TEST_COMP(ge, >=)
#undef RA_TEST_COMP

    template <class R, class A>
    double
    test_rel(R && ref, A && a, double req, double level=0, RA_CURRENT_LOC)
    {
        double e = (level<=0)
            ? amax_strict(where(isfinite(ref),
                                rel_error(ref, a),
                                where(isinf(ref),
                                      where(ref==a, 0., PINF),
                                      where(isnan(a), 0., PINF))))
            : amax_strict(where(isfinite(ref),
                                abs(ref-a)/level,
                                where(isinf(ref),
                                      where(ref==a, 0., PINF),
                                      where(isnan(a), 0., PINF))));
        test(e<=req,
             RA_LAZYINFO("rerr (", esc_yellow, "ref", esc_reset, ": ", ref, esc_yellow, ", got", esc_reset, ": ", a,
                         ") = ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")), ", req. ", req),
             RA_LAZYINFO("rerr: ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")),
                         ", req. ", req),
             loc);
        return e;
    }
    template <class R, class A>
    double
    test_abs(R && ref, A && a, double req=0, RA_CURRENT_LOC)
    {
        double e = amax_strict(where(isfinite(ref),
                                     abs(ref-a),
                                     where(isinf(ref),
                                           where(ref==a, 0., PINF),
                                           where(isnan(a), 0., PINF))));
        test(e<=req,
             RA_LAZYINFO("aerr (ref: ", ref, ", got: ", a, ") = ", format_error(e), ", req. ", req),
             RA_LAZYINFO("aerr: ", format_error(e), ", req. ", req),
             loc);
        return e;
    }

#undef RA_CURRENT_LOC
#undef RA_LAZYINFO

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
