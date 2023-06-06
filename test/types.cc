// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Show what types conform to what type predicates.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <ranges>
#include <string>
#include <chrono>
#include <numeric>
#include <typeinfo>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using ra::mp::int_list, ra::mp::nil;

template <class A>
void
test_predicates(char const * type, TestRecorder & tr,
                bool ra, bool slice, bool iterator, bool scalar, bool foreign_vector)
{
    cout << std::string(90-size(std::string(type)), ' ') << type << " : "
         << (ra::is_ra<A> ? "ra " : "")
         << (ra::is_slice<A> ? "slice " : "")
         << (ra::IteratorConcept<A> ? "iterator " : "")
         << (ra::is_scalar<A> ? "scalar " : "")
         << (ra::is_foreign_vector<A> ? "fovector " : "")
         << (ra::is_builtin_array<A> ? "builtinarray " : "")
         << (std::ranges::range<A> ? "range " : "")
         << (std::is_const_v<A> ? "const " : "")
         << (std::is_lvalue_reference_v<A> ? "ref " : "") << endl;
    tr.quiet().info(type).info("ra").test_eq(ra, ra::is_ra<A>);
    tr.quiet().info(type).info("slice").test_eq(slice, ra::is_slice<A>);
    tr.quiet().info(type).info("Iterator").test_eq(iterator, ra::IteratorConcept<A>);
    tr.quiet().info(type).info("scalar").test_eq(scalar, ra::is_scalar<A>);
    tr.quiet().info(type).info("foreign_vector").test_eq(foreign_vector, ra::is_foreign_vector<A>);
    tr.quiet().info(type).info("std::ranges::range").test_eq(std::ranges::range<A>, std::ranges::range<A>);
}

// Not registered with is_scalar_def.
struct Unreg { int x; };

// prefer decltype(declval(...)) to https://stackoverflow.com/a/13842784 the latter drops const
#define TESTPRED(A, ...) test_predicates <A> (STRINGIZE(A), tr, ##__VA_ARGS__)

int main()
{
    TestRecorder tr(std::cout, TestRecorder::NOISY);
    {
        using T = std::chrono::duration<long int, std::ratio<1, 1000000000>>;
        ra::Big<T, 1> a;
        static_assert(std::is_same_v<ra::value_t<decltype(a)>, T>);
    }
    {
// SmallBase is not excluded in is_slice.
        TESTPRED(decltype(std::declval<ra::SmallBase<ra::SmallArray, ra::Dim, int_list<3>, int_list<1>>>()),
                 true, true, false, false, false);
        TESTPRED(int,
                 false, false, false, true, false);
        TESTPRED(std::complex<double>,
                 false, false, false, true, false);
        TESTPRED(decltype(std::declval<ra::Unique<int, 2>>()),
                 true, true, false, false, false);
        TESTPRED(decltype(std::declval<ra::View<int, 2>>()),
                 true, true, false, false, false);
        TESTPRED(decltype(ra::Unique<int, 2>().iter()),
                 true, false, true, false, false);
        static_assert(ra::IteratorConcept<decltype(ra::Unique<int, 2>().iter())>);
        TESTPRED(decltype(ra::Unique<int, 1>().iter()) &,
                 true, false, true, false, false);
        TESTPRED(decltype(ra::iota(5)),
                 true, false, true, false, false);
        TESTPRED(ra::TensorIndex<0>,
                 true, false, true, false, false);
        TESTPRED(ra::TensorIndex<0> const, // is_iterator but not IteratorConcept (see RA_IS_DEF)
                 true, false, false, false, false);
        TESTPRED(ra::TensorIndex<0> &,
                 true, false, true, false, false);
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
        TESTPRED(decltype(ra::start(std::vector<int> {})),
                 true, false, true, false, false);
        TESTPRED(int *,
                 false, false, false, false, false);
        TESTPRED(decltype(std::ranges::iota_view(-5, 10)),
                 false, false, false, false, true);
// std::string can be registered as is_scalar or not [ra13]. One may do ra::vector(std::string) or ra::scalar(std::string) to get the other behavior.
        if constexpr(ra::is_scalar<std::string>) {
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
        static_assert(ra::is_slice<ra::View<int, 0>>);
        static_assert(ra::is_slice<ra::View<int, 2>>);
        static_assert(ra::is_slice<ra::SmallView<int, int_list<>, int_list<>>>);

        static_assert(ra::is_ra<ra::Small<int>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallView<int, nil, nil>>, "bad is_ra SmallView");
        static_assert(ra::is_ra<ra::Unique<int, 0>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::View<int, 0>>, "bad is_ra View");

        static_assert(ra::is_ra<ra::Small<int, 1>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallView<int, int_list<1>, int_list<1>>>, "bad is_ra SmallView");
        static_assert(ra::is_ra<ra::Unique<int, 1>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::View<int, 1>>, "bad is_ra View");
        static_assert(ra::is_ra<ra::View<int>>, "bad is_ra View");

        using Scalar = decltype(ra::scalar(3));
        using Vector = decltype(ra::start({1, 2, 3}));
        static_assert(ra::is_ra_scalar<Scalar>, "bad is_ra_scalar Scalar");
        static_assert(ra::is_ra<decltype(ra::scalar(3))>, "bad is_ra Scalar");
        static_assert(ra::is_ra<Vector>, "bad is_ra Vector");
        static_assert(!ra::is_ra<int *>, "bad is_ra int *");

        static_assert(ra::is_scalar<double>, "bad is_scalar real");
        static_assert(ra::is_scalar<std::complex<double>>, "bad is_scalar complex");
        static_assert(ra::is_scalar<int>, "bad is_scalar int");

        static_assert(!ra::is_scalar<decltype(ra::scalar(3))>, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<Vector>, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<decltype(ra::start(3))>, "bad is_scalar Scalar");
        int a = 3;
        static_assert(!ra::is_scalar<decltype(ra::start(a))>, "bad is_scalar Scalar");
// a regression.
        static_assert(ra::is_ra_zero_rank<ra::Scalar<int>>, "bad");
        static_assert(!ra::is_ra_pos_rank<ra::Scalar<int>>, "bad");
        static_assert(!ra::is_ra_zero_rank<ra::TensorIndex<0>>, "bad");
        static_assert(ra::is_ra_pos_rank<ra::TensorIndex<0>>, "bad");
        static_assert(!ra::ra_zero<ra::TensorIndex<0>>, "bad");
        static_assert(ra::is_ra_pos_rank<ra::Expr<ra::plus, std::tuple<ra::TensorIndex<0>, ra::Scalar<int>>>>, "bad");
        static_assert(ra::is_ra_pos_rank<ra::Pick<std::tuple<Vector, Vector, Vector>>>, "bad");
    }
    tr.section("builtin arrays I");
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
        static_assert(requires { ra::ra_traits<decltype(a)>::size(a); });
        static_assert(requires { ra::ra_traits<decltype(b)>::size(b); });
        static_assert(requires { ra::ra_traits<decltype(c)>::size(c); });
        tr.test_eq(1, ra::rank_s(a));
        tr.test_eq(1, ra::rank_s(b));
        tr.test_eq(2, ra::rank_s(c));
        tr.test_eq(3, ra::size(a));
        tr.test_eq(3, ra::size(b));
        tr.test_eq(4, ra::size(c));
        tr.test_eq(3, ra::size_s(a));
        tr.test_eq(3, ra::size_s(b));
        tr.test_eq(4, ra::size_s(c));
        tr.test_eq(3, ra::ra_traits<decltype(a)>::size_s());
        tr.test_eq(3, ra::ra_traits<decltype(b)>::size_s());
        tr.test_eq(4, ra::ra_traits<decltype(c)>::size_s());
    }
    tr.section("adaptors I");
    {
        tr.test_eq(2, size_s(ra::start(std::array<int, 2> { 1, 2 })));
        tr.test_eq(2, ra::size_s(std::array<int, 2> { 1, 2 }));
        tr.test_eq(ra::DIM_ANY, size_s(ra::start(std::vector<int> { 1, 2, 3})));
        tr.test_eq(ra::DIM_ANY, ra::size_s(std::vector<int> { 1, 2, 3}));
        tr.test_eq(2, ra::start(std::array<int, 2> { 1, 2 }).len_s(0));
        tr.test_eq(ra::DIM_ANY, ra::start(std::vector<int> { 1, 2, 3 }).len_s(0));
        tr.test_eq(1, ra::start(std::array<int, 2> { 1, 2 }).rank_s());
        tr.test_eq(1, ra::start(std::vector<int> { 1, 2, 3 }).rank_s());
        tr.test_eq(1, ra::start(std::array<int, 2> { 1, 2 }).rank());
        tr.test_eq(1, ra::start(std::vector<int> { 1, 2, 3 }).rank());
    }
    tr.section("adaptors II");
    {
        static_assert(ra::is_iterator<decltype(ra::vector(std::array { 1, 2 }))>);
        static_assert(ra::is_iterator<decltype(ra::start(std::array { 1, 2 }))>);
    }
    return tr.summary();
}
