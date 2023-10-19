// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression traversal.

// (c) Daniel Llorens - 2013-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Make traversal order a parameter, some operations (e.g. output, ravel) require specific orders.
// TODO Better heuristic for traversal order.
// TODO Tiling, etc. (see eval.cc in Blitz++).
// TODO std::execution::xxx-policy
// TODO Validate output argument strides.

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
// ply, run time order/rank.
// --------------

struct Nop {};

// step() must give 0 for k>=their own rank, to allow frame matching.
template <IteratorConcept A, class Early = Nop>
constexpr auto
ply_ravel(A && a, Early && early = Nop {})
{
    rank_t rank = a.rank();
// must avoid 0-length vlas [ra40].
    if (0>=rank) {
        if (0>rank) [[unlikely]] { std::abort(); }
        if constexpr (requires {early.def;}) {
            return (*(a.flat())).value_or(early.def);
        } else {
            *(a.flat());
            return;
        }
    }
// inside first. FIXME better heuristic - but first need a way to force row-major
    rank_t order[rank];
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
    dim_t sha[rank], ind[rank] = {};
// find outermost compact dim.
    rank_t * ocd = order;
    dim_t ss = a.len(*ocd);
    for (--rank, ++ocd; rank>0 && a.keep_step(ss, order[0], *ocd); --rank, ++ocd) {
        ss *= a.len(*ocd);
    }
    for (int k=0; k<rank; ++k) {
// ss takes care of the raveled dimensions ss.
        if (0>=(sha[k]=a.len(ocd[k]))) {
            if (0>sha[k]) [[unlikely]] { std::abort(); }
            if constexpr (requires {early.def;}) {
                return early.def;
            } else {
                return;
            }
        }
    }
    auto ss0 = a.step(order[0]);
    for (;;) {
        dim_t s = ss;
        for (auto p=a.flat(); --s>=0; p+=ss0) {
            if constexpr (requires {early.def;}) {
                if (auto stop = *p) {
                    return stop.value();
                }
            } else {
                *p;
            }
        }
        for (int k=0; ; ++k) {
            if (k>=rank) {
                if constexpr (requires {early.def;}) {
                    return early.def;
                } else {
                    return;
                }
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
// ply, compile time order/rank.
// -------------------------

template <auto order, int k, int urank, class A, class S, class Early>
constexpr auto
subply(A & a, dim_t s, S const & ss0, Early & early)
{
    if constexpr (k < urank) {
        for (auto p=a.flat(); --s>=0; p+=ss0) {
            if constexpr (requires {early.def;}) {
                if (auto stop = *p) {
                    return stop;
                }
            } else {
                *p;
            }
        }
    } else {
        dim_t size = a.len(order[k]); // TODO precompute above
        for (dim_t i=0; i<size; ++i) {
            if constexpr (requires {early.def;}) {
                if (auto stop = subply<order, k-1, urank>(a, s, ss0, early)) {
                    return stop;
                }
            } else {
                subply<order, k-1, urank>(a, s, ss0, early);
            }
            a.adv(order[k], 1);
        }
        a.adv(order[k], -size);
    }
    if constexpr (requires {early.def;}) {
        return static_cast<decltype(*(a.flat()))>(std::nullopt);
    } else {
        return;
    }
}

// possible pessimization in ply_fixed(). See bench-dot [ra43]
#ifndef RA_STATIC_UNROLL
#define RA_STATIC_UNROLL 0
#endif

template <IteratorConcept A, class Early = Nop>
constexpr decltype(auto)
ply_fixed(A && a, Early && early = Nop {})
{
    constexpr rank_t rank = rank_s<A>();
    static_assert(0<=rank, "ply_fixed needs static rank");
// inside first. FIXME better heuristic - but first need a way to force row-major
    constexpr /* static P2647 gcc13 */ auto order = mp::tuple_values<int, mp::reverse<mp::iota<rank>>>();

    if constexpr (0==rank) {
        if constexpr (requires {early.def;}) {
            return (*(a.flat())).value_or(early.def);
        } else {
            *(a.flat());
            return;
        }
    } else {
        auto ss0 = a.step(order[0]);
// static keep_step implies all else is static.
        if constexpr (bool(RA_STATIC_UNROLL) && rank>1 && requires (dim_t st, rank_t z, rank_t j) { A::keep_step(st, z, j); }) {
// find outermost compact dim.
            constexpr auto sj = [&order]
            {
                dim_t ss = A::len_s(order[0]);
                int j = 1;
                for (; j<rank && A::keep_step(ss, order[0], order[j]); ++j) {
                    ss *= A::len_s(order[j]);
                }
                return std::make_tuple(ss, j);
            } ();
            if constexpr (requires {early.def;}) {
                return (subply<order, rank-1, std::get<1>(sj)>(a, std::get<0>(sj), ss0, early)).value_or(early.def);
            } else {
                subply<order, rank-1, std::get<1>(sj)>(a, std::get<0>(sj), ss0, early);
            }
        } else {
// not worth unrolling.
            if constexpr (requires {early.def;}) {
                return (subply<order, rank-1, 1>(a, a.len(order[0]), ss0, early)).value_or(early.def);
            } else {
                subply<order, rank-1, 1>(a, a.len(order[0]), ss0, early);
            }
        }
    }
}


// ---------------------------
// ply, best for each type
// ---------------------------

template <IteratorConcept A, class Early = Nop>
constexpr decltype(auto)
ply(A && a, Early && early = Nop {})
{
    static_assert(!has_len<A>, "len used outside subscript context.");
    static_assert(0<=rank_s<A>() || ANY==rank_s<A>());
    if constexpr (ANY==size_s<A>()) {
        return ply_ravel(std::forward<A>(a), std::forward<Early>(early));
    } else {
        return ply_fixed(std::forward<A>(a), std::forward<Early>(early));
    }
}

template <class Op, class ... A>
constexpr void
for_each(Op && op, A && ... a)
{
    ply(map(std::forward<Op>(op), std::forward<A>(a) ...));
}


// ---------------------------
// ply, short-circuiting
// ---------------------------

template <class T> struct Default { T def; };
template <class T> Default(T &&) -> Default<T>;

template <IteratorConcept A, class Def>
constexpr decltype(auto)
early(A && a, Def && def)
{
    return ply(std::forward<A>(a), Default {def});
}


// --------------------
// STLIterator for CellSmall / CellBig. FIXME make it work for any IteratorConcept.
// --------------------

template <class S, class I, class P>
constexpr void
cube_next(rank_t k, S const & it, I & i, P & p)
{
    for (; k>=0; --k) {
        if (++i[k]<it.len(k)) {
            p += it.step(k);
            return;
        } else {
            i[k] = 0;
            p -= it.step(k)*(it.len(k)-1);
        }
    }
    p = nullptr;
}

template <int k, class S, class I, class P>
constexpr void
cube_next(S const & it, I & i, P & p)
{
    if constexpr (k>=0) {
        if (++i[k]<it.len(k)) {
            p += it.step(k);
        } else {
            i[k] = 0;
            p -= it.step(k)*(it.len(k)-1);
            cube_next<k-1>(it, i, p);
        }
    } else {
        p = nullptr;
    }
}

template <class Iterator>
struct STLIterator
{
    using difference_type = dim_t;
    using value_type = typename Iterator::value_type;
    using shape_type = decltype(ra::shape(std::declval<Iterator>()));

    Iterator ii;
    shape_type i;
    STLIterator(STLIterator const & it) = default;
    constexpr STLIterator & operator=(STLIterator const & it)
    {
        i = it.i;
        ii.Iterator::~Iterator(); // no-op except for View<ANY>. Still...
        new (&ii) Iterator(it.ii); // avoid ii = it.ii [ra11]
        return *this;
    }
    STLIterator(Iterator const & ii_)
        : ii(ii_),
// shape_type may be std::array or std::vector.
          i([&] {
              if constexpr (ANY==Iterator::rank_s()) {
                  return shape_type(ii.rank(), 0);
              } else {
                  return shape_type {0};
              }
          }())
    {
// [ra12] Null p_ so begin()==end() for empty range. ply() uses lens so this doesn't matter.
        if (0==ra::size(ii)) {
            ii.c.cp = nullptr;
        }
    };

    template <class PP> bool operator==(PP const & j) const { return ii.c.cp==j.ii.c.cp; }

    decltype(auto) operator*() const { if constexpr (0==Iterator::cellr) return *ii.c.cp; else return ii.c; }
    decltype(auto) operator*() { if constexpr (0==Iterator::cellr) return *ii.c.cp; else return ii.c; }
    STLIterator & operator++()
    {
        if constexpr (ANY==Iterator::rank_s()) {
            cube_next(ii.rank()-1, ii, i, ii.c.cp);
        } else {
            cube_next<Iterator::rank_s()-1>(ii, i, ii.c.cp);
        }
        return *this;
    }
    STLIterator & operator++(int)
    {
        auto old = *this;
        ++(*this);
        return old;
    }
};


// ---------------------------
// i/o
// ---------------------------

// TODO once ply_ravel lets one specify row-major, reuse that.
template <class A>
inline std::ostream &
operator<<(std::ostream & o, FormatArray<A> const & fa)
{
    static_assert(!has_len<A>, "len used outside subscript context.");
// FIXME note that this copies / resets the Iterator if fa.a already is one; see [ra35].
    auto a = ra::start(fa.a);
    static_assert(size_s(a)!=BAD, "cannot print type");
    rank_t const rank = a.rank();
    auto sha = shape(a);
    if (withshape==fa.shape || (defaultshape==fa.shape && size_s(a)==ANY)) {
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
template <class C> requires (!is_scalar<C> && size_s<C>()!=ANY)
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
template <class C> requires (size_s<C>()==ANY && !std::is_convertible_v<C, std::string_view>)
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
