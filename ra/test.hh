// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Test library.

// (c) Daniel Llorens - 2012-2023
// This library is free software; you can redisribute it and/or modify it under
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

namespace esc {
constexpr char const * bold = "\x1b[01m";
constexpr char const * unbold = "\x1b[0m";
constexpr char const * invert = "\x1b[07m";
constexpr char const * underline = "\x1b[04m";
constexpr char const * red = "\x1b[31m";
constexpr char const * green = "\x1b[32m";
constexpr char const * cyan = "\x1b[36m";
constexpr char const * yellow = "\x1b[33m";
constexpr char const * blue = "\x1b[34m";
constexpr char const * white = "\x1b[97m"; // AIXTERM
constexpr char const * plain = "\x1b[39m";
constexpr char const * reset = "\x1b[39m\x1b[0m"; // plain + unbold
constexpr char const * pink = "\x1b[38;5;225m";
} // namespace esc

struct TestRecorder
{
    constexpr static double QNAN = std::numeric_limits<double>::quiet_NaN();
    constexpr static double PINF = std::numeric_limits<double>::infinity();

// ra::amax ignores nans like fmax does, we don't want that here.
    static auto
    amax_strict(auto && a)
    {
        using T = ncvalue_t<decltype(a)>;
        T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
        return early(map([&c](auto && a) { if (c<a) { c=a; }; return isnan(a) ? std::make_optional(QNAN*a) : std::nullopt; },
                         RA_FWD(a)),
                     c);
    }

    enum verbose_t { QUIET, // as NOISY if failed, else no output
                     ERRORS, // as NOISY if failed, else info and fp errors (default)
                     NOISY }; // full output of info, test arguments, fp errors

    std::ostream & o;
    verbose_t verbose_default, verbose;
    bool willskip=false, willexpectfail=false, willstrictshape=false;
    int total=0, skipped=0, passed_good=0, passed_bad=0, failed_good=0, failed_bad=0;
    std::vector<int> bad;
    std::string info_str;

    TestRecorder(std::ostream & o_=std::cout, verbose_t verbose_default_=ERRORS)
        : o(o_), verbose_default(verbose_default_), verbose(verbose_default_) {}

    void
    section(auto const & ... a)
    {
        o << "\n" << esc::bold << format(a ...) << esc::unbold << std::endl;
    }
    static std::string
    format_error(double e)
    {
        return format(esc::yellow, std::setprecision(2), e, esc::reset);
    }
    TestRecorder &
    info(auto && ... a)
    {
        bool empty = (info_str=="");
        info_str += esc::pink;
        info_str += (empty ? "" : "; ") + format(a ...) + esc::reset;
        return *this;
    }
    TestRecorder & quiet(verbose_t v=QUIET) { verbose = v; return *this; }
    TestRecorder & noisy(verbose_t v=NOISY) { verbose = v; return *this; }
    TestRecorder & skip(bool s=true) { willskip = s; return *this; }
    TestRecorder & strictshape(bool s=true) { willstrictshape = s; return *this; }
    TestRecorder & expectfail(bool s=true) { willexpectfail = s; return *this; }

#define RA_CURRENT_LOC std::source_location const loc = std::source_location::current()
#define RA_LAZYINFO(...) [&] { return format(info_str, (info_str=="" ? "" : "; "), __VA_ARGS__); }

    void
    test(bool c, auto && info_full, auto && info_min, RA_CURRENT_LOC)
    {
        switch (verbose) {
        case QUIET: {
            if (!c) {
                o << format(esc::cyan, "[", total, ":", loc, "]", esc::reset, " ...",
                            esc::bold, esc::red, " FAILED", esc::reset,
                            esc::yellow, (willskip ? " skipped" : ""), (willexpectfail ? " expected" : ""), esc::reset,
                            " ", info_full())
                  << std::endl;
            }
        }; break;
        case NOISY: case ERRORS: {
            o << format(esc::cyan, "[", total, ":", loc, "]", esc::reset, " ...")
              << (c ? std::string(esc::green) + " ok" + esc::reset
                  : std::string(esc::bold) + esc::red + " FAILED" + esc::reset)
              << esc::yellow << (willskip ? " skipped" : "")
              << (willexpectfail ? (c ? " not expected" : " expected") : "") << esc::reset
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
    void
    test(bool c, auto && info_full, RA_CURRENT_LOC)
    {
        test(c, info_full, info_full, loc);
    }
    void
    test(bool c, RA_CURRENT_LOC)
    {
        test(c, RA_LAZYINFO(""), loc);
    }

    bool
    test_scomp(auto && a, auto && b, auto && comp, char const * msg, RA_CURRENT_LOC)
    {
        bool c = comp(a, b);
        test(c, RA_LAZYINFO(b, " (", msg, " ", a, ")"), RA_LAZYINFO(""), loc);
        return c;
    }
    bool
    test_seq(auto && ref, auto && a, RA_CURRENT_LOC)
    {
        return test_scomp(ref, a, [](auto && a, auto && b) { return a==b; }, "should be strictly ==", loc);
    }

// Comp = ... is non-deduced context, so can't replace test_eq() with a default argument here.
// where() is used to match shapes if either REF or A don't't have one.
    bool
    test_comp(auto && a, auto && b, auto && comp, char const * msg, RA_CURRENT_LOC)
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
                 RA_LAZYINFO("Mismatched shapes [", ra::noshape, ra::shape(a), "] [", ra::noshape, ra::shape(b), "]",
                             willstrictshape ? " (strict shape)" : ""),
                 RA_LAZYINFO("Mismatched shapes", willstrictshape ? " (strict shape)" : ""),
                 loc);
            return false;
        }
    }
#define RA_TEST_COMP(NAME, OP)                                          \
    bool                                                                \
    JOIN(test_, NAME)(auto && ref, auto && a, RA_CURRENT_LOC)           \
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

    double
    test_rel(auto && ref_, auto && a_, double req, double level=0, RA_CURRENT_LOC)
    {
        decltype(auto) ref = ra::start(ref_);
        decltype(auto) a = ra::start(a_);
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
             RA_LAZYINFO("rerr (", esc::yellow, "ref", esc::reset, ": ", ref, esc::yellow, ", got", esc::reset, ": ", a,
                         ") = ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")), ", req. ", req),
             RA_LAZYINFO("rerr: ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")),
                         ", req. ", req),
             loc);
        return e;
    }
    double
    test_abs(auto && ref_, auto && a_, double req=0, RA_CURRENT_LOC)
    {
        decltype(auto) ref = ra::start(ref_);
        decltype(auto) a = ra::start(a_);
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
            o << format(bad.size(), " bad tests: [", esc::bold, esc::red, ra::noshape, format_array(bad),
                        esc::reset, "].\n");
        }
        return bad.size();
    }
};

} // namespace ra
