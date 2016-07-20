
// (c) Daniel Llorens - 2014-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_IO_H
#define RA_IO_H

#include "ra/ra-type.H"
#include "ra/ra-traits.H"

/// @file ra-io.H
/// @brief Write and read arrays, expressions.

namespace ra {

// a version of ply_index(). @TODO Eventually try to merge with that.
// is_foreign_vector is included b/c std::vector or std::array may be used as shape_type.
template <class A> inline
std::enable_if_t<is_ra<A> || is_foreign_vector<A>, std::ostream &>
operator<<(std::ostream & o, A && a_)
{
// work with ArrayIterator
    auto a = ra::start(a_);
    static_assert(decltype(a)::size_s()!=DIM_BAD, "cannot print type");
    rank_t const rank = a.rank();
    auto sha(a.shape());
    if (a.size_s()==DIM_ANY) {
        o << start(sha) << '\n';
    }
    if (a.done()) {
        return o;
    }
    auto ind = ra_traits<decltype(sha)>::make(rank, 0);
// unlike in ply_index(), order here is row-major on purpose.
    for (;;) {
    next: ;
        o << a.at(ind) << " ";
        for (int k=0; k<rank; ++k) {
            if (++ind[rank-1-k]<sha[rank-1-k]) {
                std::fill_n(std::ostream_iterator<char>(o, ""), k, '\n');
                goto next;
            } else {
                ind[rank-1-k] = 0;
            }
        }
        return o;
    }
}

// Static size.
template <class C> inline
std::enable_if_t<ra_traits<C>::size_s()!=DIM_ANY, std::istream &>
operator>>(std::istream & i, C & c)
{
    using T = typename ra_traits<C>::value_type;
    std::copy_n(std::istream_iterator<T>(i), c.size(), c.begin());
    return i;
}

// Special case for std::vector, to handle create-new / resize() difference.
template <class T, class A> inline
std::istream &
operator>>(std::istream & i, std::vector<T, A> & c)
{
    dim_t n;
    i >> n;
    assert(n>=0);
    if (i) {
        c.resize(n);
        std::copy_n(std::istream_iterator<T>(i), c.size(), c.begin());
    }
    return i;
}

// Expr size, so read shape and possibly allocate (@TODO try to avoid).
template <class C> inline
std::enable_if_t<ra_traits<C>::size_s()==DIM_ANY, std::istream &>
operator>>(std::istream & i, C & c)
{
    using T = typename ra_traits<C>::value_type;
    typename ra_traits<C>::shape_type s;
    i >> s;
    if (i) {
        c = std::decay_t<C>(s, ra::unspecified);
        if (c.size()>0) { // I'd say a bug in copy_n; see failing case in test-ra-nested.C.
            std::copy_n(std::istream_iterator<T>(i), c.size(), c.begin());
        }
    }
    return i;
}

} // namespace ra

#endif // RA_IO_H
