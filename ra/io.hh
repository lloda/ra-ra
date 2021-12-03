// -*- mode: c++; coding: utf-8 -*-
/// @file io.hh
/// @brief Write and read arrays, expressions.

// (c) Daniel Llorens - 2014-2018, 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "atom.hh"
#include "format.hh"
#include "concrete.hh"
#include <iosfwd>
#include <sstream>

namespace ra {

// TODO merge with ply_ravel @ ply.hh. But should control order.
template <class A>
inline std::ostream &
operator<<(std::ostream & o, FormatArray<A> const & fa)
{
// FIXME note that this copies / resets the RaIterator if fa.a already is one; see [ra35].
    auto a = ra::start(fa.a);
    static_assert(size_s(a)!=DIM_BAD, "cannot print type");
    rank_t const rank = a.rank();
    auto sha = concrete(shape(a));
    auto ind = with_same_shape(sha, 0);
    if (withshape==fa.shape || (defaultshape==fa.shape && size_s(a)==DIM_ANY)) {
        o << start(sha) << '\n';
    }
    for (rank_t k=0; k<rank; ++k) {
        if (sha[k]==0) {
            return o;
        }
    }
// order here is row-major on purpose.
    for (;;) {
        o << *(a.flat());
        for (int k=0; ; ++k) {
            if (k>=rank) {
                return o;
            } else if (ind[rank-1-k]<sha[rank-1-k]-1) {
                ++ind[rank-1-k];
                a.adv(rank-1-k, 1);
                switch (k) {
                case 0: o << fa.sep0; break;
                case 1: o << fa.sep1; break;
                default: std::fill_n(std::ostream_iterator<char const *>(o, ""), k, fa.sep2);
                }
                break;
            } else {
                ind[rank-1-k] = 0;
                a.adv(rank-1-k, 1-sha[rank-1-k]);
            }
        }
    }
}

// is_foreign_vector is included b/c std::vector or std::array may be used as the type of shape().
template <class A> requires (is_ra<A> || is_foreign_vector<A>)
inline std::ostream &
operator<<(std::ostream & o, A && a)
{
    return o << format_array(a);
}

// initializer_list cannot match A && above.
template <class T>
inline std::ostream &
operator<<(std::ostream & o, std::initializer_list<T> const & a)
{
    return o << format_array(a);
}

// Static size.
template <class C> requires (!is_scalar<C> && size_s<C>()!=DIM_ANY)
inline std::istream &
operator>>(std::istream & i, C & c)
{
    for (auto & ci: c) { i >> ci; }
    return i;
}

// Special case for std::vector, to handle create-new / resize() difference.
template <class T, class A>
inline std::istream &
operator>>(std::istream & i, std::vector<T, A> & c)
{
    if (dim_t n; !((i >> n).fail())) {
        RA_CHECK(n>=0 && "negative sizes in input");
        c.resize(n);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

// Expr size, so read shape and possibly allocate (TODO try to avoid).
template <class C> requires (size_s<C>()==DIM_ANY)
inline std::istream &
operator>>(std::istream & i, C & c)
{
    if (decltype(shape(c)) s; i >> s) {
        std::decay_t<C> cc(s, ra::none);
        RA_CHECK(every(start(s)>=0), "negative sizes in input ", s);
// avoid copying in case Container's elements don't support it.
        swap(c, cc);
// need row-major, serial iteration here. FIXME use ra:: traversal.
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

} // namespace ra
