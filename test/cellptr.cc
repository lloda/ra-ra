// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Replacement sandbox of Oldptr (old ptr/iota) by Cell (current)

// (c) Daniel Llorens - 2025-2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/base.hh"
#include <exception>


// -------------------------------------
// bit from example/throw.cc which FIXME maybe a prepared option.

struct ra_error: public std::exception
{
    std::string s;
    template <class ... A> ra_error(A && ... a): s(ra::format(std::forward<A>(a) ...)) {}
    virtual char const * what() const throw () { return s.c_str(); }
};

#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) [[unlikely]] throw ra_error("ra::", std::source_location::current(), " (" RA_STRINGIZE(cond) ") " __VA_OPT__(,) __VA_ARGS__); }
// -------------------------------------

#include "ra/test.hh"
#include "test/mpdebug.hh"

using std::cout, std::endl, std::flush;

namespace ra {

template <class P, class N, class S>
struct Oldptr final
{
    static_assert(has_len<P> || std::bidirectional_iterator<P>);
    static_assert(has_len<N> || is_ctype<N> || std::is_integral_v<N>);
    static_assert(has_len<S> || is_ctype<S> || std::is_integral_v<S>);
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(0<=nn || ANY==nn || UNB==nn);

    P cp;
    [[no_unique_address]] struct { [[no_unique_address]] N const len = {}; [[no_unique_address]] S const step = {}; } dimv[1];
    constexpr Oldptr(P p, N n, S s): cp(p), dimv { n, s } {}
    RA_ASSIGNOPS_ITER(Oldptr)
    template <rank_t c=0> constexpr auto iter() const { static_assert(0==c); return *this; }
    constexpr auto data() const { return cp; }
    consteval static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { return nn; }
    constexpr static dim_t len(int k) requires (is_ctype<N>) { return nn; }
    constexpr dim_t len(int k) const requires (!is_ctype<N>) { return dimv[0].len; }
    constexpr static dim_t step(int k) requires (is_ctype<S>) { return 0==k ? S {} : 0; }
    constexpr dim_t step(int k) const requires (!is_ctype<S>) { return 0==k ? dimv[0].step : 0; }
    constexpr static bool keep(dim_t st, int z, int j) requires (is_ctype<S>) { return st*step(z)==step(j); }
    constexpr bool keep(dim_t st, int z, int j) const requires (!is_ctype<S>) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { mov(step(k)*d); }
    constexpr decltype(*cp) at(auto const & i) const { return *indexer(*this, cp, ra::iter(i)); }
    constexpr decltype(*cp) operator*() const { return *cp; }
    constexpr auto save() const { return cp; }
    constexpr void load(P p) { cp=p; }
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Waggressive-loop-optimizations" // Seq<!=dim_t> in gcc14/15 -O3
    constexpr void mov(dim_t d) { std::ranges::advance(cp, d); }
#pragma GCC diagnostic pop
};

template <class P, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
oldptr(P && p, N && n=N {}, S && s=S(maybe_step<S>))
{
    if constexpr (std::ranges::bidirectional_range<std::remove_reference_t<P>>) {
        static_assert(std::is_same_v<ic_t<dim_t(UNB)>, N>, "Object has own length.");
        static_assert(std::is_same_v<ic_t<dim_t(1)>, S>, "No step with deduced size.");
        if constexpr (ANY==size_s(p)) {
            return oldptr(std::begin(RA_FW(p)), std::ssize(p), RA_FW(s));
        } else {
            return oldptr(std::begin(RA_FW(p)), ic<size_s(p)>, RA_FW(s));
        }
    } else {
        if constexpr (std::is_integral_v<N>) { RA_CK(n>=0, "Bad Oldptr length ", n, "."); }
        return Oldptr<std::decay_t<P>, sarg<N>, sarg<S>> { p, RA_FW(n), RA_FW(s) };
    }
}

template <class P, class N, class S> requires (has_len<P> || has_len<N> || has_len<S>)
struct WLen<Oldptr<P, N, S>>
{
    constexpr static auto
    f(auto ln, auto && e) { return oldptr(wlen(ln, e.data()), VAL(wlen(ln, e.dimv[0].len)), VAL(wlen(ln, e.dimv[0].step))); }
};

template <class A> concept is_oldptr = requires (A a) { []<class I, class N, class S>(Oldptr<Seq<I>, N, S> const &){}(a); };

template <class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota1(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    return Oldptr<Seq<sarg<I>>, sarg<N>, sarg<S>> { {RA_FW(i)}, RA_FW(n), RA_FW(s) };
}

template <class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota2(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    return oldptr(Seq<sarg<I>>(RA_FW(i)), RA_FW(n), RA_FW(s));
}

template <class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota3(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    return ptr(Seq<sarg<I>>(RA_FW(i)), RA_FW(n), RA_FW(s));
}

} // namespace ra

int main()
{
    ra::TestRecorder tr;
    tr.section("dim/1");
    {
        using N = ra::dim_t;
        using S = ra::ic_t<1>;
        using P = ra::Seq<ra::dim_t>;
        ra::Ptr<P, N, S> cptr(P {0}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> { .len=10 }});
        static_assert(1==cptr.step(ra::ic<0>));
        static_assert(1==decltype(cptr)::step(ra::ic<0>));
        static_assert(1==decltype(cptr)::step(0));
        static_assert(ra::ANY==decltype(cptr)::len_s(0));
        tr.test_eq(10, cptr.len(0));
        cout << sizeof(cptr) << endl;
        cout << cptr << endl;
        static_assert(ra::is_ptr<decltype(cptr)>);
    }
    tr.section("4/1");
    {
        using N = ra::ic_t<4>;
        using S = ra::ic_t<2>;
        using P = ra::Seq<ra::dim_t>;
        ra::Ptr<P, N, S> cptr(P {-9}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> {}});
        static_assert(2==cptr.step(0));
        static_assert(4==cptr.len(0));
        tr.test_eq(2, decltype(cptr)::step(0));
        tr.test_eq(4, decltype(cptr)::len(0));
        cout << sizeof(cptr) << endl;
        cout << cptr << endl;
        static_assert(ra::is_ptr<decltype(cptr)>);
    }
    tr.section("len/1");
    {
        using N = decltype(ra::len);
        using S = ra::ic_t<1>;
        using P = ra::Seq<ra::dim_t>;
        ra::Ptr<P, N, S> cptr(P {0}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> {}});
        static_assert(1==cptr.step(0));
        tr.test_eq(1, decltype(cptr)::step(0));
        cout << sizeof(cptr.dimv) << endl;
        cout << sizeof(cptr.c) << endl;
        cout << sizeof(cptr) << endl;
        static_assert(ra::is_ptr<decltype(cptr)>);
    }
    tr.section("misc");
    {
        cout << sizeof(ra::ViewBig<int *, 0>) << endl;
    }
    tr.section("lens");
    {
        cout << ra::mp::type_name<decltype(ra::map(std::plus<>(), ra::iota(ra::ic<4>), ra::iota(4, 0, 2)).len(ra::ic<0>))>() << endl;
// this is static 4 even though runtime check is still needed.
        cout << ra::mp::type_name<decltype(ra::clen(ra::map(std::plus<>(), ra::iota(ra::ic<4>), ra::iota(5, 0, 2)), ra::ic<0>))>() << endl;
    }
    tr.section("len in ptr (ptr)");
    {
        int aa[] = { 1, 2, 3, 4, 5, 6 };
        int * a = aa;
        static_assert(ra::has_len<decltype(ra::ptr(a, ra::len))>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::ptr(a, ra::len)).dimv[0].len)>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::ptr(a, ra::len-1)).dimv[0].len)>);
        tr.test_eq(ra::ptr(a, 5), wlen(5, ra::ptr(a, ra::len)));
        tr.info("len in step argument").test_eq(ra::ptr(a, 3)*2, wlen(6, ra::ptr(a+1, ra::len/2, ra::len/3)));
    }
    tr.section("iota simulation with ptr(iota_view)");
    {
        tr.strict().test_eq(ra::iota(10, 0, 3), ra::ptr(std::ranges::iota_view(0, 10))*3);
    }
    tr.section("regression from test/compatibility");
    {
        char hello[] = "hello";
        tr.test_eq(6, size_s(ra::iter(hello)));
        tr.test_eq(6, size_s(ra::ptr(hello)));
    }
    tr.section("what about iota");
    {
        std::cout << ra::mp::type_name<decltype(ra::iota1(ra::len, ra::len*0))>() << std::endl;
        std::cout << ra::mp::type_name<decltype(ra::iota2(ra::len, ra::len*0))>() << std::endl;
    }
    tr.section("regression from test/stl-compat");
    {
        tr.info("adapted std::array has static size").test_eq(3, size_s(ra::ptr(std::array {1, 2, 0})));
        tr.info("adapted std::array has static size").test_eq(3, ra::ptr(std::array {1, 2, 0}).len_s(0));
    }
    tr.section("regression from test/checks");
    {
        int x = 0;
        try {
            int p[] = {10, 20, 30};
            tr.test_eq(-1, ra::ptr((int *)p, -1));
        } catch (ra_error & e) {
            x = 1;
        }
        tr.info("ptr len").test_eq(x, 1);
    }
    tr.section("iota must be slice");
    {
        tr.test(ra::Slice<decltype(ra::iota1(10))>);
        tr.test(ra::Slice<decltype(ra::iota2(10))>);
        tr.test(ra::Slice<decltype(ra::iota3(10))>);
    }
    return tr.summary();
}
