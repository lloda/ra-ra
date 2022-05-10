// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Declarations that break namespace hygiene.

// (c) Daniel Llorens - 2016, 2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "atom.hh"

// Cf using std::abs, etc. in real.hh - functions that need to work with scalars as well as with ra objects.
using ra::odd, ra::every, ra::any;

// These global versions must be available so that e.g. ra::transpose<> may be searched by ADL even when giving explicit template args. See http://stackoverflow.com/questions/9838862 .

template <class A> inline constexpr void transpose(ra::no_arg) { abort(); }
template <int A> inline constexpr void iter(ra::no_arg) { abort(); }
