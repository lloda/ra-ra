// -*- mode: c++; coding: utf-8 -*-
// ek/box - Special object len

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// [X] if constexpr (no len in three) then don't rt-walk the tree, just identity
// [X] support Pick like Expr
// [ ] handle iota with len (meaning also expr!) members
// [ ] plug in view.operator() etc.

#include "ra/test.hh"
#include "ra/mpdebug.hh"
#include <iomanip>
#include <chrono>
#include <span>

using std::cout, std::endl, std::flush;

namespace ra {

// Exists solely to be rewritten, never to be ply()ed
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

static_assert(IteratorConcept<Len>);
constexpr Len len {};

// Don't attempt to reduce len+len etc.
template <> constexpr bool is_special_def<Len> = true;

RA_IS_DEF(has_len, false)

template <>
constexpr bool has_len_def<Len> = true;

template <IteratorConcept ... P>
constexpr bool has_len_def<Pick<std::tuple<P ...>>> = (has_len<P> || ...);

template <class Op, IteratorConcept ... P>
constexpr bool has_len_def<Expr<Op, std::tuple<P ...>>> = (has_len<P> || ...);

template <class E_>
struct WithSize
{
    static_assert(IteratorConcept<E_>);
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return std::forward<E>(e);
    }
};

template <>
struct WithSize<Len>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return Scalar<dim_t>(len);
    }
};

template <class Op, IteratorConcept ... P, int ... I>
requires (has_len<P> || ...)
struct WithSize<Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return expr(std::forward<E>(e).op, WithSize<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <IteratorConcept ... P, int ... I>
requires (has_len<P> || ...)
struct WithSize<Pick<std::tuple<P ...>, mp::int_list<I ...>>>
{
    template <class E> constexpr static decltype(auto)
    f(dim_t len, E && e)
    {
        return pick(WithSize<std::decay_t<P>>::f(len, std::get<I>(std::forward<E>(e).t)) ...);
    }
};

template <class E>
constexpr decltype(auto)
with_size(dim_t len, E && e)
{
    return WithSize<std::decay_t<E>>::f(len, std::forward<E>(e));
}

} // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
    tr.section("type predicates");
    {
        tr.test(ra::IteratorConcept<ra::Len>);
        tr.test(!ra::is_scalar<ra::Len>);
        tr.test(ra::is_zero_or_scalar<ra::Len>);
        tr.test(ra::ra_zero<ra::Len>);
        tr.test(!ra::is_ra_pos_rank<ra::Len>);
        tr.test(ra::is_special<ra::Len>);
        tr.test(ra::ra_irreducible<ra::Len>);
        tr.test(!ra::ra_reducible<ra::Len>);
        tr.test(ra::ra_irreducible<ra::Len, ra::Len>);
        tr.test(!ra::ra_reducible<ra::Len, ra::Len>);
        tr.test(ra::ra_irreducible<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
        tr.test(!ra::ra_reducible<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
        tr.test(ra::is_zero_or_scalar<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
        tr.test(ra::IteratorConcept<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
    }
    tr.section("bare len");
    {
        tr.test_eq(0, (ra::len + ra::len).rank());
        tr.test_eq(0, ra::map(std::plus(), ra::len, ra::len).rank());
        tr.test_eq(3, with_size(5, ra::scalar(3)));
        tr.test_eq(5, with_size(5, ra::len));
        tr.test_eq(10, with_size(5, ra::len + ra::len));
        tr.test_eq(20, ra::with_size(5, (ra::len + ra::len) + (ra::len + ra::len)));
        tr.test_eq(100, ra::with_size(5, ra::map(std::plus(), 99, 1)));
        tr.test_eq(10, ra::with_size(5, ra::map(std::plus(), ra::len, ra::len)));
        tr.test_eq(104, with_size(5, ra::map(std::plus(), 99, ra::len)));
        tr.test_eq(10, with_size(5, ra::map(std::plus(), ra::len, ra::len)));
        tr.test_eq(19, with_size(8, ra::map(std::plus(), ra::len, ra::map(std::plus(), 3, ra::len))));
        tr.test_eq(ra::iota(10, 11), with_size(8, ra::map(std::plus(), ra::iota(10), ra::map(std::plus(), 3, ra::len))));
        tr.test_eq(ra::iota(10, 3), with_size(8, ra::map(std::plus(), ra::iota(10), 3)));
        tr.test_eq(11, sum(with_size(4, ra::pick(std::array {0, 1, 0}, ra::len, 3))));
    }
    return tr.summary();
}
