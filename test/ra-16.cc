// -*- mode: c++; coding: utf-8 -*-
// ra/test - Spurious out of range error with gcc11 -O3 I can't reproduce

// (c) Daniel Llorens - 2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using real = float;
using real2 = ra::Small<real, 2>;
using real3 = ra::Small<real, 3>;
using real23 = ra::Small<real, 2, 3>;
using real32 = ra::Small<real, 3, 2>;
using complex = std::complex<float>;

template <class Tu, class TR>
constexpr void
sph22c_s_op(Tu const & u, TR & R)
{
    real ct = cos(u[0]), st = sin(u[0]), cp = cos(u[1]), sp = sin(u[1]);
    R = { ct*cp, -sp, ct*sp,  cp, -st,  real(0) };
}

constexpr real2
c2s2(real3 const c)
{
    real2 s;
    real p2 = ra::sqr(c[0])+ra::sqr(c[1]);
    s[0] = atan2(sqrt(p2), c[2]);
    s[1] = atan2(c[1], c[0]);
    return s;
}

ra::Big<real, 2> rr = { { 0., 0., 1. } };
ra::Big<complex, 3> a({20, 1, 3}, 0.);
ra::Big<complex, 2> dip({20, 2}, 1.);

void
far(real const f, ra::ViewBig<real, 2> const & rr, ra::ViewBig<complex, 3> & a)
{
    real A = 1., m = .5, n = .4;
    for_each([&](auto && r, auto & a) {
        a = real(0);
        if (r[2]>0) {
            real32 op;
            sph22c_s_op(c2s2(r), op);
            real pn = A*std::pow(r[2], n);
            real23 opmn = transpose(op, ra::int_list<1, 0>{})*real2 { pn*std::pow(r[2], m-1), pn };
            gemm(dip, gemm(op, opmn), a);
        }
    },  iter<1>(rr), iter<2>(ra::transpose(a, ra::int_list<1, 0, 2>{})));
}

int main()
{
    ra::TestRecorder tr(std::cout);
    far(20e9, rr, a);
    tr.test_eq(complex(40.), sum(a));
    return tr.summary();
}
