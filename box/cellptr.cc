// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Special Dimv to let Cell<> replace Ptr

// (c) Daniel Llorens - 2025
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

// FIXME iota() was Slice before through Ptr, but now it's not because Cell isn't.
// Either iota() should be View not Cell, or Cell should be Slice. Hmm.

template <class N, class S> struct SDim { [[no_unique_address]] N const len = {}; [[no_unique_address]] S const step = {}; };
template <class P, class N, class S> using CellPtr = Cell<P, std::array<SDim<N, S>, 1>, ic_t<0>>;

template <class P, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
cellptr(P && p, N && n=N {}, S && s=S(maybe_step<S>))
{
    if constexpr (std::ranges::bidirectional_range<std::remove_reference_t<P>>) {
        static_assert(std::is_same_v<ic_t<dim_t(UNB)>, N>, "Object has own length.");
        static_assert(std::is_same_v<ic_t<dim_t(1)>, S>, "No step with deduced size.");
        if constexpr (ANY==size_s(p)) {
            return ptr(std::begin(RA_FW(p)), std::ssize(p), RA_FW(s));
        } else {
            return ptr(std::begin(RA_FW(p)), ic<size_s(p)>, RA_FW(s));
        }
    } else {
        if constexpr (std::is_integral_v<N>) { RA_CK(n>=0, "Bad Ptr length ", n, "."); }
        using Dim = ra::SDim<sarg<N>, sarg<S>>;
        return CellPtr<std::decay_t<P>, sarg<N>, sarg<S>> { p, {Dim {.len=RA_FW(n), .step=RA_FW(s) }}};
    }
}

template <class P, class N, class S> requires (has_len<P> || has_len<N> || has_len<S>)
struct WLen<CellPtr<P, N, S>>
{
    constexpr static auto
    f(auto ln, auto && e) { return cellptr(wlen(ln, e.c.cp), VAL(wlen(ln, e.dimv[0].len)), VAL(wlen(ln, e.dimv[0].step))); }
};

template <class A> concept is_cellptr = requires (A a) { []<class I, class N, class S>(CellPtr<Seq<I>, N, S> const &){}(a); };

template <class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota1(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    return Ptr<Seq<sarg<I>>, sarg<N>, sarg<S>> { {RA_FW(i)}, RA_FW(n), RA_FW(s) };
}

template <class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota2(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    return ptr(Seq<sarg<I>>(RA_FW(i)), RA_FW(n), RA_FW(s));
}

template <class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota3(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    return cellptr(Seq<sarg<I>>(RA_FW(i)), RA_FW(n), RA_FW(s));
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
        ra::CellPtr<P, N, S> cptr(P {0}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> { .len=10 }});
        static_assert(1==cptr.step(ra::ic<0>));
        static_assert(1==decltype(cptr)::step(ra::ic<0>));
        static_assert(1==decltype(cptr)::step(0));
        static_assert(ra::ANY==decltype(cptr)::len_s(0));
        tr.test_eq(10, cptr.len(0));
        cout << sizeof(cptr) << endl;
        cout << cptr << endl;
        static_assert(ra::is_cellptr<decltype(cptr)>);
    }
    tr.section("4/1");
    {
        using N = ra::ic_t<4>;
        using S = ra::ic_t<2>;
        using P = ra::Seq<ra::dim_t>;
        ra::CellPtr<P, N, S> cptr(P {-9}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> {}});
        static_assert(2==cptr.step(0));
        static_assert(4==cptr.len(0));
        tr.test_eq(2, decltype(cptr)::step(0));
        tr.test_eq(4, decltype(cptr)::len(0));
        cout << sizeof(cptr) << endl;
        cout << cptr << endl;
        static_assert(ra::is_cellptr<decltype(cptr)>);
    }
    tr.section("len/1");
    {
        using N = decltype(ra::len);
        using S = ra::ic_t<1>;
        using P = ra::Seq<ra::dim_t>;
        ra::CellPtr<P, N, S> cptr(P {0}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> {}});
        static_assert(1==cptr.step(0));
        tr.test_eq(1, decltype(cptr)::step(0));
        cout << sizeof(cptr.dimv) << endl;
        cout << sizeof(cptr.c) << endl;
        cout << sizeof(cptr) << endl;
        static_assert(ra::is_cellptr<decltype(cptr)>);
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
    tr.section("len in ptr (cellptr)");
    {
        int aa[] = { 1, 2, 3, 4, 5, 6 };
        int * a = aa;
        static_assert(ra::has_len<decltype(ra::cellptr(a, ra::len))>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::cellptr(a, ra::len)).dimv[0].len)>);
        static_assert(std::is_integral_v<decltype(wlen(5, ra::cellptr(a, ra::len-1)).dimv[0].len)>);
        tr.test_eq(ra::cellptr(a, 5), wlen(5, ra::cellptr(a, ra::len)));
        tr.info("len in step argument").test_eq(ra::cellptr(a, 3)*2, wlen(6, ra::cellptr(a+1, ra::len/2, ra::len/3)));
    }
    tr.section("iota simulation with cellptr(iota_view)");
    {
        tr.strict().test_eq(ra::iota(10, 0, 3), ra::cellptr(std::ranges::iota_view(0, 10))*3);
    }
    tr.section("regression from test/compatibility");
    {
        char hello[] = "hello";
        tr.test_eq(6, size_s(ra::iter(hello)));
        tr.test_eq(6, size_s(ra::cellptr(hello)));
    }
    tr.section("what about iota");
    {
        std::cout << ra::mp::type_name<decltype(ra::iota1(ra::len, ra::len*0))>() << std::endl;
        std::cout << ra::mp::type_name<decltype(ra::iota2(ra::len, ra::len*0))>() << std::endl;
    }
    tr.section("regression from test/stl-compat");
    {
        tr.info("adapted std::array has static size").test_eq(3, size_s(ra::cellptr(std::array {1, 2, 0})));
        tr.info("adapted std::array has static size").test_eq(3, ra::cellptr(std::array {1, 2, 0}).len_s(0));
    }
    tr.section("regression from test/checks");
    {
        int x = 0;
        try {
            int p[] = {10, 20, 30};
            tr.test_eq(-1, ra::cellptr((int *)p, -1));
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
// 1st goes to constexpr SmallArray(T const & t) { std::ranges::fill(cp, t); }, the other idk :-/
    tr.section("regression from test/big-0");
    {
        auto x = ra::iota1(3, 1);
        cout << "conv1 " << std::is_convertible_v<decltype(x), int> << endl;
        ra::Small<ra::Big<int, 1>, 1> f = { {x} }; // ok; broken with { x }
        tr.test_eq(ra::iota(3, 1), f(0));
    }
    {
        auto x = ra::iota3(3, 1);
        cout << "conv1 " << std::is_convertible_v<decltype(x), int> << endl;
        ra::Small<ra::Big<int, 1>, 1> f = { {x} }; // ok; broken with { x }
        tr.test_eq(ra::iota(3, 1), f(0));
    }
    return tr.summary();
}
