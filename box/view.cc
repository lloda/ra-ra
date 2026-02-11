// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Refactor Views (WIP)

// (c) Daniel Llorens - 2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush;

// TODO
// fix deduction of f(view); once viewbig is an alias R isn't deduced anymore. Handle viewsmall & viewbig together.

namespace ra {

template <class Dimv>
struct ViewBase
{
    constexpr static Dimv const simv = {};
    [[no_unique_address]] Dimv dimv; // FIXME const after init
};

template <is_ctype Dimv>
struct ViewBase<Dimv>
{
    constexpr static auto simv = Dimv::value;
    constexpr static auto dimv = simv;
};

template <class P, class Dimv>
struct View: public ViewBase<Dimv>
{
    static_assert(has_len<P> || std::bidirectional_iterator<P>);
    P cp;
    using ViewBase<Dimv>::simv, ViewBase<Dimv>::dimv;
    constexpr static bool CT = is_ctype<Dimv>;
// exclude T and sub constructors by making T & sub noarg
    constexpr static bool have_braces = CT && std::is_reference_v<decltype(*cp)>;
    using T = std::conditional_t<have_braces, std::remove_reference_t<decltype(*cp)>, noarg<>>;
// ---------
// on dimv
    constexpr static rank_t R = rank_frame(size_s<decltype(simv)>(), 0);
    consteval static rank_t rank() requires (ANY!=R) { return R; }
    constexpr rank_t rank() const requires (ANY==R) { return rank_t(dimv.size()); }
#define RAC(k, f) is_ctype<decltype(simv[k].f)>
    constexpr static dim_t len_s(auto k) { if constexpr (CT || RAC(k, len)) return len(k); else return ANY; }
    constexpr static dim_t len(auto k) requires (CT || RAC(k, len)) { return simv[k].len; }
    constexpr dim_t len(int k) const requires (!(CT || RAC(k, len))) { return dimv[k].len; }
    constexpr static dim_t step(auto k) requires (CT || RAC(k, step)) { return k<rank() ? simv[k].step : 0; }
    constexpr dim_t step(int k) const requires (!(CT || RAC(k, step))) { return k<rank() ? dimv[k].step : 0; }
#undef RAC
// ---------
// maybe remove entirely
    constexpr dim_t size() const requires (!CT) { return ra::size(*this); }
    constexpr bool empty() const requires (!CT) { return any(0==map(&Dim::len, dimv)); }
    consteval static dim_t size() requires (CT) { return std::apply([](auto ... i){ return (i.len * ... * 1); }, dimv); }
    consteval static dim_t slen() { if constexpr (CT) return size(); else return ANY; } // FIXME maybe do without
    consteval bool empty() const requires (CT) { return any(0==map(&Dim::len, dimv)); }
// ---------
// common constructors
    constexpr explicit View(P cp_): cp(cp_) {} // empty dimv, but also uninit by slicers, esp. has_len<P>
    constexpr View(View const & s) = default;
// ---------
// constructors for CT
    template <class PP> constexpr View(View<PP, Dimv> const & x) requires (CT): View(x.cp) {} // FIXME Slice
    constexpr View const &
    operator=(T (&&x)[have_braces ? slen() : 0]) const
        requires (CT && have_braces && R>1 && slen()>1)
    {
        std::ranges::copy(std::ranges::subrange(x), begin()); return *this;
    }
    constexpr View const &
    operator=(typename nested_arg<T, Dimv>::sub (&&x)[have_braces ? (R>0 ? len_s(0) : 0) : 0]) const
        requires (CT && have_braces && 0<R && 0<len_s(0) && (1!=R || 1!=len_s(0)))
    {
        ra::iter<-1>(*this) = x;
        if !consteval { asm volatile("" ::: "memory"); } // patch for [ra01]
        return *this;
    }
// ---------
// constructors for !CT
    constexpr View() requires (!CT) {} // used by Container constructors
    constexpr View(Slice auto const & x) requires (!CT): View(x.dimv, x.cp) {}
    constexpr View(auto const & s, P cp_) requires (!CT && requires { [](dim_t){}(VAL(s)); }): cp(cp_) { filldimv(ra::iter(s), dimv); }
    constexpr View(auto const & s, P cp_) requires (!CT && requires { [](Dim){}(VAL(s)); }): cp(cp_) { resize(dimv, ra::size(s)); ra::iter(dimv) = s; } // [ra37]
    template <class D> using dimbraces = std::conditional_t<R==ANY, std::initializer_list<D>, D (&&)[R==ANY?0:R]>;
    constexpr View(std::conditional_t<!CT, dimbraces<dim_t>, noarg<dim_t>> s, P cp_) requires (!CT): View(ra::iter(s), cp_) {}
    constexpr View(std::conditional_t<!CT, dimbraces<Dim>, noarg<Dim>> s, P cp_) requires (!CT): View(ra::iter(s), cp_) {}
// ---------
// row-major & nested braces for !CT
    constexpr View const & operator=(std::initializer_list<T> x) const requires (!CT && 1!=R)
    {
        RA_CK(size()==ssize(x), "Mismatched sizes ", size(), " ", ssize(x), ".");
        std::ranges::copy(std::ranges::subrange(x), begin()); return *this;
    }
    constexpr View const & operator=(braces<T, R> x) const requires (!CT && R!=ANY) { iter<-1>() = x; return *this; }
#define RA_BRACES(N)                                                    \
    constexpr View const & operator=(braces<T, N> x) const requires (!CT && R==ANY) { iter<-1>() = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES
// ---------
// CT
    template <dim_t s, dim_t o=0> constexpr auto as() const requires (CT) { return from(*this, ra::iota(ic<s>, o)); }
    template <rank_t c=0> constexpr auto iter() const requires (CT) { return Cell<P, Dimv, ic_t<c>>(cp); }
    constexpr auto iter(rank_t c) const requires (CT) { return Cell<P, decltype(dimv) const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const requires (CT) { if constexpr (c_order(dimv)) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (CT && c_order(dimv)) { return cp+size(); }
    constexpr static auto end() requires (CT && !c_order(dimv)) { return std::default_sentinel; }
    constexpr decltype(auto) back() const requires (CT) { static_assert(size()>0, "Bad back()."); return cp[size()-1]; }
// ---------
// !CT
    template <rank_t c=0> constexpr auto iter() const && requires (!CT) { return Cell<P, Dimv, ic_t<c>>(cp, std::move(dimv)); }
    template <rank_t c=0> constexpr auto iter() const & requires (!CT) { return Cell<P, Dimv const &, ic_t<c>>(cp, dimv); }
    constexpr auto iter(rank_t c) const && requires (!CT) { return Cell<P, Dimv, dim_t>(cp, std::move(dimv), c); }
    constexpr auto iter(rank_t c) const & requires (!CT) { return Cell<P, Dimv const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const requires (!CT) { return STLIterator(iter<0>()); }
    constexpr decltype(auto) static end() requires (!CT) { return std::default_sentinel; }
    constexpr decltype(auto) back() const requires (!CT) { dim_t s=size(); RA_CK(s>0, "Bad back()."); return cp[s-1]; }
// conversion to const, used by Container::view(). FIXME cf CT cases
    constexpr operator View<T const *, Dimv> const & () const requires (!CT && !std::is_const_v<T>)
    {
        return *reinterpret_cast<View<T const *, Dimv> const *>(this);
    }
// ---------
// either
// T not is_scalar [ra44]
    constexpr View const & operator=(T const & t) const { ra::iter(*this) = ra::scalar(t); return *this; }
// cf RA_ASSIGNOPS_ITER [ra38][ra34]
    View const & operator=(View const & x) const { ra::iter(*this) = x; return *this; }
#define RA_ASSIGNOPS(OP)                                                   \
    constexpr View const & operator OP(auto const & x) const { ra::iter(*this) OP x; return *this; } \
    constexpr View const & operator OP(Iterator auto && x) const { ra::iter(*this) OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return *indexer(*this, cp, ra::iter(i)); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
    constexpr auto data() const { return cp; }
};

template <class P, rank_t R> using ViewB = View<P, std::conditional_t<ANY==R, vector_default_init<Dim>, std::array<Dim, ANY==R ? 0 : R>>>;
template <class P, class Dimv> using ViewS = View<P, Dimv>;

} // namespace ra

int main()
{
    ra::TestRecorder tr;

    return tr.summary();
}
