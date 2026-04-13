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

template <class T, class Dimv>
struct Brray
{
    using Dimv = Dimv_;
    Dimv dimv;
    constexpr static Dimv const simv = {};
    constexpr static rank_t R = ra::size_s<Dimv>();
#define RAC(k, f) (0==R || is_ctype<decltype(simv[k].f)>)
    consteval static rank_t rank() requires (R!=ANY) { return R; }
    constexpr rank_t rank() const requires (R==ANY) { return dimv.size(); }
    constexpr static dim_t len_s(auto k) { if constexpr (RAC(k, len)) return len(k); else return ANY; }
    constexpr static dim_t len(auto k) requires (RAC(k, len)) { return simv[k].len; }
    constexpr dim_t len(int k) const requires (!(RAC(k, len))) { return dimv[k].len; }
    constexpr static dim_t step(auto k) requires (RAC(k, step)) { return simv[k].step; }
    constexpr dim_t step(int k) const requires (!(RAC(k, step))) { return dimv[k].step; }
    constexpr dim_t size() const { return dimv_size(dimv); }
    constexpr bool empty() const { return dimv_empty(dimv); }
#undef RAC


} // namespace ra

int main()
{


    return 0;
}
