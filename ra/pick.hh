// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression template that picks one of several arguments.

// (c) Daniel Llorens - 2016-2017, 2019, 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// This class is needed because Expr evaluates its arguments before calling its
// operator. The implementation itself is parallel to Expr / Flat. Note that
// pick() is a normal function, so its *arguments* will always be evaluated; it
// is the individual array elements that will not be.

#pragma once
#include "match.hh"

namespace ra {

// -----------------
// return type of pick expression, otherwise compiler complains of ambiguity.
// TODO & is crude, maybe is_assignable?
// -----------------

template <class T>
struct pick_type
{
    using type = mp::apply<std::common_type_t, T>;
};

// lvalue
template <class P0, class ... P>
requires (!std::is_const_v<P0> && (std::is_same_v<P0 &, P> && ...))
struct pick_type<std::tuple<P0 &, P ...>>
{
    using type = P0 &;
};

// const lvalue
template <class P0, class ... P>
requires ((std::is_same_v<std::decay_t<P0>, std::decay_t<P>> && ...)
          && (std::is_const_v<P0> || (std::is_const_v<P> || ...)))
struct pick_type<std::tuple<P0 &, P & ...>>
{
    using type = P0 const &;
};

// -----------------
// runtime to compile time conversion for Pick::at() and Pick::Flat::operator*()
// -----------------

template <class T, class J> struct pick_at_type;
template <class ... P, class J> struct pick_at_type<std::tuple<P ...>, J>
{
    using type = typename pick_type<std::tuple<decltype(std::declval<P>().at(std::declval<J>())) ...>>::type;
};
template <class T, class J> using pick_at_t = typename pick_at_type<mp::drop1<std::decay_t<T>>, J>::type;

template <std::size_t I, class T, class J>
constexpr pick_at_t<T, J>
pick_at(std::size_t p0, T && t, J const & j)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return std::get<I+1>(t).at(j);
        } else {
            return pick_at<I+1>(p0, t, j);
        }
    } else {
        RA_CHECK(p0==I, " p0 ", p0, " I ", I);
        return std::get<I+1>(t).at(j);
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = typename pick_type<std::tuple<decltype(*std::declval<P>()) ...>>::type;
};
template <class T> using pick_star_t = typename pick_star_type<mp::drop1<std::decay_t<T>>>::type;

template <std::size_t I, class T>
constexpr pick_star_t<T>
pick_star(std::size_t p0, T && t)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return *(std::get<I+1>(t));
        } else {
            return pick_star<I+1>(p0, t);
        }
    } else {
        RA_CHECK(p0==I, " p0 ", p0, " I ", I);
        return *(std::get<I+1>(t));
    }
}

template <class T, class K=mp::iota<mp::len<T>>> struct Pick;

template <class ... P, int ... I>
struct Pick<std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    static_assert(sizeof...(P)>1);

    template <class T_>
    struct Flat
    {
        T_ t;
        template <class S> void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
        decltype(auto) operator*() { return pick_star<0>(*std::get<0>(t), t); }
    };

    template <class ... P_>
    constexpr static auto
    flat(P_ && ... p)
    {
        return Flat<std::tuple<P_ ...>> { std::tuple<P_ ...> { std::forward<P_>(p) ... } };
    }

    using Match_ = Match<true, std::tuple<P ...>>;

// test/ra-9.cc [ra1]
    constexpr Pick(P ... p_): Match_(std::forward<P>(p_) ...) {}
    RA_DEF_ASSIGNOPS_SELF(Pick)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    template <class J>
    constexpr decltype(auto)
    at(J const & j)
    {
        return pick_at<0>(std::get<0>(this->t).at(j), this->t, j);
    }

    template <class J>
    constexpr decltype(auto)
    at(J const & j) const
    {
        return pick_at<0>(std::get<0>(this->t).at(j), this->t, j);
    }

    constexpr decltype(auto)
    flat()
    {
        return flat(std::get<I>(this->t).flat() ...);
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(flat(std::get<I>(Match_::t).flat() ...))) ()
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

template <class ... P>
constexpr auto
pick_in(P && ... p)
{
    return Pick<std::tuple<P ...>> { std::forward<P>(p) ... };
}

template <class ... P>
constexpr auto
pick(P && ... p)
{
    return pick_in(start(std::forward<P>(p)) ...);
}

} // namespace ra
