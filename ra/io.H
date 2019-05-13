
// (c) Daniel Llorens - 2014-2018

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file io.H
/// @brief Write and read arrays, expressions.

#pragma once
#include "ra/atom.H"
#include "ra/format.H"
#include <iosfwd>
#include <sstream>

#ifndef RA_CHECK_BOUNDS_IO
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_IO 1
  #else
    #define RA_CHECK_BOUNDS_IO RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_IO==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) RA_ASSERT( cond )
#endif

namespace ra {

// After ply_index(). TODO merge with that.
// TODO custom spacers at turn of dimension.
// is_foreign_vector is included b/c std::vector or std::array may be used as the type of shape().
template <class A> inline
std::ostream & operator<<(std::ostream & o, FormatArray<A> const & fa)
{
// work with ArrayIterator. FIXME note that this copies / resets the ArrayIterator if fa.a already is one; see [ra35].
    auto a = ra::start(fa.a);
    static_assert(decltype(a)::size_s()!=DIM_BAD, "cannot print type");
    rank_t const rank = a.rank();
    auto sha(a.shape());
    auto ind = ra_traits<decltype(sha)>::make(rank, 0);
    if (withshape==fa.shape
        || (defaultshape==fa.shape && a.size_s()==DIM_ANY)) {
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
                default: std::fill_n(std::ostream_iterator<char const *>(o, ""), k-1, fa.sep2);
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
std::enable_if_t<!is_scalar<C> && ra_traits<C>::size_s()!=DIM_ANY, std::istream &>
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
std::enable_if_t<ra_traits<C>::size_s()==DIM_ANY, std::istream &>
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
#undef RA_CHECK_BOUNDS_IO
