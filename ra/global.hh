// -*- mode: c++; coding: utf-8 -*-
/// @file global.hh
/// @brief Had to break namespace hygiene.

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/wedge.hh"
#include "ra/atom.hh"

// Cf using std::abs, etc. in real.hh.
using ra::odd, ra::every, ra::any;

// These global versions must be available so that e.g. ra::transpose<> may be searched by ADL even when giving explicit template args. See http://stackoverflow.com/questions/9838862 .

template <class A>
requires (ra::is_scalar<A>)
inline constexpr decltype(auto) transpose(A && a) { return std::forward<A>(a); }

// We also define the scalar specializations that couldn't be found through ADL in any case. See complex.hh, real.hh.
// These seem unnecessary, just as I do ra::concrete() I could do ra::where(). FIXME

template <int cell_rank>
inline constexpr void iter() { abort(); }
