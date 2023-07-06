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
// does expr tree contain Len?
// ---------------------

template <>
constexpr bool has_len_def<Len> = true;

template <IteratorConcept ... P>
constexpr bool has_len_def<Pick<std::tuple<P ...>>> = (has_len<P> || ...);

template <class Op, IteratorConcept ... P>
constexpr bool has_len_def<Expr<Op, std::tuple<P ...>>> = (has_len<P> || ...);

template <int w, class O, class N, class S>
constexpr bool has_len_def<Iota<w, O, N, S>> = (has_len<O> || has_len<N> || has_len<S>);


// ---------------------
// replace Len in expr tree.
// ---------------------

template <class E_>
struct WithLen
{
// constant & scalar appear in Iota args. dots_t and insert_t appear in subscripts.
// FIXME what else? restrict to IteratorConcept<E_> || is_constant<E_> || is_scalar<E_> ...
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
requires (has_len<P> || ...)
struct WithLen<Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return expr(std::forward<E>(e).op, WithLen<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <IteratorConcept ... P, int ... I>
requires (has_len<P> || ...)
struct WithLen<Pick<std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return pick(WithLen<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <int w, class O, class N, class S>
requires (has_len<O> || has_len<N> || has_len<S>)
struct WithLen<Iota<w, O, N, S>>
{
// usable iota types must be either is_constant or is_scalar.
    template <class T> constexpr static decltype(auto)
    coerce(T && t)
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
