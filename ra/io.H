// -*- mode: c++; coding: utf-8 -*-
/// @file io.H
/// @brief Write and read arrays, expressions.

// (c) Daniel Llorens - 2014-2018
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/atom.H"
#include "ra/format.H"
#include "ra/concrete.H"
#include <iosfwd>
#include <sstream>

#if defined(RA_CHECK_BOUNDS) && RA_CHECK_BOUNDS==0
  #define CHECK_BOUNDS( cond )
#else
  #define CHECK_BOUNDS( cond ) RA_ASSERT( cond, 0 )
#endif

namespace ra {

// TODO merge with ply_ravel @ ply.H, but should control order.
// TODO custom spacers at turn of dimension.
// is_foreign_vector is included b/c std::vector or std::array may be used as the type of shape().
template <class A> inline
std::ostream & operator<<(std::ostream & o, FormatArray<A> const & fa)
{
// FIXME note that this copies / resets the ArrayIterator if fa.a already is one; see [ra35].
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
// unlike in ply_index(), order here is row-major on purpose.
    for (;;) {
    next: ;
        o << a.at(ind);
        for (int k=0; k<rank; ++k) {
            if (++ind[rank-1-k]<sha[rank-1-k]) {
                switch (k) {
                case 0: o << fa.sep0; break;
                case 1: o << fa.sep1; break;
                default: std::fill_n(std::ostream_iterator<char const *>(o, ""), k, fa.sep2);
                }
                goto next;
            } else {
                ind[rank-1-k] = 0;
            }
        }
        return o;
    }
}

template <class A> inline
std::enable_if_t<is_ra<A> || is_foreign_vector<A>, std::ostream &>
operator<<(std::ostream & o, A && a)
{
    return o << format_array(a);
}

// initializer_list cannot match A && above.
template <class T> inline
std::ostream & operator<<(std::ostream & o, std::initializer_list<T> const & a)
{
    return o << format_array(a);
}

// Static size.
template <class C> inline
std::enable_if_t<!is_scalar<C> && size_s<C>()!=DIM_ANY, std::istream &>
operator>>(std::istream & i, C & c)
{
    for (auto & ci: c) { i >> ci; }
    return i;
}

// Special case for std::vector, to handle create-new / resize() difference.
template <class T, class A> inline
std::istream &
operator>>(std::istream & i, std::vector<T, A> & c)
{
    if (dim_t n; !((i >> n).fail())) {
        CHECK_BOUNDS(n>=0 && "negative sizes in input");
        c.resize(n);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

// Expr size, so read shape and possibly allocate (TODO try to avoid).
template <class C> inline
std::enable_if_t<size_s<C>()==DIM_ANY, std::istream &>
operator>>(std::istream & i, C & c)
{
    if (decltype(shape(c)) s; i >> s) {
        std::decay_t<C> cc(s, ra::none);
        CHECK_BOUNDS(every(start(s)>=0) && "negative sizes in input");
// avoid copying in case Container's elements don't support it.
        swap(c, cc);
// need row-major, serial iteration here. FIXME use ra:: traversal.
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

} // namespace ra

#undef CHECK_BOUNDS
