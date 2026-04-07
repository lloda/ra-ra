// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - Maxwell, 4-vector potential vacuum field equations
// After Chaitin1986, p. 14. Attempt at straight translation from APL.

// (c) Daniel Llorens - 2016-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <thread>
#include <string>
#include "ra/test.hh"
#include "ra/ra.hh"
#include "ra/test.hh"
#include <numbers>

using real = double;
constexpr auto H = ra::all;
template <int n> constexpr ra::dots_t<n> HH = ra::dots<n>;
constexpr auto PI = std::numbers::pi_v<double>;

using std::cout, std::endl, std::println;
using ra::iota, ra::ic, ra::TestRecorder, ra::Benchmark, ra::Big, ra::len;

int main()
{
    TestRecorder tr(cout);

    real delta = 1;
    int o=20, n=20, m=2, l=2;
    Big<real, 5> A({o, n, m, l, 4}, 0.);
    Big<real, 6> DA({o, n, m, l, 4, 4}, 0.);
    Big<real, 6> F({o, n, m, l, 4, 4}, 0.);
    Big<real, 4> divA({o, n, m, l}, 0.);
    Big<real, 4> X({n, m, l, 4}, 0.), Y({n, m, l, 4}, 0.);

    A(0, H, H, H, 2) = -cos(iota(n)*(2*PI/n))/(2*PI/n);
    A(1, H, H, H, 2) = -cos((iota(n)-delta)*(2*PI/n))/(2*PI/n);

    auto t0 = Benchmark::clock::now();
// FIXME painful without a roll operator, but we'd need one without temps.
    for (int t=1; t+1<o; ++t) {
// X←(1⌽[0]A[T;;;;])+(1⌽[1]A[T;;;;])+(1⌽[2]A[T;;;;])
        X(iota(len-1)) = A(t, iota(len-1, 1));
        X(len-1) = A(t, 0);
        X(H, iota(len-1)) += A(t, H, iota(len-1, 1));
        X(H, len-1) += A(t, H, 0);
        X(H, H, iota(len-1)) += A(t, H, H, iota(len-1, 1));
        X(H, H, len-1) += A(t, H, H, 0);
// Y←(¯1⌽[0]A[T;;;;])+(¯1⌽[1]A[T;;;;])+(¯1⌽[2]A[T;;;;])
        Y(iota(len-1, 1)) = A(t, iota(len-1));
        Y(0) = A(t, len-1);
        Y(H, iota(len-1, 1)) += A(t, H, iota(len-1));
        Y(H, 0) += A(t, H, len-1);
        Y(H, H, iota(len-1, 1)) += A(t, H, H, iota(len-1));
        Y(H, H, 0) += A(t, H, H, len-1);

        A(t+1) = X + Y - A(t-1) - 4*A(t);
    }
    auto time_A = Benchmark::clock::now()-t0;

// FIXME should try to traverse the array once, eg explode() = pack(...), but we need to wrap around boundaries.
    auto diff = [&DA, &A, &delta](auto k, real factor){
        if (DA.len(k)>=2) {
            DA(HH<k>, iota(len-2, 1), HH<4-k>, k) = A(HH<k>, iota(len-2, 2)) - A(HH<k>, iota(len-2, 0));
            DA(HH<k>, 0, HH<4-k>, k) = A(HH<k>, 1) - A(HH<k>, len-1);
            DA(HH<k>, len-1, HH<4-k>, k) = A(HH<k>, 0) - A(HH<k>, len-2);
            DA(HH<5>, k) *= factor;
        }
    };

    t0 = Benchmark::clock::now();
    diff(ic<0>, +1/(2*delta));
    diff(ic<1>, -1/(2*delta));
    diff(ic<2>, -1/(2*delta));
    diff(ic<3>, -1/(2*delta));
    auto time_DA = Benchmark::clock::now()-t0;
    F = transpose(DA, ra::ilist<0, 1, 2, 3, 5, 4>) - DA;
    divA += transpose(DA, ra::ilist<0, 1, 2, 3, 4, 4>); // abuse prefix matching to reduce last axis

    tr.info("Lorentz test max div A (1)").test_eq(0., amax(divA));
// an alternative without a temporary.
    tr.info("Lorentz test max div A (2)")
        .test_eq(0., amax(map([](auto && a){ return sum(a); },
                              iter<1>(transpose(DA, ra::ilist<0, 1, 2, 3, 4, 4>)))));
    tr.quiet().test_eq(0.3039588939177449, F(19, 0, 0, 0, 2, 1));

    auto show = [&tr, &delta](char const * name, int t, auto && F){
        tr.quiet().test(amin(F)>=-1);
        tr.quiet().test(amax(F)<=+1);
        println(cout, "(0)={:10.8} t={}:", F(0), (t*delta));
        for_each([](auto && F){ println(cout, "{}*", std::string(int(round(20*(clamp(F, -1., 1.)+1))), ' ')); }, F);
    };

    for (int t=0; t<o; ++t) {
        show("Ey", t, F(t, H, 0, 0, 2, 0));
        show("Bz", t, F(t, H, 0, 0, 2, 1));
        println(cout, "");
    }
    println(cout, "{:3f} μs time_A", Benchmark::toseconds(time_A)/1e-6);
    println(cout, "{:3f} μs time_DA", Benchmark::toseconds(time_DA)/1e-6);

    return tr.summary();
}
