
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench.H
/// @brief Minimal benchmarking library.

#pragma once
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "ra/operators.H"
#include "ra/io.H"

/*
  TODO
  - measure empty loops
  - better reporting
  - allow benchmarked functions to return results
*/

struct Benchmark
{
    using clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                     std::chrono::high_resolution_clock,
                                     std::chrono::steady_clock>;

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
        return sqrt(sum(sqr(map(toseconds, bv.times)/bv.repeats-m))/bv.times.size());
    }

    template <class B>
    static void report(std::ostream & o, B const & b, double frac)
    {
        o << map([](auto && bv) { return avg(bv); }, b)/frac << std::endl;
        o << map([](auto && bv) { return stddev(bv); }, b)/frac << std::endl;
    }

    int const repeats_ = 1;
    int const runs_ = 1;
    std::string const name_ = "";

    Benchmark name(std::string name_) { return Benchmark { repeats_, runs_, name_ }; }
    Benchmark repeats(int repeats_) { return Benchmark { repeats_, runs_, name_ }; }
    Benchmark runs(int runs_) { return Benchmark { repeats_, runs_, name_ }; }

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
            times.push_back(full>empty ? full-empty : full);
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
                  times.push_back(full>empty ? full-empty : full);
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
