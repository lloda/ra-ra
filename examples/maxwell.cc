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
template <class T, int rank> using array = ra::Big<T, rank>;
auto H = ra::all;
template <int n> constexpr ra::dots_t<n> HH = ra::dots<n>;
constexpr auto PI = std::numbers::pi_v<double>;

using std::cout, std::endl;
using ra::iota, ra::int_c, ra::TestRecorder, ra::Benchmark;

int main()
{
    TestRecorder tr(cout);

    real delta = 1;
    int o=20, n=20, m=2, l=2;
    array<real, 5> A({o, n, m, l, 4}, 0.);
    array<real, 6> DA({o, n, m, l, 4, 4}, 0.);
    array<real, 6> F({o, n, m, l, 4, 4}, 0.);
    array<real, 4> divA({o, n, m, l}, 0.);
    array<real, 4> X({n, m, l, 4}, 0.), Y({n, m, l, 4}, 0.);

    A(0, H, H, H, 2) = -cos(iota(n)*(2*PI/n))/(2*PI/n);
    A(1, H, H, H, 2) = -cos((iota(n)-delta)*(2*PI/n))/(2*PI/n);

    auto t0 = Benchmark::clock::now();
// FIXME this is painful without a roll operator, but we need a roll operator without temps.
    for (int t=1; t+1<o; ++t) {
// X←(1⌽[0]A[T;;;;])+(1⌽[1]A[T;;;;])+(1⌽[2]A[T;;;;])
        X(iota(n-1)) = A(t, iota(n-1, 1));
        X(n-1) = A(t, 0);
        X(H, iota(m-1)) += A(t, H, iota(m-1, 1));
        X(H, m-1) += A(t, H, 0);
        X(H, H, iota(l-1)) += A(t, H, H, iota(l-1, 1));
        X(H, H, l-1) += A(t, H, H, 0);
// Y←(¯1⌽[0]A[T;;;;])+(¯1⌽[1]A[T;;;;])+(¯1⌽[2]A[T;;;;])
        Y(iota(n-1, 1)) = A(t, iota(n-1));
        Y(0) = A(t, n-1);
        Y(H, iota(m-1, 1)) += A(t, H, iota(m-1));
        Y(H, 0) += A(t, H, m-1);
        Y(H, H, iota(l-1, 1)) += A(t, H, H, iota(l-1));
        Y(H, H, 0) += A(t, H, H, l-1);

        A(t+1) = X + Y - A(t-1) - 4*A(t);
    }
    auto time_A = Benchmark::clock::now()-t0;

// FIXME should try to traverse the array once, e.g. explode() = pack(...), but we need to wrap around boundaries.
    auto diff = [&DA, &A, &delta](auto k_, real factor)
        {
            constexpr int k = k_;
            const int o = DA.len(k);
            if (o>=2) {
                DA(HH<k>, iota(o-2, 1), HH<4-k>, k) = (A(HH<k>, iota(o-2, 2)) - A(HH<k>, iota(o-2, 0)));
                DA(HH<k>, 0, HH<4-k>, k) = (A(HH<k>, 1) - A(HH<k>, o-1));
                DA(HH<k>, o-1, HH<4-k>, k) = (A(HH<k>, 0) - A(HH<k>, o-2));
                DA(HH<5>, k) *= factor;
            }
        };

    t0 = Benchmark::clock::now();
    diff(int_c<0>(), +1/(2*delta));
    diff(int_c<1>(), -1/(2*delta));
    diff(int_c<2>(), -1/(2*delta));
    diff(int_c<3>(), -1/(2*delta));
    auto time_DA = Benchmark::clock::now()-t0;

    F = ra::transpose<0, 1, 2, 3, 5, 4>(DA) - DA;

// abuse shape matching to reduce last axis.
    divA += ra::transpose<0, 1, 2, 3, 4, 4>(DA);
    tr.info("Lorentz test max div A (1)").test_eq(0., amax(divA));
// an alternative without a temporary.
    tr.info("Lorentz test max div A (2)")
        .test_eq(0., amax(map([](auto && a) { return sum(a); },
                              iter<1>(ra::transpose<0, 1, 2, 3, 4, 4>(DA)))));
    tr.quiet().test_eq(0.3039588939177449, F(19, 0, 0, 0, 2, 1));

    auto show = [&tr, &delta](char const * name, int t, auto && F)
        {
            tr.quiet().test(amin(F)>=-1);
            tr.quiet().test(amax(F)<=+1);
            cout << name << "(0)=" << std::setprecision(10) << std::setw(12) << F(0) << " t=" << (t*delta) << ":\n";
            for_each([](auto && F) { cout << std::string(int(round(20*(clamp(F, -1., 1.)+1))), ' ') << "*\n"; }, F);
        };

    for (int t=0; t<o; ++t) {
        show("Ey", t, F(t, H, 0, 0, 2, 0));
        show("Bz", t, F(t, H, 0, 0, 2, 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cout << endl;
    }
    cout << Benchmark::toseconds(time_A)/1e-6 << " μs time_A" << endl;
    cout << Benchmark::toseconds(time_DA)/1e-6 << " μs time_DA" << endl;

    return tr.summary();
}
