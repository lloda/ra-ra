
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file cghs.C
/// @brief Conjugate gradient after Hestenes und Stiefel
// Adapted from lsolver/cghs.C by Christian Badura, 1998/05.

#pragma once
#include "ra/operators.H"

template <class A, class B, class X, class W>
inline int
cghs(A & a, B const & b, X & x, W & work, double eps)
{
    work = 0.;
    auto g = work(0);
    auto r = work(1);
    auto p = work(2);

    double err = sqr(eps)*reduce_sqrm(b);

    mult(a, x, g);
    g = b-g;
    r = g;

    int i;
    for (i=0; reduce_sqrm(g)>err; ++i) {
        mult(a, r, p);
        double rho = reduce_sqrm(p);
        double sig = dot(r, p);
        double tau = dot(g, r);
        double t = tau/sig;
        double gam = (sqr(t)*rho-tau)/tau;
        x += t*r;
        g -= t*p;
        r = r*gam + g;
    }
    return i;
}
