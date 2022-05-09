// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Nodes for expression templates.

// (c) Daniel Llorens - 2011-2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"
#include "wrank.hh"

namespace ra {

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Expr;

template <class Op, class ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>: public Match<std::tuple<P ...>>
{
    template <class T_>
    struct Flat
    {
        Op & op;
        T_ t;
        template <class S> constexpr void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
        constexpr decltype(auto) operator*() { return op(*std::get<I>(t) ...); }
    };

    template <class ... P_> inline constexpr static auto
    flat(Op & op, P_ && ... p)
    {
        return Flat<std::tuple<P_ ...>> { op, std::tuple<P_ ...> { std::forward<P_>(p) ... } };
    }

    using Match_ = Match<std::tuple<P ...>>;
    Op op;

// test/ra-9.cc [ra1]
    constexpr Expr(Op op_, P ... p_): Match_(std::forward<P>(p_) ...), op(std::forward<Op>(op_)) {}
    RA_DEF_ASSIGNOPS_SELF(Expr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    template <class J> constexpr decltype(auto)
    at(J const & i)
    {
        return op(std::get<I>(this->t).at(i) ...);
    }

    template <class J> constexpr decltype(auto)
    at(J const & i) const
    {
        return op(std::get<I>(this->t).at(i) ...);
    }

    constexpr decltype(auto)
    flat()
    {
        return flat(op, std::get<I>(this->t).flat() ...);
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(flat(op, std::get<I>(Match_::t).flat() ...))) ()
    {
        if constexpr (this->rank_s()!=1 || size_s(*this)!=1) { // for coord types; so fixed only
            if constexpr (this->rank_s()!=0) {
                static_assert(this->rank_s()==RANK_ANY);
                assert(this->rank()==0);
            }
        }
        return *flat();
    }
};

template <class V, class ... T, int ... i> inline constexpr auto
expr_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return expr(FM::op(std::forward<V>(v)), reframe<mp::ref<typename FM::R, i>>(std::forward<T>(t)) ...);
}

template <class Op, class ... P> inline constexpr auto
expr(Op && op, P && ... p)
{
    if constexpr (is_verb<Op>) {
        return expr_verb(mp::iota<sizeof...(P)> {}, std::forward<Op>(op), std::forward<P>(p) ...);
    } else {
        return Expr<Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(p) ... };
    }
}

template <class Op, class ... A> inline constexpr auto
map(Op && op, A && ... a)
{
    return expr(std::forward<Op>(op), start(std::forward<A>(a)) ...);
}

template <class Op, class ... A> inline constexpr void
for_each(Op && op, A && ... a)
{
    ply(map(std::forward<Op>(op), std::forward<A>(a) ...));
}

} // namespace ra
