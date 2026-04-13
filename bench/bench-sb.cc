// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - Benchmark for various Array::store

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#include "test/mpdebug.hh"
#include "test/svector.hh"
#include <memory_resource>

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::Benchmark, ra::dim_t;

namespace ra {

template <rank_t R> using LargeDimv = std::conditional_t<ANY==R, svector<Dim>, std::array<Dim, ANY==R?0:R>>;
template <rank_t R> using Large1Dimv = std::conditional_t<1==R, std::array<SDim<dim_t, ic_t<1>>, 1>, LargeDimv<R>>;
template <class T, rank_t R=ANY> using Large = Array<svector<T>, Large1Dimv<R>>;
template <class T, rank_t R=ANY> using Bulky = Array<vector_default_init<T>, Large1Dimv<R>>;
template <class T, rank_t R=ANY> using Pmr = Array<std::pmr::vector<T>, Large1Dimv<R>>;

} // namespace ra

using ra::Big, ra::Large, ra::Bulky, ra::Pmr;

int main(int argc, char * * argv)
{
    ra::TestRecorder tr(cout);
    int reps = argc>1 ? std::stoi(argv[1]) : 100000;
    std::println(cout, "reps = {}", reps);

    tr.section("svector is fov");
    {
        ra::svector s = { 1, 2, 3 };
        tr.test_eq(ra::iter({1, 2, 3}), s);
    }
    tr.section("other svector behavior");
    {
        ra::svector<int, 4> a = { 1, 2, 3 };
        ra::svector<int, 4> b = std::move(a);
        tr.test_eq(size_t(4), b.capacity());
    }
    tr.section("types handle the same");
    {
        Big<int, 2> a = {{1, 2}, {3, 4}};
        Large<int, 2> b = {{1, 2}, {3, 4}};
        Bulky<int, 2> c = {{1, 2}, {3, 4}};
        Pmr<int, 2> d = {{1, 2}, {3, 4}};
        tr.strict().test_eq(a, b);
        tr.strict().test_eq(a, c);
        tr.strict().test_eq(a, d);
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
        Big<int, 2> bigs({10, 4}, ra::none);
        Big<int> bigd({10, 4}, ra::none);
        test(bigs, bigd, reps, "warmup");
        test(bigs, bigs, reps, "\nbigs/bigs");
        test(bigd, bigd, reps, "bigd/bigd");

        Large<int, 2> larges({10, 4}, ra::none);
        Large<int> larged({10, 4}, ra::none);
        test(larges, larged, reps, "warmup");
        test(larges, larges, reps, "\nlarges/larges");
        test(larged, larged, reps, "larged/larged");

        Bulky<int, 2> bulkys({10, 4}, ra::none);
        Bulky<int> bulkyd({10, 4}, ra::none);
        test(bulkys, bulkyd, reps, "warmup");
        test(bulkys, bulkys, reps, "\nbulkys/bulkys");
        test(bulkyd, bulkyd, reps, "bulkyd/bulkyd");

        std::byte buf[1000];
        std::pmr::monotonic_buffer_resource mem_res(std::data(buf), std::size(buf));
        std::pmr::set_default_resource(&mem_res);

        Pmr<int, 2> pmrs({10, 4}, ra::none);
        Pmr<int> pmrd({10, 4}, ra::none);
        test(pmrs, pmrd, reps, "warmup");
        test(pmrs, pmrs, reps, "\npmrs/pmrs");
        test(pmrd, pmrd, reps, "pmrd/pmrd");

    }
    return tr.summary();
}
