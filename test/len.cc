// -*- mode: c++; coding: utf-8 -*-
// ek/box - Special object len

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#include "mpdebug.hh"
#include <iomanip>
#include <chrono>
#include <span>

using std::cout, std::endl, std::flush;

namespace ra {

template <auto A, auto B> constexpr auto operator-(ic_t<A> const &, ic_t<B> const &) { return ic<A-B>; } // (*)

} // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
    tr.section("type predicates");
    {
        tr.test(ra::Iterator<ra::Len>);
        tr.test(!ra::is_scalar<ra::Len>);
        tr.test(ra::is_zero_or_scalar<ra::Len>);
        tr.test(!ra::is_ra_pos<ra::Len>);
        tr.test(ra::is_special<ra::Len>);
        tr.test(ra::tomap<ra::Len>);
        tr.test(!ra::toreduce<ra::Len>);
        tr.test(ra::tomap<ra::Len, ra::Len>);
        tr.test(!ra::toreduce<ra::Len, ra::Len>);
        tr.test(ra::tomap<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
        tr.test(!ra::toreduce<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
        tr.test(ra::is_zero_or_scalar<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
        tr.test(ra::Iterator<decltype((ra::len + ra::len) + (ra::len + ra::len))>);
    }
    tr.section("bare len");
    {
        static_assert(ra::has_len<decltype(ra::len+1)>);
        tr.test_eq(0, (ra::len + ra::len).rank());
        tr.test_eq(0, ra::map(std::plus(), ra::len, ra::len).rank());
        tr.test_eq(3, wlen(5, ra::scalar(3)));
        tr.test_eq(5, wlen(5, ra::len));
        tr.test_eq(10, wlen(5, ra::len + ra::len));
        tr.test_eq(20, ra::wlen(5, (ra::len + ra::len) + (ra::len + ra::len)));
        tr.test_eq(100, ra::wlen(5, ra::map(std::plus(), 99, 1)));
        tr.test_eq(10, ra::wlen(5, ra::map(std::plus(), ra::len, ra::len)));
        tr.test_eq(104, ra::wlen(5, ra::map(std::plus(), 99, ra::len)));
        tr.test_eq(10, ra::wlen(5, ra::map(std::plus(), ra::len, ra::len)));
        tr.test_eq(19, ra::wlen(8, ra::map(std::plus(), ra::len, ra::map(std::plus(), 3, ra::len))));
        tr.test_eq(ra::iota(10, 11), wlen(8, ra::map(std::plus(), ra::iota(10), ra::map(std::plus(), 3, ra::len))));
        tr.test_eq(ra::iota(10, 3), wlen(8, ra::map(std::plus(), ra::iota(10), 3)));
        tr.test_eq(11, sum(wlen(4, ra::pick(std::array {0, 1, 0}, ra::len, 3))));
    }
    tr.section("constexpr");
    {
        constexpr int val = sum(wlen(4, ra::pick(std::array {0, 1, 0}, ra::len, 3)));
        tr.test_eq(11, val);
    }
    tr.section("len in iota");
    {
        static_assert(ra::has_len<decltype(ra::iota(ra::len+1, ra::len+ra::len))>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::iota(ra::len)).i.i)>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::iota(ra::len)).n)>);
        static_assert(std::is_integral_v<decltype(wlen(10, ra::iota(ra::len+1, ra::len+ra::len)).i.i)>);
        static_assert(std::is_integral_v<decltype(wlen(10, ra::iota(ra::len+1, ra::len+ra::len)).n)>);
        tr.test_eq(ra::iota(5), wlen(5, ra::iota(ra::len)));
        tr.test_eq(ra::iota(5, 5), wlen(5, ra::iota(ra::len, ra::len)));
        tr.test_eq(ra::iota(5, 20), wlen(10, ra::iota(5, ra::len+ra::len)));
        tr.test_eq(ra::iota(10, 20), wlen(10, ra::iota(ra::len, ra::len+ra::len)));
        tr.test_eq(ra::iota(11, 20), wlen(10, ra::iota(ra::len+1, ra::len+ra::len)));
        tr.test_eq(ra::iota(10, 20, 2), wlen(10, ra::iota(ra::len, ra::len+ra::len, ra::len/5)));
    }
    tr.section("len in ptr");
    {
        int aa[] = { 1, 2, 3, 4, 5, 6 };
        int * a = aa;
        static_assert(ra::has_len<decltype(ra::ptr(a, ra::len))>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::ptr(a, ra::len)).n)>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::ptr(a, ra::len-1)).n)>);
        tr.test_eq(ra::ptr(a, 5), wlen(5, ra::ptr(a, ra::len)));
        tr.info("len in step argument").test_eq(ra::ptr(a, 3)*2, wlen(6, ra::ptr(a+1, ra::len/2, ra::len/3)));
    }
    tr.section("static len is preserved");
    {
        tr.test_eq(5, wlen(ra::ic<5>, ra::iota(ra::len)).len_s(0));
        // tr.test_eq(4, std::decay<decltype(*(wlen(ra::ic<5>, ra::len-ra::ic<1>)))>::type::value); // FIXME (*) wlen
        // tr.test_eq(5, wlen(ra::ic<5>, ra::iota(ra::len-ra::ic<1>)).ronk()); // FIXME
        // tr.test_eq(5, wlen(ra::ic<6>, ra::iota(ra::len-ra::ic<1>)).len_s(0)); // FIXME
    }
    tr.section("ra::len in ... in x.len(...)");
    {
        // ra::Big<int, 3> a({2, 3, 4}, 0);
        // int z = ra::len-1; // static assert; use outside subscript context [ra42]
        // std::cout << z << std::endl;
        // cout << a.len(ra::len-1) << endl; // FIXME
    }
    return tr.summary();
}
