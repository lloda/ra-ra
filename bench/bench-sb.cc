// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - Benchmark for small-buffer vector

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#include "test/mpdebug.hh"
#include "test/svector.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::Benchmark, ra::dim_t;

namespace ra {

template <rank_t R> using LargeDimv = std::conditional_t<ANY==R, svector<Dim>, std::array<Dim, ANY==R?0:R>>;
template <class T, rank_t R=ANY> using Large = Array<svector<T>, std::conditional_t<1==R, std::array<SDim<dim_t, ic_t<1>>, 1>, LargeDimv<R>>>;
template <class T, rank_t R=ANY> using Bulky = Array<vector_default_init<T>, std::conditional_t<1==R, std::array<SDim<dim_t, ic_t<1>>, 1>, LargeDimv<R>>>;

} // namespace ra

using ra::Big, ra::Large, ra::Bulky;

int main(int argc, char * * argv)
{
    ra::TestRecorder tr(cout);
    int reps = argc>1 ? std::stoi(argv[1]) : 10000;
    std::println(cout, "reps = {}", reps);

    tr.section("svector is fov");
    {
        ra::svector s = { 1, 2, 3 };
        tr.test_eq(ra::iter({1, 2, 3}), s);
    }
    tr.section("types handle the same");
    {
        Big<int, 2> a = {{1, 2}, {3, 4}};
        Large<int, 2> b = {{1, 2}, {3, 4}};
        Bulky<int, 2> c = {{1, 2}, {3, 4}};
        tr.strict().test_eq(a, b);
        tr.strict().test_eq(a, c);
    }

// FIXME copied from bench-at.cc, not really an allocation test.

    auto test = [&tr](auto && C, auto && I, int reps, std::string tag){
        if ("warmup"!=tag) tr.section(tag);
        int M = C.len(0);
        int N = C.len(1);
        int O = I.len(0);
        C = 4*ra::_0 + ra::_1;
        I(ra::all, 0) = map([&](auto && i){ return i%M; }, ra::_0 + (std::rand() & 1));
        I(ra::all, 1) = map([&](auto && i){ return i%N; }, ra::_0 + (std::rand() & 1));

        int ref0 = sum(at(C, iter<1>(I))), val0 = 0;
        Benchmark bm { reps, 3 };
        auto report = [&](std::string const & stag, auto && bv){
            if ("warmup"!=tag) tr.info(Benchmark::report(bv, M*N), " ", stag).test_eq(val0, ref0);
        };

        report("direct subscript",
               bm.run([&]{
                   int val = 0;
                   for (int i=0; i<O; ++i) {
                       val += C(ra::dim_t(I(i, 0)), ra::dim_t(I(i, 1))); // need convert for var rank I
                   }
                   val0 = val;
               }));
        report("at member + loop",
               bm.run([&]{
                   int val = 0;
                   for (int i=0; i<O; ++i) {
                       val += at(C, I(i));
                   }
                   val0 = val;
               }));
        report("at op + iter",
               bm.run([&]{
                   val0 = sum(at(C, iter<1>(I)));
               }));
    };

    tr.section("bench");
    {
        ra::Big<int, 2> bigs({10, 4}, ra::none);
        ra::Big<int> bigd({10, 4}, ra::none);
        ra::Large<int, 2> larges({10, 4}, ra::none);
        ra::Large<int> larged({10, 4}, ra::none);
        ra::Bulky<int, 2> bulkys({10, 4}, ra::none);
        ra::Bulky<int> bulkyd({10, 4}, ra::none);

        test(bigs, bigd, reps, "warmup");

        test(bigs, bigs, reps, "bigs/bigs");
        test(larges, larges, reps, "larges/larges");
        test(bulkys, bulkys, reps, "bulkys/bulkys");

        test(bigd, bigd, reps, "bigd/bigd");
        test(larged, larged, reps, "larged/larged");
        test(bulkyd, bulkyd, reps, "bulkyd/bulkyd");
    }

    return tr.summary();
}
