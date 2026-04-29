// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Sandbox/1

// (c) Daniel Llorens - 2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "test/mpdebug.hh"
#include <memory_resource>

using std::cout, std::endl, std::flush;

// Allocator aware Array. Maybe pack dimv with data

namespace ra {

template <class T, auto dims_>
struct Brray
{
    constexpr static auto dims = dims_;
    constexpr static rank_t R = []{ if constexpr (std::is_integral_v<decltype(dims)>) return dims; else return dims.size(); }();

// from std::array
    // struct T0
    // {
    //     [[noreturn]] constexpr T & operator[](size_t) const noexcept { abort(); }
    //     constexpr explicit operator T*() const noexcept { return nullptr; }
    // };
    // [[no_unique_address]] std::conditional_t<0==size(), T0, T[size()]> cp;


//     using Dimv = Dimv_;
//     Dimv dimv;
//     constexpr static Dimv const simv = {};
// #define RAC(k, f) (0==R || is_ctype<decltype(simv[k].f)>)
//     consteval static rank_t rank() requires (R!=ANY) { return R; }
//     constexpr rank_t rank() const requires (R==ANY) { return dimv.size(); }
//     constexpr static dim_t len_s(auto k) { if constexpr (RAC(k, len)) return len(k); else return ANY; }
//     constexpr static dim_t len(auto k) requires (RAC(k, len)) { return simv[k].len; }
//     constexpr dim_t len(int k) const requires (!(RAC(k, len))) { return dimv[k].len; }
//     constexpr static dim_t step(auto k) requires (RAC(k, step)) { return simv[k].step; }
//     constexpr dim_t step(int k) const requires (!(RAC(k, step))) { return dimv[k].step; }
//     constexpr dim_t size() const { return dimv_size(dimv); }
//     constexpr bool empty() const { return dimv_empty(dimv); }
// #undef RAC

};

} // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
    tr.section("basic");
    {
        ra::Brray<int, 1> a;
        ra::Brray<int, ra::ANY> b;
        ra::Brray<int, std::array<ra::Dim, 2> {ra::Dim{3, 2}, ra::Dim{2, 1}}> c;
        cout << a.R << endl;
        cout << b.R << endl;
        cout << c.R << endl;
    }
    return tr.summary();
}
