// -*- mode: c++; coding: utf-8 -*-
// After Chaitin1986, p. 14. Attempt at straight translation from APL.
// Newton -- Orbits

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <chrono>
#include <thread>
#include "ra/io.H"
#include "ra/operators.H"
#include "ra/format.H"

using real = double;
using real3 = ra::Small<real, 3>;
using int3 = ra::Small<int, 3>;
template <class T, int rank> using array = ra::Big<T, rank>;

int main()
{
    constexpr real G = 6.674e-11;
    real delta = 50;
    int bodies = 2;

    array<real, 1> m = { 6.e24, 10. };
    array<char, 1> c = { 'E', '*' };
    array<real, 2> x({bodies, 3}, { 0, 0, 0,  1e7, 0, 0 });
    array<real, 2> v({bodies, 3}, { 0, 0, 0,  0, 6e3, 0 });

    auto force = [&m, &x](int i, int j) -> real3
        {
            if (i==j) {
                return 0.;
            } else {
                real3 deltax = x(j)-x(i);
                return deltax*(G*m(i)*m(j) / pow(sum(deltax*deltax), 3/2.));
            }
        };

    array<char, 2> orbit({50, 50}, ' ');
    auto draw = [&orbit, &c](auto && x, auto && t)
        {
            auto mapc = [](real x) { return clamp(int(round(25+x/5e5)), 0, 49); };
// 1. TODO we still can't use iter<1> on an ET.
            array<int, 2> xi = map(mapc, x);
            at(orbit, iter<1>(xi(ra::all, ra::iota(2)))) = c;
// 2. alternative w/o temps
            for_each([&orbit, &mapc] (auto && x, auto && c) { orbit(mapc(x(0)), mapc(x(1))) = c; },
                     iter<1>(x), c);

            std::cout << "TIME IN HOURS " << (t/3600.) << " " << ra::noshape << format_array(orbit, "") << "\n";
        };

    real t = 0;
    array<real, 3> F({bodies, bodies, 3}, 0.);
    array<real, 2> a({bodies, 3}, 0.);
    for (int step=1; step<(12*15); ++step, t+=delta) {
// RHS is rank 2 so we need to match on LHS. Either of these works, but the second needs a compact last axis.
// TODO ideally [F = from(force, ra::iota(2), ra::iota(2));] would work #nestedarrays
        // for_each([](auto && x, auto && y) { x = y; }, iter<1>(F), from(force, ra::iota(2), ra::iota(2)));
        ra::explode<real3>(F) = from(force, ra::iota(bodies), ra::iota(bodies));
// match axes a[0, 1] with F[0, 2]; accumulate on F[1]. TODO proper reductions.
        a = 0.;
        a += transpose<0, 2, 1>(F) / m;
        v += a*delta;
        x += v*delta;
        draw(x, t);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return 0;
}
