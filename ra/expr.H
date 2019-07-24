// -*- mode: c++; coding: utf-8 -*-
/// @file expr.H
/// @brief Operator nodes for expression templates.

// (c) Daniel Llorens - 2011-2014, 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/ply.H"
#include "ra/match.H"

namespace ra {

// Manipulate ET through flat (raw pointer-like) iterators P ...
template <class Op, class T, class I=mp::iota<mp::len<T>>> struct Flat;

template <class Op, class T, int ... I>
struct Flat<Op, T, mp::int_list<I ...>>
{
    Op & op;
    T t;
    template <class S> constexpr void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
    constexpr decltype(auto) operator*() { return op(*std::get<I>(t) ...); }
};

template <class Op, class ... P> inline constexpr auto
flat(Op & op, P && ... p)
{
    return Flat<Op, std::tuple<P ...>> { op, std::tuple<P ...> { std::forward<P>(p) ... } };
}

template <class Op, class ... P> inline constexpr auto
expr(Op && op, P && ... p)
{
    return Expr<Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(p) ... };
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

// forward decl in atom.H
template <class Op, class ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>: public Match<std::tuple<P ...>>
{
    using Match_ = Match<std::tuple<P ...>>;
    Op op;

// see test/ra-9.C [ra01] for forward() here.
    constexpr Expr(Op op_, P ... p_): Match_(std::forward<P>(p_) ...), op(std::forward<Op>(op_)) {}

    template <class J> constexpr decltype(auto)
    at(J const & i)
    {
        return op(std::get<I>(this->t).at(i) ...);
    }

    constexpr decltype(auto)
    flat()
    {
        return ra::flat(op, std::get<I>(this->t).flat() ...);
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    using scalar = decltype(*(ra::flat(op, std::get<I>(Match_::t).flat() ...)));
    operator scalar()
    {
        if constexpr (this->rank_s()!=1 || size_s(*this)!=1) { // for coord types; so fixed only
            if constexpr (this->rank_s()!=0) {
                static_assert(this->rank_s()==RANK_ANY);
                assert(this->rank()==0);
            }
        }
        return *flat();
    }

// forward to make sure value y is not misused as ref [ra05].
#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

} // namespace ra
