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

int main()
{
    ra::TestRecorder tr(std::cout);
    tr.section("type predicates");
    {
        tr.test(ra::IteratorConcept<ra::Len>);
        tr.test(!ra::is_scalar<ra::Len>);
        tr.test(ra::is_zero_or_scalar<ra::Len>);
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
        static_assert(ra::has_len<decltype(ra::len+1)>);
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
        static_assert(ra::has_len<decltype(ra::iota(ra::len+1, ra::len+ra::len))>);
        static_assert(std::is_integral_v<decltype(with_len(5, ra::iota(ra::len)).i)>);
        static_assert(std::is_integral_v<decltype(with_len(5, ra::iota(ra::len)).n)>);
        static_assert(std::is_integral_v<decltype(with_len(10, ra::iota(ra::len+1, ra::len+ra::len)).i)>);
        static_assert(std::is_integral_v<decltype(with_len(10, ra::iota(ra::len+1, ra::len+ra::len)).n)>);
        tr.test_eq(ra::iota(5), with_len(5, ra::iota(ra::len)));
        tr.test_eq(ra::iota(5, 5), with_len(5, ra::iota(ra::len, ra::len)));
        tr.test_eq(ra::iota(5, 20), with_len(10, ra::iota(5, ra::len+ra::len)));
        tr.test_eq(ra::iota(10, 20), with_len(10, ra::iota(ra::len, ra::len+ra::len)));
        tr.test_eq(ra::iota(11, 20), with_len(10, ra::iota(ra::len+1, ra::len+ra::len)));
        tr.test_eq(ra::iota(10, 20, 2), with_len(10, ra::iota(ra::len, ra::len+ra::len, ra::len/5)));
    }
    tr.section("len in ptr");
    {
        int a[] = { 1, 2, 3, 4, 5 };
        static_assert(ra::has_len<decltype(ra::ptr(a, ra::len))>);
        static_assert(std::is_integral_v<decltype(with_len(5, ra::ptr(a, ra::len)).n)>);
        static_assert(std::is_integral_v<decltype(with_len(5, ra::ptr(a, ra::len-1)).n)>);
        tr.test_eq(ra::ptr(a, 5), with_len(5, ra::ptr(a, ra::len)));
    }
    return tr.summary();
}
