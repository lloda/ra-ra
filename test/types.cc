// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Show what types conform to what type predicates.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <chrono>
#include <numeric>
#include <typeinfo>
#include <iostream>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using ra::ilist_t, ra::mp::nil;

template <class A>
void
test_predicates(char const * type, TestRecorder & tr,
                std::optional<bool> ra,
                std::optional<bool> slice,
                std::optional<bool> iterator,
                std::optional<bool> scalar,
                std::optional<bool> fov)
{
    constexpr int width = 70;
    std::string s = type;
    std::string prefix = "decltype(std::declval<";
    std::string suffix = ">())";
    if (0==s.find(prefix) && (s.size()-size(suffix)==s.rfind(suffix))) {
        s = s.substr(size(prefix), s.size()-size(prefix)-size(suffix));
    }
    suffix = ">()) const";
    if (0==s.find(prefix) && (s.size()-size(suffix)==s.rfind(suffix))) {
        s = s.substr(size(prefix), s.size()-size(prefix)-size(suffix)) + " const";
    }
    suffix = ">()) &";
    if (0==s.find(prefix) && (s.size()-size(suffix)==s.rfind(suffix))) {
        s = s.substr(size(prefix), s.size()-size(prefix)-size(suffix)) + " &";
    }
    if (size(s)>width) { s = s.substr(0, width-size(std::string(" …"))) + " …"; }
    cout << std::string(width-size(s), ' ') << s << " : "
         << (ra::is_ra<A> ? "ra " : "")
         << (ra::Slice<A> ? "slice " : "")
         << (ra::Iterator<A> ? "iterator " : "")
         << (ra::is_scalar<A> ? "scalar " : "")
         << (ra::is_fov<A> ? "fov " : "")
         << (ra::is_builtin_array<A> ? "builtinarray " : "")
         << (std::ranges::range<A> ? "range " : "")
         << (std::is_const_v<A> ? "const " : "")
         << (std::is_lvalue_reference_v<A> ? "ref " : "") << endl;
    if (ra) tr.quiet().info(type).info("ra").test_eq(ra.value(), ra::is_ra<A>);
    if (slice) tr.quiet().info(type).info("slice").test_eq(slice.value(), ra::Slice<A>);
    if (iterator) tr.quiet().info(type).info("Iterator").test_eq(iterator.value(), ra::Iterator<A>);
    if (scalar) tr.quiet().info(type).info("scalar").test_eq(scalar.value(), ra::is_scalar<A>);
    if (fov) tr.quiet().info(type).info("fov").test_eq(fov.value(), ra::is_fov<A>);
    tr.quiet().info(type).info("std::ranges::range").test_eq(std::ranges::range<A>, std::ranges::range<A>);
}

// Not registered with is_scalar_def.
struct Unreg { int x; };

// prefer decltype(declval(...)) to https://stackoverflow.com/a/13842784 the latter drops const
#define TESTPRED(A, ...) test_predicates <A> (RA_STRINGIZE(A), tr __VA_OPT__(,) __VA_ARGS__)

int main()
{
    TestRecorder tr(std::cout, TestRecorder::NOISY);
    {
        using T = std::chrono::duration<long int, std::ratio<1, 1000000000>>;
        ra::Big<T, 1> a;
        static_assert(std::is_same_v<ra::value_t<decltype(a)>, T>);
        ra::Small<T const, 1> ac;
        static_assert(std::is_same_v<ra::value_t<decltype(ac)>, T const>);
        static_assert(std::is_same_v<ra::ncvalue_t<decltype(ac)>, T>);
        ra::Small<T, 1> an;
        static_assert(std::is_same_v<ra::value_t<decltype(an)>, T>);
        static_assert(std::is_same_v<ra::ncvalue_t<decltype(an)>, T>);
    }
    {
        TESTPRED(decltype(std::declval<ra::ViewSmall<ra::Dim *, ra::ic_t<std::array { ra::Dim {3, 1} }>>>()),
                 true, true, false, false, false);
        TESTPRED(int,
                 false, false, false, true, false);
        TESTPRED(ra::int_c<3>,
                 false, false, false, true, false);
        TESTPRED(ra::dim_c<0>,
                 false, false, false, true, false);
        TESTPRED(std::complex<double>,
                 false, false, false, true, false);
#if RA_FLOAT128
        TESTPRED(std::float128_t,
                 false, false, false, true, false);
        TESTPRED(std::complex<std::float128_t>,
                 false, false, false, true, false);
#endif // RA_FLOAT128
        TESTPRED(decltype(std::declval<ra::Unique<int, 2>>()),
                 true, true, false, false, false);
        TESTPRED(decltype(std::declval<ra::Unique<int, 2>>()) const,
                 true, true, false, false, false);
        TESTPRED(decltype(std::declval<ra::Unique<int, 2>>()) &,
                 true, true, false, false, false);
        TESTPRED(decltype(std::declval<ra::ViewBig<int *, 2>>()),
                 true, true, false, false, false);
        TESTPRED(decltype(ra::Unique<int, 2>().iter()),
                 true, false, true, false, false);
        static_assert(ra::Iterator<decltype(ra::Unique<int, 2>().iter())>);
// this iter is Ptr, but it'll be used as iter and we don't care if it's also slice.
        TESTPRED(decltype(ra::Unique<int, 1>().iter()) &,
                 true, std::nullopt, true, false, false);
        TESTPRED(decltype(ra::iota(5)),
                 true, true, true, false, false);
        TESTPRED(decltype(ra::iota<0>()),
                 true, true, true, false, false);
// is_iterator by RA_IS_DEF, but not Iterator, since it cannot be traversed.
        TESTPRED(decltype(ra::iota<0>()) const,
                 true, true, false, false, false);
        TESTPRED(decltype(ra::iota<0>()) &,
                 true, true, true, false, false);
        TESTPRED(decltype(std::declval<ra::Small<int, 2>>()),
                 true, true, false, false, false);
        TESTPRED(decltype(ra::Small<int, 2>().iter()),
                 true, false, true, false, false);
        TESTPRED(decltype(ra::Small<int, 2, 2>()()),
                 true, true, false, false, false);
        TESTPRED(decltype(ra::Small<int, 2, 2>().iter()),
                 true, false, true, false, false);
        TESTPRED(decltype(ra::Small<int, 2>()+3),
                 true, false, true, false, false);
        TESTPRED(decltype(3+ra::Big<int>()),
                 true, false, true, false, false);
        TESTPRED(std::vector<int>,
                 false, false, false, false, true);
// this iter is Ptr, but it'll be used as iter and we don't care if it's also slice.
        TESTPRED(decltype(ra::iter(std::vector<int> {})),
                 true, std::nullopt, true, false, false);
        TESTPRED(int *,
                 false, false, false, false, false);
        TESTPRED(decltype(std::ranges::iota_view(-5, 10)),
                 false, false, false, false, true);
// std::string can be registered as is_scalar or not [ra13]. Use ra::ptr(std::string) or ra::scalar(std::string) to get the other behavior.
        if constexpr (ra::is_scalar<std::string>) {
            TESTPRED(std::string,
                     false, false, false, true, false);
        } else {
            TESTPRED(std::string,
                     false, false, false, false, true);
        }
        TESTPRED(Unreg,
                 false, false, false, false, false);
        TESTPRED(int [4],
                 false, false, false, false, false);
        TESTPRED(int (&) [3],
                 false, false, false, false, false);
        TESTPRED(decltype("cstring"),
                 false, false, false, false, false);
    }
    tr.section("establish meaning of selectors (TODO / convert to TestRecorder)");
    {
        static_assert(ra::Slice<ra::ViewBig<int *, 0>>);
        static_assert(ra::Slice<ra::ViewBig<int *, 2>>);
        static_assert(ra::Slice<ra::ViewSmall<int *, ra::ic_t<std::array<ra::Dim, 0> {} >>>);
        static_assert(ra::is_ra<ra::Small<int>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::ViewSmall<int *, ra::ic_t<std::array<ra::Dim, 0> {} >>>, "bad is_ra ViewSmall");
        static_assert(ra::is_ra<ra::Unique<int, 0>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::ViewBig<int *, 0>>, "bad is_ra View");

        static_assert(ra::Slice<ra::Small<int, 1>>);
        static_assert(ra::Slice<ra::Big<int, 2>>);
        static_assert(ra::is_ra<ra::Small<int, 1>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::ViewSmall<int *, ra::ic_t<std::array {ra::Dim {1, 1}} >>>, "bad is_ra ViewSmall");
        static_assert(ra::is_ra<ra::Unique<int, 1>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::ViewBig<int *, 1>>, "bad is_ra View");
        static_assert(ra::is_ra<ra::ViewBig<int *>>, "bad is_ra View");

        using Vector = decltype(ra::iter({1, 2, 3}));
        static_assert(ra::is_ra<decltype(ra::scalar(3))>, "bad is_ra Scalar");
        static_assert(ra::is_ra<Vector>, "bad is_ra Vector");
        static_assert(!ra::is_ra<int *>, "bad is_ra int *");

        static_assert(ra::is_scalar<double>, "bad is_scalar real");
        static_assert(ra::is_scalar<std::complex<double>>, "bad is_scalar complex");
        static_assert(ra::is_scalar<int>, "bad is_scalar int");

        static_assert(!ra::is_scalar<decltype(ra::scalar(3))>, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<Vector>, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<decltype(ra::iter(3))>, "bad is_scalar Scalar");
        int a = 3;
        static_assert(!ra::is_scalar<decltype(ra::iter(a))>, "bad is_scalar Scalar");
// a regression.
        static_assert(ra::is_ra_0<ra::Scalar<int>>, "bad");
        static_assert(!ra::is_ra_pos<ra::Scalar<int>>, "bad");
        static_assert(!ra::is_ra_0<decltype(ra::iota<0>())>, "bad");
        static_assert(ra::is_ra_pos<decltype(ra::iota<0>())>, "bad");
        static_assert(ra::is_ra_pos<ra::Map<std::multiplies<>, std::tuple<decltype(ra::iota<0>()), ra::Scalar<int>>>>, "bad");
        static_assert(ra::is_ra_pos<ra::Pick<std::tuple<Vector, Vector, Vector>>>, "bad");
    }
    tr.section("builtin arrays");
    {
        int const a[] = {1, 2, 3};
        int b[] = {1, 2, 3};
        int c[][2] = {{1, 2}, {4, 5}};
        static_assert(ra::is_builtin_array<decltype(a) &>);
        static_assert(ra::is_builtin_array<decltype(a)>);
        static_assert(ra::is_builtin_array<decltype(b) &>);
        static_assert(ra::is_builtin_array<decltype(b)>);
        static_assert(ra::is_builtin_array<decltype(c) &>);
        static_assert(ra::is_builtin_array<decltype(c)>);
        tr.test_eq(1, ra::rank_s(a));
        tr.test_eq(1, ra::rank_s(b));
        tr.test_eq(2, ra::rank_s(c));
        tr.test_eq(3, ra::size(a));
        tr.test_eq(3, ra::size(b));
        tr.test_eq(4, ra::size(c));
        tr.test_eq(3, ra::size_s(a));
        tr.test_eq(3, ra::size_s(b));
        tr.test_eq(4, ra::size_s(c));
    }
    tr.section("adaptors I");
    {
        tr.test_eq(2, size_s(ra::iter(std::array<int, 2> { 1, 2 })));
        tr.test_eq(2, ra::size_s(std::array<int, 2> { 1, 2 }));
        tr.test_eq(ra::ANY, ra::size_s(ra::iter(std::vector<int> { 1, 2, 3})));
        tr.test_eq(ra::ANY, ra::size_s(std::vector<int> { 1, 2, 3}));
        tr.test_eq(2, ra::iter(std::array<int, 2> { 1, 2 }).len_s(0));
        tr.test_eq(ra::ANY, ra::iter(std::vector<int> { 1, 2, 3 }).len_s(0));
        tr.test_eq(1, ra::rank_s(ra::iter(std::array<int, 2> { 1, 2 })));
        tr.test_eq(1, ra::rank_s(ra::iter(std::vector<int> { 1, 2, 3 })));
        tr.test_eq(1, ra::iter(std::array<int, 2> { 1, 2 }).rank());
        tr.test_eq(1, ra::iter(std::vector<int> { 1, 2, 3 }).rank());
    }
    tr.section("adaptors II");
    {
        static_assert(ra::is_iterator<decltype(ra::ptr(std::array { 1, 2 }))>);
        static_assert(ra::is_iterator<decltype(ra::iter(std::array { 1, 2 }))>);
    }
    tr.section("compatibility");
    {
        static_assert(std::input_iterator<decltype(ra::Big<int, 2> {{1,2},{3,4}}.begin())>);
// would be if (unusable with I=Seq) Ptr::operator= and constructor were removed
        // static_assert(std::is_aggregate_v<ra::Ptr<Seq<ra::dim_t>, ra::dim_t, ra::dim_t>>);
        static_assert(std::random_access_iterator<ra::Seq<ra::dim_t>>);
        cout << ra::ptr(ra::Seq {0}, 10) << endl;
        cout << ra::ptr(ra::Seq {3.}, 8) << endl;
    }
    tr.section("mixed float128/double ops");
    {
#if RA_FLOAT128
        tr.test_eq(9., real(std::complex<std::float128_t>(3.) * 3.));
        tr.test_eq(9., real(std::complex<double>(3.) * std::float128_t(3.)));
#else
        std::println(std::cout, "no float128_t support!");
#endif // RA_FLOAT128
    }
    return tr.summary();
}
