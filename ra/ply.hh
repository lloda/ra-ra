// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Traverse expression.

// (c) Daniel Llorens - 2013-2019, 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Lots of room for improvement: small (fixed sizes) and large (tiling, etc. see eval.cc in Blitz++).
// TODO Traversal order should be a parameter, since some operations (e.g. output, ravel) require a specific order.
// TODO Better heuristic for traversal order.
// TODO std::execution::xxx-policy, validate output argument strides.

#pragma once
#include "expr.hh"

namespace ra {


// ---------------------
// does expr tree contain Len?
// ---------------------

template <>
constexpr bool has_len_def<Len> = true;

template <IteratorConcept ... P>
constexpr bool has_len_def<Pick<std::tuple<P ...>>> = (has_len<P> || ...);

template <class Op, IteratorConcept ... P>
constexpr bool has_len_def<Expr<Op, std::tuple<P ...>>> = (has_len<P> || ...);

template <int w, class O, class N, class S>
constexpr bool has_len_def<Iota<w, N, O, S>> = (has_len<N> || has_len<O> || has_len<S>);

template <class I, class N>
constexpr bool has_len_def<Ptr<I, N>> = has_len<N>;


// ---------------------
// replace Len in expr tree.
// ---------------------

template <class E_>
struct WithLen
{
// constant & scalar appear in Iota args. dots_t and insert_t appear in subscripts.
// FIXME what else? restrict to IteratorConcept<E_> || is_constant<E_> || is_scalar<E_> ...
    template <class L, class E> constexpr static decltype(auto)
    f(L len, E && e)
    {
        return std::forward<E>(e);
    }
};

template <>
struct WithLen<Len>
{
    template <class L, class E> constexpr static decltype(auto)
    f(L len, E && e)
    {
        return Scalar<L>(len);
    }
};

template <class Op, IteratorConcept ... P, int ... I>
requires (has_len<P> || ...)
struct WithLen<Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class L, class E> constexpr static decltype(auto)
    f(L len, E && e)
    {
        return expr(std::forward<E>(e).op, WithLen<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <IteratorConcept ... P, int ... I>
requires (has_len<P> || ...)
struct WithLen<Pick<std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class L, class E> constexpr static decltype(auto)
    f(L len, E && e)
    {
        return pick(WithLen<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

// usable iota types must be either is_constant or is_scalar.
template <class T>
constexpr static decltype(auto)
coerce(T && t)
{
    if constexpr (IteratorConcept<T>) {
        return FLAT(t);
    } else {
        return std::forward<T>(t);
    }
}

template <int w, class N, class O, class S>
requires (has_len<N> || has_len<O> || has_len<S>)
struct WithLen<Iota<w, N, O, S>>
{
    template <class L, class E> constexpr static decltype(auto)
    f(L len, E && e)
    {
        return iota<w>(coerce(WithLen<std::decay_t<N>>::f(len, std::forward<E>(e).n)),
                       coerce(WithLen<std::decay_t<O>>::f(len, std::forward<E>(e).i)),
                       coerce(WithLen<std::decay_t<S>>::f(len, std::forward<E>(e).s)));
    }
};

template <class I, class N>
requires (has_len<N>)
struct WithLen<Ptr<I, N>>
{
    template <class L, class E> constexpr static decltype(auto)
    f(L len, E && e)
    {
        return ptr(std::forward<E>(e).i, coerce(WithLen<std::decay_t<N>>::f(len, std::forward<E>(e).n)));
    }
};

template <class L, class E>
constexpr decltype(auto)
with_len(L len, E && e)
{
    static_assert(std::is_integral_v<std::decay_t<L>> || is_constant<std::decay_t<L>>);
    return WithLen<std::decay_t<E>>::f(len, std::forward<E>(e));
}


// --------------
// ply, run time order
// --------------

// Traverse array expression looking to ravel the inner loop.
// step() must give 0 for k>=their own rank, to allow frame matching.
template <IteratorConcept A>
inline void
ply_ravel(A && a)
{
    rank_t rank = a.rank();
    if (0>=rank) {
// FIXME always check, else compiler may complain of bad vla; see test in [ra40].
        if (0>rank) [[unlikely]] { std::abort(); }
        *(a.flat());
        return;
    }
    rank_t order[rank];
    dim_t sha[rank], ind[rank] = {};
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
    // FIXME better heuristic - but first need a way to force row-major
    // if (rank>1) {
    //     std::sort(order, order+rank, [&a, &order](auto && i, auto && j)
    //               { return a.len(order[i])<a.len(order[j]); });
    // }
// find outermost compact dim.
    rank_t * ocd = order;
    dim_t ss = a.len(*ocd);
    for (--rank, ++ocd; rank>0 && a.keep_step(ss, order[0], *ocd); --rank, ++ocd) {
        ss *= a.len(*ocd);
    }
    for (int k=0; k<rank; ++k) {
// ss takes care of the raveled dimensions ss.
        if (0 == (sha[k]=a.len(ocd[k]))) {
            return;
        }
        RA_CHECK(DIM_BAD!=sha[k], "Undefined len[", ocd[k], "].");
    }
// all sub xpr steps advance in compact dims, as they might be different.
    auto const ss0 = a.step(order[0]);
    for (;;) {
        dim_t s = ss;
        for (auto p=a.flat(); --s>=0; p+=ss0) {
            *p;
        }
        for (int k=0; ; ++k) {
            if (k>=rank) {
                return;
            } else if (++ind[k]<sha[k]) {
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
// ply, compile time order
// -------------------------

template <class order, int ravel_rank, class A, class S>
constexpr void
subindex(A & a, dim_t s, S const & ss0)
{
    if constexpr (mp::len<order> == ravel_rank) {
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wstringop-overflow"
#pragma GCC diagnostic warning "-Wstringop-overread"
        for (auto p=a.flat(); --s>=0; p+=ss0) {
            *p;
        }
#pragma GCC diagnostic pop
    } else {
        dim_t size = a.len(mp::first<order>::value); // TODO Precompute above
        for (dim_t i=0; i<size; ++i) {
            subindex<mp::drop1<order>, ravel_rank>(a, s, ss0);
            a.adv(mp::first<order>::value, 1);
        }
        a.adv(mp::first<order>::value, -size);
    }
}

// convert runtime jj into compile time j. TODO a.adv<k>().
template <class order, int j, int jj, class A, class S>
constexpr void
until(A & a, dim_t s, S const & ss0)
{
    if constexpr (j<jj) {
        until<order, j+1, jj>(a, s, ss0);
    } else {
        subindex<order, j>(a, s, ss0);
    }
}

template <IteratorConcept A>
constexpr void
plyf(A && a)
{
    constexpr rank_t rank = rank_s<A>();
    static_assert(0<=rank, "plyf needs static rank");

    if constexpr (0==rank) {
        *(a.flat());
// static unrolling. static keep_step implies all else is also static.
    } else if constexpr (rank>1 && requires (dim_t d, rank_t i, rank_t j) { A::keep_step(d, i, j); }) {
// find outermost compact dim.
        constexpr auto sj = []
        {
            dim_t s = A::len_s(rank-1);
            int j = 1;
            while (j<rank && A::keep_step(s, rank-1, rank-1-j)) {
                s *= A::len_s(rank-1-j);
                ++j;
            }
            return std::make_tuple(s, j);
        } ();
// sub xpr steps advance in compact dims, as they might differ. Note that order here is inverse of order.
        until<mp::iota<rank>, 0, std::get<1>(sj)>(a, std::get<0>(sj), a.step(rank-1));
    } else {
// not worth unrolling.
        subindex<mp::iota<rank>, 1>(a, a.len(rank-1), a.step(rank-1));
    }
}


// ---------------------------
// select best for each type
// ---------------------------

template <IteratorConcept A>
constexpr void
ply(A && a)
{
    static_assert(!has_len<A>, "len used outside subscript context.");
    static_assert(0<=rank_s<A>() || RANK_ANY==rank_s<A>());

    if constexpr (DIM_ANY==size_s<A>()) {
        ply_ravel(std::forward<A>(a));
    } else {
        plyf(std::forward<A>(a));
    }
}


// ---------------------------
// ply, short-circuiting
// ---------------------------

// TODO Refactor with ply_ravel. Make exit available to plyf.
// TODO These are reductions. How about higher rank?
template <IteratorConcept A, class DEF>
inline auto
ply_ravel_exit(A && a, DEF && def)
{
    static_assert(!has_len<A>, "len used outside subscript context.");
    static_assert(0<=rank_s<A>() || RANK_ANY==rank_s<A>());

    rank_t rank = a.rank();
    if (0>=rank) {
// FIXME always check, else compiler may complain of bad vla; see test in [ra40].
        if (0>rank) [[unlikely]] { std::abort(); }
        if (auto what = *(a.flat()); std::get<0>(what)) {
            return std::get<1>(what);
        }
        return def;
    }
    rank_t order[rank];
    dim_t sha[rank], ind[rank] = {};
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
    // FIXME better heuristic - but first need a way to force row-major
    // if (rank>1) {
    //     std::sort(order, order+rank, [&a, &order](auto && i, auto && j)
    //               { return a.len(order[i])<a.len(order[j]); });
    // }
// find outermost compact dim.
    rank_t * ocd = order;
    dim_t ss = a.len(*ocd);
    for (--rank, ++ocd; rank>0 && a.keep_step(ss, order[0], *ocd); --rank, ++ocd) {
        ss *= a.len(*ocd);
    }
    for (int k=0; k<rank; ++k) {
// ss takes care of the raveled dimensions ss.
        if (0 == (sha[k]=a.len(ocd[k]))) {
            return def;
        }
        RA_CHECK(DIM_BAD!=sha[k], "Undefined len[", ocd[k], "].");
    }
// all sub xpr steps advance in compact dims, as they might be different.
    auto const ss0 = a.step(order[0]);
    for (;;) {
        dim_t s = ss;
        for (auto p=a.flat(); --s>=0; p+=ss0) {
            if (auto what = *p; std::get<0>(what)) {
                return std::get<1>(what);
            }
        }
        for (int k=0; ; ++k) {
            if (k>=rank) {
                return def;
            } else if (++ind[k]<sha[k]) {
                a.adv(ocd[k], 1);
                break;
            } else {
                ind[k] = 0;
                a.adv(ocd[k], 1-sha[k]);
            }
        }
    }
}

template <IteratorConcept A, class DEF>
constexpr decltype(auto)
early(A && a, DEF && def)
{
    return ply_ravel_exit(std::forward<A>(a), std::forward<DEF>(def));
}

template <class Op, class ... A>
constexpr void
for_each(Op && op, A && ... a)
{
    ply(map(std::forward<Op>(op), std::forward<A>(a) ...));
}


// ---------------------------
// i/o
// ---------------------------

// TODO once ply_ravel lets one specify row-major, reuse that
template <class A>
inline std::ostream &
operator<<(std::ostream & o, FormatArray<A> const & fa)
{
    static_assert(!has_len<A>, "len used outside subscript context.");
// FIXME note that this copies / resets the Iterator if fa.a already is one; see [ra35].
    auto a = ra::start(fa.a);
    static_assert(size_s(a)!=DIM_BAD, "cannot print type");
    rank_t const rank = a.rank();
    auto sha = shape(a);
    if (withshape==fa.shape || (defaultshape==fa.shape && size_s(a)==DIM_ANY)) {
        o << start(sha) << '\n';
    }
    for (rank_t k=0; k<rank; ++k) {
        if (0==sha[k]) {
            return o;
        }
    }
// order here is row-major on purpose.
    auto ind = sha; for_each([](auto & s) { s=0; }, ind);
    for (;;) {
        o << *(a.flat());
        for (int k=0; ; ++k) {
            if (k>=rank) {
                return o;
            } else if (++ind[rank-1-k]<sha[rank-1-k]) {
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
        RA_CHECK(n>=0, "negative sizes in input: ", n);
        c.resize(n);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

// Expr size, so read shape and possibly allocate (TODO try to avoid).
template <class C> requires (size_s<C>()==DIM_ANY && !std::is_convertible_v<C, std::string_view>)
inline std::istream &
operator>>(std::istream & i, C & c)
{
    if (decltype(shape(c)) s; i >> s) {
        std::decay_t<C> cc(s, ra::none);
        RA_CHECK(every(start(s)>=0), "negative sizes in input: ", s);
// avoid copying in case Container's elements don't support it.
        swap(c, cc);
// need row-major, serial iteration here. FIXME use ra:: traversal.
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

} // namespace ra
