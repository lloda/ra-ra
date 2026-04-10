// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests about Ptr

// (c) Daniel Llorens - 2025-2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#if 0==RA_CHECK
  #undef RA_CHECK
  #define RA_CHECK 1 // required by 'regression from test/checks' below
#endif

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

template <class A> concept is_ptr = requires (A a) { []<class I, class N, class S>(Ptr<Seq<I>, N, S> const &){}(a); }; // FIXME

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
    tr.section("iota simulation with iota_view");
    {
        tr.strict().test_eq(ra::iota(10, 0, 3), ra::iter(std::ranges::iota_view(0, 10))*3);
    }
    tr.section("regression from test/compatibility");
    {
        char hello[] = "hello";
        tr.test_eq(6, size_s(ra::iter(hello)));
    }
    tr.section("regression from test/stl-compat");
    {
        tr.info("adapted std::array has static size").test_eq(3, size_s(ra::iter(std::array {1, 2, 0})));
        tr.info("adapted std::array has static size").test_eq(3, ra::iter(std::array {1, 2, 0}).len_s(0));
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
        tr.test(ra::Slice<decltype(ra::iota(10))>);
    }
    return tr.summary();
}
