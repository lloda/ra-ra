// -*- mode: c++; coding: utf-8 -*-
/// @file global.H
/// @brief Had to break namespace hygiene.

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/wedge.H"
#include "ra/atom.H"

// TODO All users be int, then this take int.
// See also using std::abs, etc. in real.H.
using ra::odd, ra::every, ra::any;

// These global versions must be available so that e.g. ra::transpose<> may be searched by ADL even when giving explicit template args. See http://stackoverflow.com/questions/9838862 .

template <class A,
          std::enable_if_t<ra::is_scalar<A>, int> =0>
inline constexpr decltype(auto) transpose(A && a) { return std::forward<A>(a); }

// We also define the scalar specializations that couldn't be found through ADL in any case. See complex.H, real.H.
// These seem unnecessary, just as I do ra::concrete() I could do ra::where(). FIXME

template <int cell_rank>
inline constexpr void iter() { abort(); }

// These ra::start are needed b/c rank 0 converts to and from scalar, so ? can't pick the right (-> scalar) conversion.
template <class T, class F,
          std::enable_if_t<ra::is_zero_or_scalar<T> && ra::is_zero_or_scalar<F>, int> =0>
inline constexpr decltype(auto) where(bool const w, T && t, F && f)
{
    return w ? *(ra::start(t).flat()) : *(ra::start(f).flat());
}

template <int D, int Oa, int Ob, class A, class B,
          std::enable_if_t<ra::is_scalar<A> && ra::is_scalar<B>, int> =0>
inline constexpr auto wedge(A const & a, B const & b) { return a*b; }
