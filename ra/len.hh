// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Special object len.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "pick.hh"
#include "expr.hh"

namespace ra {


// ---------------------
// never ply(), solely to be rewritten.
// ---------------------

struct Len
{
    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static void adv(rank_t k, dim_t d) { std::abort(); }
    constexpr static dim_t step(int k) { std::abort(); }
    constexpr static bool keep_step(dim_t st, int z, int j) { std::abort(); }
    constexpr static Len const & flat() { std::abort(); }
    constexpr void operator+=(dim_t d) const { std::abort(); }
    constexpr dim_t operator*() const { std::abort(); }
};

constexpr Len len {};

// let operators build expr trees.
static_assert(IteratorConcept<Len>);
template <> constexpr bool is_special_def<Len> = true;

// ---------------------
// replace Len in expr tree.
// ---------------------

template <class T>
constexpr bool has_len = std::is_same_v<T, Len>;

template <IteratorConcept ... P>
constexpr bool has_len<Pick<std::tuple<P ...>>> = (has_len<std::decay_t<P>> || ...);

template <class Op, IteratorConcept ... P>
constexpr bool has_len<Expr<Op, std::tuple<P ...>>> = (has_len<std::decay_t<P>> || ...);

template <class E_>
struct WithLen
{
// constant & scalar are allowed for Iota args.
    static_assert(IteratorConcept<E_> || is_constant<E_> || is_scalar<E_>);
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return std::forward<E>(e);
    }
};

template <>
struct WithLen<Len>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return Scalar<dim_t>(len);
    }
};

template <class Op, IteratorConcept ... P, int ... I>
requires (has_len<std::decay_t<P>> || ...)
struct WithLen<Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return expr(std::forward<E>(e).op, WithLen<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <IteratorConcept ... P, int ... I>
requires (has_len<std::decay_t<P>> || ...)
struct WithLen<Pick<std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return pick(WithLen<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <int w, class O, class N, class S>
requires (has_len<std::decay_t<O>> || has_len<std::decay_t<O>> || has_len<std::decay_t<S>>)
struct WithLen<Iota<w, O, N, S>>
{
// usable iota types must be either is_constant or is_scalar.
    template <class T>
    constexpr static decltype(auto) coerce(T && t)
    {
        if constexpr (IteratorConcept<T>) {
            return FLAT(t);
        } else {
            return std::forward<T>(t);
        }
    }

    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return iota<w>(coerce(WithLen<std::decay_t<N>>::f(len, std::forward<E>(e).n)),
                       coerce(WithLen<std::decay_t<O>>::f(len, std::forward<E>(e).i)),
                       coerce(WithLen<std::decay_t<S>>::f(len, std::forward<E>(e).s)));
    }
};

template <class E>
constexpr decltype(auto)
with_len(dim_t len, E && e)
{
    return WithLen<std::decay_t<E>>::f(len, std::forward<E>(e));
}

} // namespace ra
