// -*- mode: c++; coding: utf-8 -*-
/// @file foreign.cc
/// @brief Regression for value_t, rank_s

// (c) Daniel Llorens - 2020
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

template <class C>
struct bsphere
{
    C c;
};

template <class C>
double above(C const & p, bsphere<C> const & o)
{
    return 0;
}

template <class B, class C>
requires (0!=ra::rank_s<B>() && std::is_same_v<C, ra::value_t<B>>)
double
above(C const & p, B const & o)
{
    return 1;
}

int main()
{
    TestRecorder tr(cout);

    using P = ra::Small<double, 2>;
    tr.test_eq(0, above(P {1, 1}, bsphere<P> {{1, 2}}));
    tr.test_eq(1, above(P {1, 1}, ra::Small<P, 2> {{1, 2}, {3, 4}}));

    return tr.summary();
}
