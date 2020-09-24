// -*- mode: c++; coding: utf-8 -*-
/// @file bench.hh
/// @brief Minimal benchmarking library.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "ra/operators.hh"
#include "ra/io.hh"

/*
  TODO
  - measure empty loops
  - better reporting
  - allow benchmarked functions to return results
*/

struct Benchmark
{
    constexpr static char const * esc_bold = "\x1b[01m";
    constexpr static char const * esc_unbold = "\x1b[0m";
    constexpr static char const * esc_red = "\x1b[31m";
    constexpr static char const * esc_green = "\x1b[32m";
    constexpr static char const * esc_cyan = "\x1b[36m";
    constexpr static char const * esc_yellow = "\x1b[33m";
    constexpr static char const * esc_blue = "\x1b[34m";
    constexpr static char const * esc_white = "\x1b[97m"; // an AIXTERM sequence
    constexpr static char const * esc_plain = "\x1b[39m";
    constexpr static char const * esc_reset = "\x1b[39m\x1b[0m"; // plain + unbold

    using clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                     std::chrono::high_resolution_clock,
                                     std::chrono::steady_clock>;

    static clock::duration
    lapse(clock::duration empty, clock::duration full)
    {
// [ra08] count() to work around https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95242 on gcc 10.
        return (full.count()>empty.count()) ? full-empty : full;
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

    static double avg(Value const & bv)
    {
        return toseconds(sum(bv.times))/bv.repeats/bv.times.size();
    }
    static double stddev(Value const & bv)
    {
        double m = avg(bv);
        return sqrt(sum(sqr(ra::map(toseconds, bv.times)/bv.repeats-m))/bv.times.size());
    }

    template <class B>
    void report(std::ostream & o, B const & b, double frac)
    {
        o << (info_str=="" ? "" : info_str + " : ") << ra::map([](auto && bv) { return avg(bv); }, b)/frac << std::endl;
        o << (info_str=="" ? "" : info_str + " : ") << ra::map([](auto && bv) { return stddev(bv); }, b)/frac << std::endl;
        info_str = "";
    }

    int const repeats_ = 1;
    int const runs_ = 1;
    std::string const name_ = "";
    std::string info_str = "";

    template <class ... A> Benchmark & info(A && ... a)
    {
        bool empty = (info_str=="");
        info_str += esc_plain;
        info_str += (empty ? "" : "; ");
        info_str += ra::format(a ...);
        info_str += esc_plain;
        return *this;
    }

    Benchmark name(std::string name_) { return Benchmark { repeats_, runs_, name_, "" }; }
    Benchmark repeats(int repeats_) { return Benchmark { repeats_, runs_, name_, "" }; }
    Benchmark runs(int runs_) { return Benchmark { repeats_, runs_, name_, "" }; }

    template <class F, class ... A> auto
    once(F && f, A && ... a)
    {
        auto t0 = clock::now();
        clock::duration empty = clock::now()-t0;

        ra::Big<clock::duration, 1> times;
        for (int k=0; k<runs_; ++k) {
            auto t0 = clock::now();
            for (int i=0; i<repeats_; ++i) {
                f(std::forward<A>(a) ...);
            }
            clock::duration full = clock::now()-t0;
            times.push_back(lapse(empty, full));
        }
        return Value { name_, repeats_, empty, std::move(times) };
    }

    template <class G, class ... A> auto
    once_f(G && g, A && ... a)
    {
        clock::duration empty;
        g([&](auto && f)
          {
              auto t0 = clock::now();
              empty = clock::now()-t0;
          }, std::forward<A>(a) ...);

        ra::Big<clock::duration, 1> times;
        for (int k=0; k<runs_; ++k) {
            g([&](auto && f)
              {
                  auto t0 = clock::now();
                  for (int i=0; i<repeats_; ++i) {
                      f();
                  }
                  clock::duration full = clock::now()-t0;
                  times.push_back(lapse(empty, full));
              }, std::forward<A>(a) ...);
        }
        return Value { name_, repeats_, empty, std::move(times) };
    }

    template <class F, class ... A> auto
    run(F && f, A && ... a)
    {
        return ra::concrete(ra::from([this, &f](auto && ... b) { return this->once(f, b ...); }, a ...));
    }

    template <class F, class ... A> auto
    run_f(F && f, A && ... a)
    {
        return ra::concrete(ra::from([this, &f](auto && ... b) { return this->once_f(f, b ...); }, a ...));
    }
};

namespace ra { template <> constexpr bool is_scalar_def<Benchmark::Value> = true; }
