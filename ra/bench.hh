// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Benchmarking library.

// (c) Daniel Llorens - 2017-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "test.hh"

// TODO measure empty loops
// TODO better reporting
// TODO allow benchmarked functions to return results

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
        double m = avg(bv);
        return sqrt(sum(sqr(ra::map(toseconds, bv.times)/bv.repeats-m))/bv.times.size());
    }
    template <class B> void
    report(std::ostream & o, B const & b, double frac)
    {
        o << (info_str=="" ? "" : info_str + " : ") << ra::map([](auto && bv) { return avg(bv); }, b)/frac << std::endl;
        o << (info_str=="" ? "" : info_str + " : ") << ra::map([](auto && bv) { return stddev(bv); }, b)/frac << std::endl;
        info_str = "";
    }

    int const repeats_ = 1;
    int const runs_ = 1;
    std::string const name_ = "";
    std::string info_str = "";

    template <class ... A> Benchmark &
    info(A && ... a)
    {
        bool empty = (info_str=="");
        info_str += ra::esc::plain;
        info_str += (empty ? "" : "; ");
        info_str += ra::format(a ...);
        info_str += ra::esc::plain;
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
                f(RA_FWD(a) ...);
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
          }, RA_FWD(a) ...);

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
              }, RA_FWD(a) ...);
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
