// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Testing and benchmarking.

// (c) Daniel Llorens - 2012-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <string>
#include <iomanip>
#include <iostream>
#include <chrono>
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
} // namespace ra::esc

struct TestRecorder
{
    constexpr static double QNAN = std::numeric_limits<double>::quiet_NaN();
    constexpr static double PINF = std::numeric_limits<double>::infinity();

// ra::amax ignores nans and we don't want that here.
    __attribute__((optimize("-fno-finite-math-only")))
    static auto
    amax_strict(auto const & a)
    {
        using T = ncvalue_t<decltype(a)>;
        T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
        return early(map([&c](auto const & a){ if (c<a) c=a; return isnan(a) ? std::make_optional(QNAN*a) : std::nullopt; }, a), c);
    }

    enum verbose_t { QUIET, // as NOISY if failed, else no output
                     ERRORS, // as NOISY if failed, else info and fp errors (default)
                     NOISY }; // full output of info, test arguments, fp errors

    std::ostream & o;
    verbose_t verbose_default, verbose;
    bool willskip=false, willexpectfail=false, willstrict=false;
    int total=0, skipped=0, passed_good=0, passed_bad=0, failed_good=0, failed_bad=0;
    std::vector<int> bad;
    std::string infos;

    TestRecorder(std::ostream & o_=std::cout, verbose_t verbose_default_=ERRORS)
        : o(o_), verbose_default(verbose_default_), verbose(verbose_default_) {}

    void
    section(auto const & ... a)
    {
        print(o, esc::bold, a ..., esc::unbold, "\n");
    }
    static std::string
    format_error(double e, char const * col = esc::yellow)
    {
        return std::format("{}{:.2}{}", col, e, esc::reset);
    }
    TestRecorder &
    info(auto && ... a)
    {
        bool empty = (infos=="");
        infos += format(esc::pink, (empty ? "" : "; "), a ..., esc::reset);
        return *this;
    }
    TestRecorder & quiet(verbose_t v=QUIET) { verbose = v; return *this; }
    TestRecorder & noisy(verbose_t v=NOISY) { verbose = v; return *this; }
    TestRecorder & skip(bool s=true) { willskip = s; return *this; }
    TestRecorder & strict(bool s=true) { willstrict = s; return *this; }
    TestRecorder & expectfail(bool s=true) { willexpectfail = s; return *this; }

#define RA_CURRENT_LOC std::source_location const loc = std::source_location::current()
#define RA_LAZYINFO(...) [&]{ return format(infos, (infos=="" ? "" : "; "), __VA_ARGS__); }

    void
    test(bool c, auto && info_full, auto && info_min, RA_CURRENT_LOC)
    {
        switch (verbose) {
        case QUIET: {
            if (!c) {
                print(o, esc::cyan, total, ":", loc, "]", esc::reset, " ...", esc::bold, esc::red, " FAIL", esc::reset,
                      esc::yellow, (willskip ? " skipped" : ""), (willexpectfail ? " expected" : ""), esc::reset,
                      " ", info_full(), "\n");
            }
        }; break;
        case NOISY: case ERRORS: {
            print(o, esc::cyan, "[", total, ":", loc, "]", esc::reset, " ...",
                  (c ? std::string(esc::green) + " ok" + esc::reset
                   : std::string(esc::bold) + esc::red + " FAIL" + esc::reset),
                  esc::yellow, (willskip ? " skipped" : ""),
                  (willexpectfail ? (c ? " not expected" : " expected") : ""), esc::reset,
                  " ", (verbose==NOISY || c==willexpectfail ? info_full() : info_min()), "\n");
        }; break;
        default: std::abort();
        }
        infos = "";
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
        willstrict = willskip = willexpectfail = false;
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
        return test_scomp(ref, a, [](auto && a, auto && b){ return a==b; }, "should be strictly ==", loc);
    }

// where() is used to match shapes if either REF or A don't't have one.
    bool
    test_comp(auto && a, auto && b, auto && comp, char const * msg, RA_CURRENT_LOC)
    {
        if (willstrict
            ? [&]{
                if constexpr (ra::rank_s(a)==ra::rank_s(b) || ra::rank_s(a)==ANY || ra::rank_s(b)==ANY) {
                    return ra::rank(a)==ra::rank(b) && every(ra::iter(ra::shape(a))==ra::shape(b));
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
                 RA_LAZYINFO("Mismatched shapes [", fmt(nstyle, ra::shape(a)), "] [", fmt(nstyle, ra::shape(b)), "]",
                             willstrict ? " (strict shape)" : ""),
                 RA_LAZYINFO("Mismatched shapes", willstrict ? " (strict shape)" : ""),
                 loc);
            return false;
        }
    }
#define RA_TEST_COMP(NAME, OP)                                          \
    bool                                                                \
    NAME(auto && ref, auto && a, RA_CURRENT_LOC)                        \
    {                                                                   \
        return test_comp(ra::iter(ref), ra::iter(a), [](auto && a, auto && b){ return every(a OP b); }, \
                         "should be " RA_STRINGIZE(OP), loc);              \
    }
    RA_TEST_COMP(test_eq, ==)
    RA_TEST_COMP(test_lt, <)
    RA_TEST_COMP(test_le, <=)
    RA_TEST_COMP(test_gt, >)
    RA_TEST_COMP(test_ge, >=)
#undef RA_TEST_COMP

    __attribute__((optimize("-fno-finite-math-only")))
    double
    test_rel(auto && ref_, auto && a_, double req, double level=0, RA_CURRENT_LOC)
    {
        decltype(auto) ref = ra::iter(ref_);
        decltype(auto) a = ra::iter(a_);
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
             RA_LAZYINFO("rerr (", esc::yellow, "ref", esc::reset, ": ", ref, esc::yellow, ", got", esc::reset,
                         ": ", a, ") = ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")),
                         ", req. ", format_error(req, esc::plain)),
             RA_LAZYINFO("rerr: ", format_error(e), (level<=0 ? "" : format(" (level ", level, ")")),
                         ", req. ", format_error(req, esc::plain)),
             loc);
        return e;
    }
    __attribute__((optimize("-fno-finite-math-only")))
    double
    test_abs(auto && ref_, auto && a_, double req=0, RA_CURRENT_LOC)
    {
        decltype(auto) ref = ra::iter(ref_);
        decltype(auto) a = ra::iter(a_);
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
        print(o, "--------------\nTests end ", buf, ". ",
              "Of ", total, " tests passed ", (passed_good+passed_bad),
              " (", passed_bad, " unexpected), failed ", (failed_good+failed_bad),
              " (", failed_bad, " unexpected), skipped ", skipped, ".\n");
        if (bad.size()>0) {
            print(o, bad.size(), " bad tests: [", esc::bold, esc::red, fmt(nstyle, bad), esc::reset, "].\n");
        }
        return bad.size();
    }
};

// TODO measure empty loops, better reporting. Let benchmarked functions return results
struct Benchmark
{
    using clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                     std::chrono::high_resolution_clock,
                                     std::chrono::steady_clock>;

    static clock::duration
    lapse(clock::duration empty, clock::duration full)
    {
        return (full>empty) ? full-empty : full;
    }
    static double
    toseconds(clock::duration const & t)
    {
        return std::chrono::duration<float, std::ratio<1, 1>>(t).count();
    }

    struct Value
    {
        std::string name;
        int repeats;
        clock::duration empty;
        ra::Big<clock::duration, 1> times;
    };

    static double
    avg(Value const & bv)
    {
        return toseconds(sum(bv.times))/bv.repeats/bv.times.size();
    }
    static double
    stddev(Value const & bv)
    {
        return sqrt(sum(sqr(ra::map(toseconds, bv.times)/bv.repeats-avg(bv)))/bv.times.size());
    }
    static std::string
    report(Value const & bv, double scale=1., double u=1e-9)
    {
        return std::format("{0:3f} {2} [{1:.2f}]", avg(bv)/scale/u, stddev(bv)/avg(bv),
                           1e-9==u ? "ns" : 1e-6==u ? "us" : 1e-3==u ? "ms" : 1==u ? "s" : "?");
    }
    void
    report(std::ostream & o, auto const & b, double frac)
    {
        std::println(o, "{}{::4.2}", (infos=="" ? "" : infos + " : "), iter(ra::map(avg, b)/frac));
        std::println(o, "{}{::4.2}", (infos=="" ? "" : infos + " : "), iter(ra::map(stddev, b)/frac));
        infos = "";
    }

    int repeats_ = 1, runs_ = 1;
    std::string const name_ = "";
    std::string infos = "";

    Benchmark &
    info(auto && ... a)
    {
        bool empty = (infos=="");
        infos += format(ra::esc::plain, (empty ? "" : "; "), a ..., ra::esc::plain);
        return *this;
    }

    Benchmark name(std::string name_) { return Benchmark { repeats_, runs_, name_, "" }; }
    Benchmark repeats(int repeats_) { return Benchmark { repeats_, runs_, name_, "" }; }
    Benchmark runs(int runs_) { return Benchmark { repeats_, runs_, name_, "" }; }

    auto
    once(auto && f, auto && ... a)
    {
        auto t0 = clock::now();
        clock::duration empty = clock::now()-t0;

        ra::Big<clock::duration, 1> times;
        for (int k=0; k<runs_; ++k) {
            auto t0 = clock::now();
            for (int i=0; i<repeats_; ++i) { f(a ...); }
            clock::duration full = clock::now()-t0;
            times.push_back(lapse(empty, full));
        }
        return Value { name_, repeats_, empty, std::move(times) };
    }
    auto
    once_f(auto && g, auto && ... a)
    {
        clock::duration empty;
        g([&](auto && f) {
              auto t0 = clock::now();
              empty = clock::now()-t0;
          }, a ...);

        ra::Big<clock::duration, 1> times;
        for (int k=0; k<runs_; ++k) {
            g([&](auto && f) {
                auto t0 = clock::now();
                for (int i=0; i<repeats_; ++i) { f(); }
                clock::duration full = clock::now()-t0;
                times.push_back(lapse(empty, full));
            }, a ...);
        }
        return Value { name_, repeats_, empty, std::move(times) };
    }
    auto
    run(auto && f, auto && ... a)
    {
        return ra::concrete(ra::from([this, &f](auto && ... b){ return this->once(f, b ...); }, a ...));
    }
    auto
    run_f(auto && f, auto && ... a)
    {
        return ra::concrete(ra::from([this, &f](auto && ... b){ return this->once_f(f, b ...); }, a ...));
    }
};

template <> constexpr bool is_scalar_def<Benchmark::Value> = true;

} // namespace ra
