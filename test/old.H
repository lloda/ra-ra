// -*- mode: c++; coding: utf-8 -*-
/// @file old.H
/// @brief Bits and pieces that aren't used but that I still want to test.

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/ply.H"

namespace ra {

// if by_index<T> is true, then T must be traversed by at() (as in ply_index) and not by flat() (as in ply_ravel).
// OldTensorIndex is a prototype of such a T.
template <class X> constexpr bool by_index_def = false;
template <class T> constexpr bool by_index = by_index_def<std::decay_t<T>>;

template <class Op, class ... Ti, class K>
constexpr bool by_index_def<Expr<Op, std::tuple<Ti ...>, K>> = (by_index<Ti> || ...);
template <class ... Ti, class K>
constexpr bool by_index_def<Pick<std::tuple<Ti ...>, K>> = (by_index<Ti> || ...);
template <class Live, class A>
constexpr bool by_index_def<Reframe<Live, A>> = by_index<A>;

// value_type may be needed to avoid conversion issues.
template <int w_, class value_type=ra::dim_t>
struct OldTensorIndex
{
    static_assert(w_>=0, "bad OldTensorIndex");
    constexpr static int w = w_;
    constexpr static dim_t len(int k) { return DIM_BAD; } // used in shape checks with dyn. rank.
    constexpr static dim_t size_s() { return DIM_BAD; }
    constexpr static rank_t rank_s() { return w+1; }
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k<=w); return DIM_BAD; }

    template <class I> constexpr static value_type at(I const & i) { return value_type(i[w]); }
    constexpr static dim_t step(int k) { assert(0); return 0; } // used by Expr::step_t.
    constexpr static value_type * flat() { assert(0); return nullptr; } // used by type signatures.
};

template <int w, class value_type> constexpr bool by_index_def<OldTensorIndex<w, value_type>> = true;


// --------------
// Alternative to ply_ravel for Iterators that have at() instead of flat()/etc (e.g. OldTensorIndex).
// --------------

// This doesn't unravel and carries the indices whether they are used or not, so ply_ravel should generally be faster.
// It was used as alternate to ply_ravel before TensorIndex replaced OldTensorIndex.
// TODO See ply_ravel() for traversal order.
// TODO A(i0, i1 ...) could be partial-applied as A(i0)(i1 ...) for faster indexing
// TODO Traversal order should be a parameter, since some operations (e.g. output, ravel) require a specific order.
template <class A> inline
void ply_index(A && a)
{
    rank_t const rank = a.rank();
    auto ind = with_same_shape(ra::shape(a), 0);
    dim_t sha[rank];
    rank_t order[rank];
    for (rank_t k=0; k<rank; ++k) {
        order[k] = rank-1-k;
        sha[k] = a.len(order[k]);
        if (sha[k]==0) {
            return;
        }
    }
    for (;;) {
        a.at(ind);
        for (int k=0; ; ++k) {
            if (k==rank) {
                return;
            } else if (++ind[order[k]]<sha[k]) {
                break;
            } else {
                ind[order[k]] = 0;
            }
        }
    }
}


// -------------------------
// Compile time order; alt. to plyf for by_index. See bench-dot.C for use. Index version.
// -------------------------

template <class order, class A, class S>
inline void
subindexf(A & a, S & i_)
{
    if constexpr (mp::len<order> == 0) {
        a.at(i_);
    } else if constexpr (mp::len<order> > 0) {
        dim_t & i = i_[mp::first<order>::value];
// on every subloop, but not worth caching
        dim_t const /* constexpr */ s = a.len(mp::first<order>::value);
        for (i=0; i<s; ++i) {
            subindexf<mp::drop1<order>>(a, i_);
        }
    } else {
        abort();
    }
}

template <class A> inline
void plyf_index(A && a)
{
    auto i = with_same_shape(ra::shape(a), 0);
    subindexf<mp::iota<std::decay_t<A>::rank_s()>>(a, i); // cf with ply_index() for C order.
}

} // namespace ra
