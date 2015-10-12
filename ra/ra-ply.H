
// (c) Daniel Llorens - 2013-2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_PLY_H
#define RA_PLY_H

/// @file ra-ply.H
/// @brief Traverse (ply) array or array expression or array statement.

// @TODO Lots of room for improvement: small (fixed sizes) and large (tiling, etc. see eval.cc in Blitz++).

#include "ra/ra-type.H"

namespace ra {

// @TODO this to protect against older convention in vtraits; eventually remove.
static_assert(mp::And<std::is_signed<rank_t>, std::is_signed<dim_t>>::value, "bad rank_t");

// --------------
// Run time order, two versions.
// --------------

// @TODO See ply_ravel() for traversal order.
// @TODO A(i0, i1 ...) can be partial-applied as A(i0)(i1 ...) for faster indexing
// @TODO Traversal order should be a parameter, since some operations (e.g. output, ravel) require a specific order.
template <class A>
void ply_index(A && a)
{
// @TODO try and merge the singular cases.
    if (a.done()) {
        return;
    }
    rank_t const rank = a.rank();
    auto sha(a.shape());
    using Shape = decltype(sha);
    Shape ind(ra_traits<Shape>::make(rank, 0));

    rank_t order[rank];
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
    for (;;) {
        a.at(ind);
        for (int k=0; ; ++k) {
            if (k==rank) {
                return;
            } else if (++ind[order[k]]<sha[order[k]]) {
                break;
            } else {
                ind[order[k]] = 0;
            }
        }
    }
}

// Traverse array expression looking for compact inner loop.
// size() and preferred_stride() are only used on the driving argument (largest rank).
// adv(), stride(), compact_stride() and flat() are used on all the leaf arguments. The strides must give 0 for k>=their own rank, to allow frame matching.
// @TODO Traversal order should be a parameter, since some operations (e.g. output, ravel) require a specific order.
template <class A>
void ply_ravel(A && a)
{
// flat()+step would not see the zero sizes above the axes used for step, so we must check for empty expr anyway.
    if (a.done()) {
        return;
    }
    rank_t const rank = a.rank();
    if (rank==0) {
        *(a.flat());
        return;
    }
    rank_t order[rank];
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
// find outermost compact dim.
    auto step = a.size(order[0]);
    int j = 1;
    for (; j!=rank && a.compact_stride(step, order[0], order[j]); ++j) {
        step *= a.size(order[j]);
    }
    step *= a.preferred_stride(order[0]);
// all sub xpr strides advance in compact dims, as they might be different.
    auto const ss0(a.stride(order[0]));
// @TODO don't the full sha or ind. But try to use them to do ply_index more efficiently.
    auto sha(a.shape());
    using Shape = decltype(sha);
    Shape ind(ra_traits<Shape>::make(rank, 0));
// @TODO Blitz++ uses explicit stack of end-of-dim p positions, has special cases for common/unit stride.
    for (;;) {
        auto p = a.flat();
        for (auto end=p+step; p!=end; p+=ss0) {
            *p;
        }
        for (int k=j; ; ++k) {
            if (k>=rank) {
                return;
            } else if (ind[order[k]]<sha[order[k]]-1) {
                ++ind[order[k]];
                a.adv(order[k], 1);
                break;
            } else {
                ind[order[k]] = 0;
                a.adv(order[k], 1-sha[order[k]]);
            }
        }
    }
}

// -------------------------
// Compile time order. See bench-array-dot-ra.C for use. Index version.
// -------------------------

template <class order, class A, class S>
enableifc_<mp::Len<order>::value==0>
subindexf(A & a, S & s, S & i)
{
    a.at(i);
}

template <class order, class A, class S>
enableifc_<(mp::Len<order>::value>0)>
subindexf(A & a, S & s_, S & i_)
{
    dim_t & i = i_[mp::First_<order>::value];
    dim_t const s = s_[mp::First_<order>::value];
    for (i=0; i!=s; ++i) {
        subindexf<mp::Drop1_<order>>(a, s_, i_);
    }
}

template <class A>
void plyf_index(A && a)
{
    auto s(a.shape());
    using Shape = decltype(s);
    Shape i(ra_traits<Shape>::make(s.size(), 0));
    subindexf<mp::Iota_<A::rank_s()>>(a, s, i); // cf with ply_index() for C order.
}

// -------------------------
// Compile time order. See bench-array-dot-ra.C for use. No index version.
// With compile-time recursion by rank, one can use adv<k>, but order must also be compile-time.
// -------------------------

template <class order, int ravel_rank, class A, class S>
enableifc_<mp::Len<order>::value==ravel_rank>
subindex(A & a, dim_t const s, S const & ss0)
{
    auto p = a.flat();
    for (auto end=p+s; p!=end; p+=ss0) {
        *p;
    }
}

template <class order, int ravel_rank, class A, class S>
enableifc_<(mp::Len<order>::value>ravel_rank)>
subindex(A & a, dim_t const s, S const & ss0)
{
    dim_t size = a.size(mp::First_<order>::value); // @TODO Precompute these at the top
    for (dim_t i=0, iend=size; i<iend; ++i) {
        subindex<mp::Drop1_<order>, ravel_rank>(a, s, ss0);
        a.adv(mp::First_<order>::value, 1);
    }
    a.adv(mp::First_<order>::value, -size);
}

// until() converts runtime jj into compile time j. @TODO a.adv<k>().

template <class order, int j, class A, class S>
enableifc_<(mp::Len<order>::value<j)>
until(int const jj, A & a, dim_t const s, S const & ss0)
{
    assert(0 && "rank too high");
}

template <class order, int j, class A, class S>
enableifc_<(mp::Len<order>::value>=j)>
until(int const jj, A & a, dim_t const s, S const & ss0)
{
    if (jj==j) {
        subindex<order, j>(a, s, ss0);
    } else {
        until<order, j+1>(jj, a, s, ss0);
    }
}

template <class A>
auto plyf(A && a) -> enableifc_<(A::rank_s()<=0)>
{
    static_assert(A::rank_s()==0, "plyf needs static rank");
    *(a.flat());
}

template <class A>
auto plyf(A && a) -> enableifc_<(A::rank_s()==1)>
{
    subindex<mp::Iota_<1>, 1>(a, a.size(0)*a.preferred_stride(0), a.stride(0));
}

template <class A>
auto plyf(A && a) -> enableifc_<(A::rank_s()>1)>
{
    rank_t const rank = a.rank();
// find the outermost compact dim.
    auto step = a.size(rank-1);
    int j = 1;
    while (j!=rank && a.compact_stride(step, rank-1, rank-1-j)) {
        step *= a.size(rank-1-j);
        ++j;
    }
    step *= a.preferred_stride(rank-1);
// all sub xpr strides advance in compact dims, as they might be different.
// send with static j. Note that order here is inverse of order.
    until<mp::Iota_<A::rank_s()>, 0>(j, a, step, a.stride(rank-1));
}

// ---------------------------
// Selectors, best performance for each type.
// ---------------------------

template <class A>
enableif_<has_tensorindex<std::decay_t<A>>>
ply_either(A && a)
{
    ply_index(std::forward<A>(a));
}

template <class A>
enableifc_<!has_tensorindex<std::decay_t<A>>::value && (A::size_s()==DIM_ANY || (A::rank_s()!=0 && A::rank_s()!=1))>
ply_either(A && a)
{
    ply_ravel(std::forward<A>(a));
}

template <class A>
enableifc_<!has_tensorindex<std::decay_t<A>>::value && (A::size_s()!=DIM_ANY && (A::rank_s()==0 || A::rank_s()==1))>
ply_either(A && a)
{
    plyf(std::forward<A>(a));
}

// ---------------------------
// Short-circuiting pliers. @TODO These are reductions. How to do higher rank?
// ---------------------------

// @BUG Slow. Options for ply should be the same as for non-short circuit.
#define DEF_SHORT_CIRCUIT_PLY(NAME, OP)                                 \
template <class A, enableifc_<A::rank_s()!=1 || has_tensorindex<std::decay_t<A>>::value, int> = 0> \
bool JOIN(ply_, NAME)(A && a)                                           \
{                                                                       \
    /* @TODO try and merge the singular cases. */                       \
    if (a.done()) {                                                     \
        return OP false;                                                \
    }                                                                   \
    rank_t const rank = a.rank();                                       \
    auto s(a.shape());                                                  \
    using Shape = decltype(s);                                          \
    Shape i(ra_traits<Shape>::make(rank, 0));                           \
                                                                        \
    rank_t order[rank];                                                 \
    for (rank_t i=0; i<rank; ++i) {                                     \
        order[i] = rank-1-i;                                            \
    }                                                                   \
    for (;;) {                                                          \
        if (OP a.at(i)) {                                               \
            return OP true;                                             \
        }                                                               \
        for (int k=0; ; ++k) {                                          \
            if (k==rank) {                                              \
                return OP false;                                        \
            } else if (++i[order[k]]<s[order[k]]) {                     \
                break;                                                  \
            } else {                                                    \
                i[order[k]] = 0;                                        \
            }                                                           \
        }                                                               \
    }                                                                   \
}                                                                       \
                                                                        \
template <class A, enableifc_<A::rank_s()==1 && !has_tensorindex<std::decay_t<A>>::value, int> = 0> \
bool JOIN(ply_, NAME)(A && a)                                           \
{                                                                       \
    auto s = a.size(0)*a.preferred_stride(0);                           \
    auto ss0 = a.stride(0);                                             \
    auto p = a.flat();                                                  \
    for (auto end=p+s; p!=end; p+=ss0) {                                \
        if (OP (*p)) {                                                  \
            return OP true;                                             \
        }                                                               \
    }                                                                   \
    return OP false;                                                    \
}                                                                       \
                                                                        \
template <class A> bool NAME(A && a)                                    \
{                                                                       \
    return JOIN(ply_, NAME)(start(a));                                  \
}

DEF_SHORT_CIRCUIT_PLY(every, !)
DEF_SHORT_CIRCUIT_PLY(any, /* nop */)

#undef DEF_SHORT_CIRCUIT_PLY

} // namespace ra

#endif // RA_PLY_H
