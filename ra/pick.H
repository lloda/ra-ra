
// (c) Daniel Llorens - 2016-2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file pick.H
/// @brief Expression template that picks one of several arguments.

// This class is needed because Expr evaluates its arguments before calling its
// operator. Note that pick() is a normal function, so its arguments will always
// be evaluated; it is the individual array elements that will not.

#pragma once
#include "ra/ply.H"

#ifndef RA_CHECK_BOUNDS_PICK
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_PICK 1
  #else
    #define RA_CHECK_BOUNDS_PICK RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_PICK==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

// -----------------
// return type of pick expression, otherwise compiler complains of ambiguity.
// TODO & is crude, maybe is_assignable?
// -----------------

template <class T, class Enable=void>
struct pick_type
{
    using type = mp::Apply_<std::common_type, T>;
};

// lvalue
template <class P0, class ... P>
struct pick_type<std::tuple<P0 &, P ...>,
                 std::enable_if_t<!std::is_const_v<P0> && (std::is_same_v<P0 &, P> && ...)>>
{
    using type = P0 &;
};

// const lvalue
template <class P0, class ... P>
struct pick_type<std::tuple<P0 &, P & ...>,
                 std::enable_if_t<(std::is_same_v<std::decay_t<P0>, std::decay_t<P>> && ...)
                                  && (std::is_const_v<P0> || (std::is_const_v<P> || ...))>>
{
    using type = P0 const &;
};

// -----------------
// runtime to compile time conversion for Pick::at() and PickFlat::operator*()
// -----------------

template <class T, class J> struct pick_at_type;
template <class ... P, class J> struct pick_at_type<std::tuple<P ...>, J>
{
    using type = typename pick_type<std::tuple<decltype(std::declval<P>().at(std::declval<J>())) ...>>::type;
};
template <class T, class J> using pick_at_t = typename pick_at_type<mp::Drop1_<std::decay_t<T>>, J>::type;

template <size_t I, class T, class J> inline constexpr
pick_at_t<T, J>
pick_at(size_t p0, T && t, J const & j)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return std::get<I+1>(t).at(j);
        } else {
            return pick_at<I+1>(p0, t, j);
        }
    } else {
        CHECK_BOUNDS(p0==I);
        return std::get<I+1>(t).at(j);
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = typename pick_type<std::tuple<decltype(*std::declval<P>()) ...>>::type;
};
template <class T> using pick_star_t = typename pick_star_type<mp::Drop1_<std::decay_t<T>>>::type;

template <size_t I, class T> inline constexpr
pick_star_t<T>
pick_star(size_t p0, T && t)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return *(std::get<I+1>(t));
        } else {
            return pick_star<I+1>(p0, t);
        }
    } else {
        CHECK_BOUNDS(p0==I);
        return *(std::get<I+1>(t));
    }
}

// -----------------
// follows closely Flat, Expr
// -----------------

// Manipulate ET through flat (raw pointer-like) iterators P ...
template <class T, class I=std::make_integer_sequence<int, mp::len<T>>>
struct PickFlat;

template <class P0, class ... P, int ... I>
struct PickFlat<std::tuple<P0, P ...>, std::integer_sequence<int, I ...>>
{
    std::tuple<P0, P ...> t;
    template <class S> void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
    decltype(auto) operator*() { return pick_star<0>(*std::get<0>(t), t); }
};

template <class P0, class ... P> inline constexpr
auto pick_flat(P0 && p0, P && ... p)
{
    return PickFlat<std::tuple<P0, P ...>> { std::tuple<P0, P ...> { std::forward<P0>(p0), std::forward<P>(p) ... } };
}

// forward decl in atom.H
template <class P0, class ... P, int ... I>
struct Pick<std::tuple<P0, P ...>, std::integer_sequence<int, I ...>>
{
// A-th argument decides rank and shape.
    constexpr static int A = largest_rank<P0, P ...>::value;
    using PA = std::decay_t<mp::Ref_<std::tuple<P0, P ...>, A>>;
    using NotA = mp::ComplementList_<mp::int_list<A>, std::tuple<mp::int_t<I> ...>>;

    std::tuple<P0, P ...> t;

// If driver is RANK_ANY, driver selection should wait til run time, unless we can tell that RANK_ANY would be selected anyway.
    constexpr static bool VALID_DRIVER = PA::size_s()!=DIM_BAD ; //&& (PA::rank_s()!=RANK_ANY || sizeof...(P)==1);

    template <int iarg>
    constexpr bool check(int const driver_rank) const
    {
        if constexpr (iarg<mp::len<NotA>) {
            rank_t ranki = std::get<mp::Ref_<NotA, iarg>::value>(t).rank();
// Provide safety where RANK_ANY was selected as driver in a leap of faith. TODO Dynamic driver selection.
            assert(ranki<=driver_rank && "driver not max rank (could be RANK_ANY)");
            for (int k=0; k!=ranki; ++k) {
                dim_t sk0 = std::get<A>(t).size(k);
                if (sk0!=DIM_BAD) { // may be == in subexpressions
                    dim_t sk = std::get<mp::Ref_<NotA, iarg>::value>(t).size(k);
                    assert((sk==sk0 || sk==DIM_BAD) && "mismatched dimensions");
                }
            }
            return check<iarg+1>(driver_rank);
        } else {
            return true;
        }
    }

// see test-compatibility.C [a1] for forward() here.
    constexpr Pick(P0 p0_, P ... p_): t(std::forward<P0>(p0_), std::forward<P>(p_) ...)
    {
// TODO Try to static_assert. E.g., size_s() vs size_s() can static_assert if we try real3==real2.
// TODO Should check only the driver: do this on ply.
        CHECK_BOUNDS(check<0>(rank()));
    }

    template <class J>
    constexpr decltype(auto) at(J const & j)
    {
        return pick_at<0>(std::get<0>(t).at(j), t, j);
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }
    constexpr bool keep_stride(dim_t step, int z, int j) const
    {
        return (std::get<I>(t).keep_stride(step, z, j) && ...);
    }
    constexpr auto stride(int i) const
    {
        return std::make_tuple(std::get<I>(t).stride(i) ...);
    }
    constexpr auto flat()
    {
        return pick_flat(std::get<I>(t).flat() ...);
    }
    constexpr auto flat() const { return flat(); }

// there's one size (by A), but each arg has its own strides.
// Note: do not require driver. This is needed by check for all leaves.
    constexpr dim_t size(int i) const { return std::get<A>(t).size(i); }
    constexpr static dim_t size_s(int i) { return PA::size_s(i); }
    constexpr static dim_t size_s() { return PA::size_s(); }
    constexpr rank_t rank() const { return std::get<A>(t).rank(); }
    constexpr static rank_t rank_s() { return PA::rank_s(); }
    constexpr decltype(auto) shape() const
    {
        static_assert(VALID_DRIVER, "can't drive this xpr");
        return std::get<A>(t).shape();
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(pick_flat(std::get<I>(t).flat() ...)))()
    {
        static_assert(rank_s()==0 || rank_s()==RANK_ANY || (rank_s()==1 && size_s()==1), // for coord types
                      "bad rank in conversion to scalar");
        assert(rank()==0 || (rank_s()==1 && size_s()==1)); // for coord types; so fixed only
        return *flat();
    }

// forward to make sure value y is not misused as ref. Cf. test-ra-8.C
#define DEF_ASSIGNOPS(OP) template <class X> void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};
#undef DEF_ASSIGNOPS_

template <class P0, class ... P> inline constexpr auto
pick_in(P0 && p0, P && ... p)
{
    return Pick<std::tuple<P0, P ...>> { std::forward<P0>(p0), std::forward<P>(p) ... };
}

template <class P0, class ... P> inline constexpr auto
pick(P0 && p0, P && ... p)
{
    return pick_in(start(std::forward<P0>(p0)), start(std::forward<P>(p)) ...);
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_PICK
