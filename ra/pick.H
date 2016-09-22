
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_PICK_H
#define RA_PICK_H

/// @file expr.H
/// @brief Select one of several array arguments.

// This class is needed because Expr evaluates its arguments before calling its
// operator. Note that pick() is a normal function, so its arguments will always
// be evaluated; it is the individual array elements that will not.

#include "ra/ply.H"

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_PICK RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_PICK
        #define RA_CHECK_BOUNDS_RA_PICK 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_PICK==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

// -----------------
// return type of pick expression, otherwise compiler complains of ambiguity.
// @TODO & is crude, maybe is_assignable?
// -----------------

template <class T, class Enable=void>
struct pick_type
{
    using type = mp::Apply_<std::common_type, T>;
};

// lvalue
template <class P0, class ... P>
struct pick_type<std::tuple<P0 &, P ...>,
                 std::enable_if_t<!std::is_const<P0>::value
                                  && (std::is_same<P0 &, P>::value && ...)>>
{
    using type = P0 &;
};

// const lvalue
template <class P0, class ... P>
struct pick_type<std::tuple<P0 &, P & ...>,
                 std::enable_if_t<(std::is_same<std::decay_t<P0>, std::decay_t<P>>::value && ...)
                                  && (std::is_const<P0>::value || (std::is_const<P>::value || ...))>>
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
auto pick_at(size_t p0, T && t, J const & j)
    -> std::enable_if_t<(I+2==std::tuple_size<std::decay_t<T>>::value), pick_at_t<T, J>>
{
    CHECK_BOUNDS(p0==I);
    return std::get<I+1>(t).at(j);
}

template <size_t I, class T, class J> inline constexpr
auto pick_at(size_t p0, T && t, J const & j)
    -> std::enable_if_t<(I+2<std::tuple_size<std::decay_t<T>>::value), pick_at_t<T, J>>
{
    if (p0==I) {
        return std::get<I+1>(t).at(j);
    } else {
        return pick_at<I+1>(p0, t, j);
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = typename pick_type<std::tuple<decltype(*std::declval<P>()) ...>>::type;
};
template <class T> using pick_star_t = typename pick_star_type<mp::Drop1_<std::decay_t<T>>>::type;

template <size_t I, class T> inline constexpr
auto pick_star(size_t p0, T && t)
    -> std::enable_if_t<(I+2==std::tuple_size<std::decay_t<T>>::value), pick_star_t<T>>
{
    CHECK_BOUNDS(p0==I);
    return *(std::get<I+1>(t));
}

template <size_t I, class T> inline constexpr
auto pick_star(size_t p0, T && t)
    -> std::enable_if_t<(I+2<std::tuple_size<std::decay_t<T>>::value), pick_star_t<T>>
{
    if (p0==I) {
        return *(std::get<I+1>(t));
    } else {
        return pick_star<I+1>(p0, t);
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
    using T = std::tuple<P0, P ...>;

    T t;
    PickFlat(P0 p0_, P ... p_): t(p0_, p_ ...) {}

// @TODO after https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68377 try std::get<I>(t) += std::get<I>(s), ...;
    template <int II, class S> void add(S const & s) { std::get<II>(t) += std::get<II>(s); }
    template <class S> void operator+=(S const & s) { (this->add<I>(s), ...); }
    decltype(auto) operator*() { return pick_star<0>(*std::get<0>(t), t); }
};

template <class P0, class ... P> inline constexpr
auto pick_in(P0 && p0, P && ... p)
{
    return Pick<std::tuple<P0, P ...>> { std::forward<P0>(p0), std::forward<P>(p) ... };
}

template <class P0, class ... P> inline constexpr
auto pick(P0 && p0, P && ... p)
{
    return pick_in(start(std::forward<P0>(p0)), start(std::forward<P>(p)) ...);
}

// forward decl in atom.H
template <class P0, class ... P, int ... I>
struct Pick<std::tuple<P0, P ...>, std::integer_sequence<int, I ...>>
{
    using T = std::tuple<P0, P ...>;

// A-th argument decides rank and shape.
    constexpr static int A = largest_rank<P0, P ...>::value;
    using PA = std::decay_t<mp::Ref_<T, A>>;
    using NotA = mp::ComplementList_<mp::int_list<A>, std::tuple<mp::int_t<I> ...>>;
    using flat_t = PickFlat<std::tuple<decltype(std::declval<P0>().flat()), decltype(std::declval<P>().flat()) ...>>;
    using stride_t = std::tuple<decltype(std::declval<P0>().stride(0)), decltype(std::declval<P>().stride(0)) ...>;

    T t;

// If driver is RANK_ANY, driver selection should wait til run time, unless we can tell statically that RANK_ANY would be selected anyway.
    constexpr static bool HAS_DRIVER = PA::size_s()!=DIM_BAD ; //&& (PA::rank_s()!=RANK_ANY || sizeof...(P)==1);

    template <bool depend=true> constexpr auto & driver() const
    {
        static_assert(HAS_DRIVER, "can't drive this xpr");
        return std::get<A>(t);
    }

    template <int iarg>
    std::enable_if_t<(iarg==mp::len<NotA>), bool>
    check(int const driver_rank) const { return true; }

    template <int iarg>
    std::enable_if_t<(iarg<mp::len<NotA>), bool>
    check(int const driver_rank) const
    {
        rank_t ranki = std::get<mp::Ref_<NotA, iarg>::value>(t).rank();
// Provide safety where RANK_ANY was selected as driver in a leap of faith. @TODO Disable this test when it's known statically that it isn't needed. @TODO Do dynamic driver selection if static is uncertain.
        assert(ranki<=driver_rank && "driver not max rank (could be RANK_ANY)");
        for (int k=0; k!=ranki; ++k) {
// sk0==DIM_BAD might happen on subexpressions.
            dim_t sk0 = std::get<A>(t).size(k);
            if (sk0!=DIM_BAD) {
                dim_t sk = std::get<mp::Ref_<NotA, iarg>::value>(t).size(k);
                assert((sk==sk0 || sk==DIM_BAD) && "mismatched dimensions");
            }
        }
        return check<iarg+1>(driver_rank);
    }

// see test-compatibility.C [a1] for forward() here.
    constexpr Pick(P0 p0_, P ... p_): t(std::forward<P0>(p0_), std::forward<P>(p_) ...)
    {
// @TODO Try to static_assert. E.g., size_s() vs size_s() can static_assert if we try real3==real2.
// @TODO Should check only the driver: do this on ply.
        CHECK_BOUNDS(check<0>(rank()));
    }

    template <class J> constexpr decltype(auto) at(J const & j)
    {
        return pick_at<0>(std::get<0>(t).at(j), t, j);
    }
    constexpr void adv(rank_t const k, dim_t const d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }

// there's one size (by A), but each arg has its own strides.
// Note: do not require driver. This is needed by check for all leaves.
    constexpr auto size(int const i) const -> dim_t
    {
        return std::get<A>(t).size(i);
    }
    constexpr bool compact_stride(dim_t const step, int const z, int const j) const
    {
        return (std::get<I>(t).compact_stride(step, z, j) && ...);
    }
    auto stride(int const i) const -> stride_t // @BUG clang 3.9 issues with constexpr/TensorIndex
    {
        return stride_t(std::get<I>(t).stride(i) ...);
    }
    auto flat() const -> flat_t // clang complains if this is constexpr, gcc does not
    {
        return flat_t(std::get<I>(t).flat() ...);
    }
    auto flat() -> flat_t // clang complains if this is constexpr, gcc does not
    {
        return flat_t(std::get<I>(t).flat() ...);
    }

    constexpr static dim_t size_s() { return PA::size_s(); }
    constexpr static rank_t rank_s() { return PA::rank_s(); }
    constexpr rank_t rank() const { return std::get<A>(t).rank(); }
    constexpr decltype(auto) shape() const { return driver().shape(); }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*std::declval<flat_t>())()
    {
        static_assert(rank_s()==0 || rank_s()==RANK_ANY || (rank_s()==1 && size_s()==1), // for coord types
                      "bad rank in conversion to scalar");
        assert(rank()==0 || (rank_s()==1 && size_s()==1)); // for coord types; so fixed only
        return *flat();
    }

// forward to make sure value y is not misused as ref. Cf. test-ra-8.C
#define DEF_EQOPS_(OP) OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }

#define DEF_EQOPS(OP) template <class X> void operator DEF_EQOPS_(OP)
    FOR_EACH(DEF_EQOPS, =, *=, +=, -=, /=)
#undef DEF_EQOPS
};

#undef DEF_EQOPS_

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_PICK

#endif // RA_PICK_H
