
// (c) Daniel Llorens - 2014-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/atom.H"
#include <iosfwd>
#include <sstream>

/// @file io.H
/// @brief Write and read arrays, expressions.

namespace ra {

template <class A>
struct FormatArray
{
    A const & a;
    bool const shape;
    char const * sep0;
    char const * sep1;
    char const * sep2;
};

// written this way to avoid conversions too bool shape.

template <class A, class B> inline
FormatArray<A>
format_array(A const & a, B const shape, char const * sep0=" ", char const * sep1="\n", char const * sep2="\n")
{
    static_assert(std::is_same_v<std::decay_t<B>, bool>, "bad call to format_array");
    return FormatArray<A> { a, shape, sep0, sep1, sep2 };
}

template <class A> inline
FormatArray<A> format_array(A const & a) { return format_array(a, true); }

// After ply_index(). TODO merge with that.
// TODO custom spacers at turn of dimension.
// is_foreign_vector is included b/c std::vector or std::array may be used as shape_type.
template <class A> inline
std::ostream & operator<<(std::ostream & o, FormatArray<A> const & fa)
{
// work with ArrayIterator. FIXME note that this copies / resets the ArrayIterator if fa.a already is one; see [ra35].
    auto a = ra::start(fa.a);
    static_assert(decltype(a)::size_s()!=DIM_BAD, "cannot print type");
    rank_t const rank = a.rank();
    auto sha(a.shape());
    auto ind = ra_traits<decltype(sha)>::make(rank, 0);
    if (fa.shape && a.size_s()==DIM_ANY) {
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
        assert(n>=0 && "negative sizes in input");
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
    if (typename ra_traits<C>::shape_type s; i >> s) {
        std::decay_t<C> cc(s, ra::unspecified);
        assert(every(start(s)>=0) && "negative sizes in input");
// avoid copying in case WithStorage's elements don't support it.
        swap(c, cc);
// need row-major, serial iteration here. FIXME use ra:: traversal.
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

} // namespace ra
