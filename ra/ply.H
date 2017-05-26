
// (c) Daniel Llorens - 2013-2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file ply.H
/// @brief Traverse (ply) array or array expression or array statement.

// TODO Lots of room for improvement: small (fixed sizes) and large (tiling, etc. see eval.cc in Blitz++).

#pragma once
#include "ra/type.H"
#include <functional>

namespace ra {

static_assert(std::is_signed<rank_t>::value && std::is_signed<dim_t>::value, "bad rank_t");

// --------------
// Run time order, two versions.
// --------------

// TODO See ply_ravel() for traversal order.
// TODO A(i0, i1 ...) can be partial-applied as A(i0)(i1 ...) for faster indexing
// TODO Traversal order should be a parameter, since some operations (e.g. output, ravel) require a specific order.
template <class A> inline
void ply_index(A && a)
{
    rank_t const rank = a.rank();
    auto ind(ra_traits<decltype(a.shape())>::make(rank, 0));
    dim_t sha[rank];
    rank_t order[rank];
    for (rank_t k=0; k<rank; ++k) {
        order[k] = rank-1-k;
        sha[k] = a.size(order[k]);
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

// Traverse array expression looking to ravel the inner loop.
// size() is only used on the driving argument (largest rank).
// adv(), stride(), keep_stride() and flat() are used on all the leaf arguments. The strides must give 0 for k>=their own rank, to allow frame matching.
// TODO Traversal order should be a parameter, since some operations (e.g. output, ravel) require a specific order.
template <class A> inline
void ply_ravel(A && a)
{
    static_assert(!has_tensorindex<A>, "bad plier for expr");

    rank_t rank = a.rank();
    rank_t order[rank];
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
    switch (rank) {
    case 0: *(a.flat()); return;
    case 1: break;
    default: // TODO find a decent heuristic
        // if (rank>1) {
        //     std::sort(order, order+rank, [&a, &order](auto && i, auto && j)
        //               { return a.size(order[i])<a.size(order[j]); });
        // }
        ;
    }
// find outermost compact dim.
    rank_t * ocd = order;
    auto ss = a.size(*ocd);
    for (--rank, ++ocd; rank>0 && a.keep_stride(ss, order[0], *ocd); --rank, ++ocd) {
        ss *= a.size(*ocd);
    }
    dim_t ind[rank], sha[rank];
    for (int k=0; k<rank; ++k) {
        ind[k] = 0;
        sha[k] = a.size(ocd[k]);
        if (sha[k]==0) { // for the ravelled dimensions ss takes care.
            return;
        }
    }
// all sub xpr strides advance in compact dims, as they might be different.
    auto const ss0 = a.stride(order[0]);
// TODO Blitz++ uses explicit stack of end-of-dim p positions, has special cases for common/unit stride.
    for (;;) {
        dim_t s = ss;
        for (auto p=a.flat(); s>0; --s, p+=ss0) {
            *p;
        }
        for (int k=0; ; ++k) {
            if (k>=rank) {
                return;
            } else if (ind[k]<sha[k]-1) {
                ++ind[k];
                a.adv(ocd[k], 1);
                break;
            } else {
                ind[k] = 0;
                a.adv(ocd[k], 1-sha[k]);
            }
        }
    }
}

// -------------------------
// Compile time order. See bench-dot.C for use. Index version.
// -------------------------

template <class order, class A, class S>
std::enable_if_t<mp::len<order> == 0> inline
subindexf(A & a, S & i)
{
    a.at(i);
}

template <class order, class A, class S>
std::enable_if_t<(mp::len<order> > 0)> inline
subindexf(A & a, S & i_)
{
    dim_t & i = i_[mp::First_<order>::value];
// on every subloop, but not worth caching
    dim_t const /* constexpr */ s = a.size(mp::First_<order>::value);
    for (i=0; i<s; ++i) {
        subindexf<mp::Drop1_<order>>(a, i_);
    }
}

template <class A> inline
void plyf_index(A && a)
{
    using Shape = std::decay_t<decltype(a.shape())>;
    Shape i(ra_traits<Shape>::make(a.rank(), 0));
    subindexf<mp::Iota_<std::decay_t<A>::rank_s()>>(a, i); // cf with ply_index() for C order.
}

// -------------------------
// Compile time order. See bench-dot.C for use. No index version.
// With compile-time recursion by rank, one can use adv<k>, but order must also be compile-time.
// -------------------------

template <class order, int ravel_rank, class A, class S> inline
std::enable_if_t<mp::len<order> == ravel_rank>
subindex(A & a, dim_t s, S const & ss0)
{
    for (auto p=a.flat(); s>0; --s, p+=ss0) {
        *p;
    }
}

template <class order, int ravel_rank, class A, class S> inline
std::enable_if_t<(mp::len<order> > ravel_rank)>
subindex(A & a, dim_t const s, S const & ss0)
{
    dim_t size = a.size(mp::First_<order>::value); // TODO Precompute these at the top
    for (dim_t i=0, iend=size; i<iend; ++i) {
        subindex<mp::Drop1_<order>, ravel_rank>(a, s, ss0);
        a.adv(mp::First_<order>::value, 1);
    }
    a.adv(mp::First_<order>::value, -size);
}

// until() converts runtime jj into compile time j. TODO a.adv<k>().

template <class order, int j, class A, class S> inline
std::enable_if_t<(mp::len<order> < j)>
until(int const jj, A & a, dim_t const s, S const & ss0)
{
    assert(0 && "rank too high");
}

template <class order, int j, class A, class S> inline
std::enable_if_t<(mp::len<order> >= j)>
until(int const jj, A & a, dim_t const s, S const & ss0)
{
    if (jj==j) {
        subindex<order, j>(a, s, ss0);
    } else {
        until<order, j+1>(jj, a, s, ss0);
    }
}

template <class A> inline
auto plyf(A && a) -> std::enable_if_t<(std::decay_t<A>::rank_s()<=0)>
{
    static_assert(!has_tensorindex<A>, "bad plier for expr");
    static_assert(std::decay_t<A>::rank_s()==0, "plyf needs static rank");
    *(a.flat());
}

template <class A> inline
auto plyf(A && a) -> std::enable_if_t<(std::decay_t<A>::rank_s()==1)>
{
    static_assert(!has_tensorindex<A>, "bad plier for expr");
    subindex<mp::Iota_<1>, 1>(a, a.size(0), a.stride(0));
}

template <class A> inline
auto plyf(A && a) -> std::enable_if_t<(std::decay_t<A>::rank_s()>1)>
{
    static_assert(!has_tensorindex<A>, "bad plier for expr");
    /* constexpr */ rank_t const rank = a.rank();
#if 0
// find the outermost compact dim.
    /* constexpr */ auto s = a.size(rank-1);
    int j = 1;
    while (j<rank && a.keep_stride(s, rank-1, rank-1-j)) {
        s *= a.size(rank-1-j);
        ++j;
    }
// all sub xpr strides advance in compact dims, as they might be different.
// send with static j. Note that order here is inverse of order.
    until<mp::Iota_<std::decay_t<A>::rank_s()>, 0>(j, a, s, a.stride(rank-1));
#else
// according to bench-dot.C, the unrolling above isn't worth it :-/ TODO
    /* constexpr */ auto s = a.size(rank-1);
    subindex<mp::Iota_<std::decay_t<A>::rank_s()>, 1>(a, s, a.stride(rank-1));
#endif
}

// ---------------------------
// Select best performance (or requirements) for each type.
// ---------------------------

template <class A> inline
std::enable_if_t<has_tensorindex<A>>
ply(A && a)
{
    ply_index(std::forward<A>(a));
}

template <class A> inline
std::enable_if_t<!has_tensorindex<A> && (std::decay_t<A>::size_s()==DIM_ANY)>
ply(A && a)
{
    ply_ravel(std::forward<A>(a));
}

template <class A> inline
std::enable_if_t<!has_tensorindex<A> && (std::decay_t<A>::size_s()!=DIM_ANY)>
ply(A && a)
{
    plyf(std::forward<A>(a));
}

// ---------------------------
// Short-circuiting pliers. TODO These are reductions. How to do higher rank?
// ---------------------------

// BUG options for ply should be the same as for non-short circuit.
template <class A, class DEF>
auto ply_index_exit(A && a, DEF && def)
{
    rank_t const rank = a.rank();
    auto ind(ra_traits<decltype(a.shape())>::make(rank, 0));
    dim_t sha[rank];
    rank_t order[rank];
    for (rank_t k=0; k<rank; ++k) {
        order[k] = rank-1-k;
        sha[k] = a.size(order[k]);
        if (sha[k]==0) {
            return def;
        }
    }
    for (;;) {
        auto what = a.at(ind);
        if (std::get<0>(what)) {
            return std::get<1>(what);
        }
        for (int k=0; ; ++k) {
            if (k==rank) {
                return def;
            } else if (++ind[order[k]]<sha[k]) {
                break;
            } else {
                ind[order[k]] = 0;
            }
        }
    }
}

template <class A, class DEF>
auto ply_index_exit_1(A && a, DEF && def)
{
    auto s = a.size(0);
    auto ss0 = a.stride(0);
    for (auto p=a.flat(); s>0; --s, p+=ss0) {
        auto what = *p;
        if (std::get<0>(what)) {
            return std::get<1>(what);
        }
    }
    return def;
}

template <class A, class DEF, std::enable_if_t<is_iterator<A> && (std::decay_t<A>::rank_s()!=1 || has_tensorindex<A>), int> = 0>
inline decltype(auto)
ply_exit(A && a, DEF && def)
{
    return ply_index_exit(std::forward<A>(a), std::forward<DEF>(def));
}
template <class A, class DEF, std::enable_if_t<is_iterator<A> && (std::decay_t<A>::rank_s()==1 && !has_tensorindex<A>), int> = 0>
inline decltype(auto)
ply_exit(A && a, DEF && def)
{
    return ply_index_exit_1(std::forward<A>(a), std::forward<DEF>(def));
}

template <class A, class DEF> inline decltype(auto)
early(A && a, DEF && def)
{
    return ply_exit(std::forward<A>(a), std::forward<DEF>(def));
}

} // namespace ra
