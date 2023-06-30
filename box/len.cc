// -*- mode: c++; coding: utf-8 -*-
// ek/box - Special object len

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// [X] if constexpr (no len in three) then don't rt-walk the tree, just identity
// [X] support Pick like Expr
// [X] handle iota with len (meaning also expr!) members
// [ ] plug in view.operator() etc.

#include "ra/test.hh"
#include "ra/mpdebug.hh"
#include <iomanip>
#include <chrono>
#include <span>

using std::cout, std::endl, std::flush;

namespace ra {


// ---------------------
// magic scalar Len exists solely to be rewritten.
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

static_assert(IteratorConcept<Len>);
constexpr Len len {};

// don't reduce len+len etc.
template <> constexpr bool is_special_def<Len> = true;

// ---------------------
// replace Len in expr tree. Only use on decayed types!
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
    static_assert(IteratorConcept<E_> || mp::is_constant<E_> || is_scalar<E_>);
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
        tr.test_eq(3, with_len(5, ra::scalar(3)));
        tr.test_eq(5, with_len(5, ra::len));
        tr.test_eq(10, with_len(5, ra::len + ra::len));
        tr.test_eq(20, ra::with_len(5, (ra::len + ra::len) + (ra::len + ra::len)));
        tr.test_eq(100, ra::with_len(5, ra::map(std::plus(), 99, 1)));
        tr.test_eq(10, ra::with_len(5, ra::map(std::plus(), ra::len, ra::len)));
        tr.test_eq(104, ra::with_len(5, ra::map(std::plus(), 99, ra::len)));
        tr.test_eq(10, ra::with_len(5, ra::map(std::plus(), ra::len, ra::len)));
        tr.test_eq(19, ra::with_len(8, ra::map(std::plus(), ra::len, ra::map(std::plus(), 3, ra::len))));
        tr.test_eq(ra::iota(10, 11), with_len(8, ra::map(std::plus(), ra::iota(10), ra::map(std::plus(), 3, ra::len))));
        tr.test_eq(ra::iota(10, 3), with_len(8, ra::map(std::plus(), ra::iota(10), 3)));
        tr.test_eq(11, sum(with_len(4, ra::pick(std::array {0, 1, 0}, ra::len, 3))));
    }
    tr.section("constexpr");
    {
        constexpr int val = sum(with_len(4, ra::pick(std::array {0, 1, 0}, ra::len, 3)));
        tr.test_eq(11, val);
    }
    tr.section("len in iota");
    {
        tr.test_eq(ra::iota(5, 20), with_len(10, ra::iota(5, ra::len+ra::len)));
        tr.test_eq(ra::iota(10, 20), with_len(10, ra::iota(ra::len, ra::len+ra::len)));
        tr.test_eq(ra::iota(11, 20), with_len(10, ra::iota(ra::len+1, ra::len+ra::len)));
        tr.test_eq(ra::iota(10, 20, 2), with_len(10, ra::iota(ra::len, ra::len+ra::len, ra::len/5)));
    }
    return tr.summary();
}
