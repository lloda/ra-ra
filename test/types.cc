// -*- mode: c++; coding: utf-8 -*-
/// @file types.cc
/// @brief Show what types conform to what type predicates.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <string>
#include <chrono>
#include <numeric>
#include <typeinfo>
#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

#define TEST_PREDICATES(A)                                              \
    [&tr](bool ra, bool slice, bool array_iterator, bool scalar, bool foreign_vector) \
    {                                                                   \
        tr.info(STRINGIZE(A)).info("ra").test_eq(ra, ra::is_ra<A>);     \
        tr.info(STRINGIZE(A)).info("slice").test_eq(slice, ra::is_slice<A>); \
        tr.info(STRINGIZE(A)).info("RaIterator").test_eq(array_iterator, ra::RaIterator<A>); \
        tr.info(STRINGIZE(A)).info("scalar").test_eq(scalar, ra::is_scalar<A>); \
        tr.info(STRINGIZE(A)).info("foreign_vector").test_eq(foreign_vector, ra::is_foreign_vector<A>); \
    }

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
        TEST_PREDICATES(decltype(std::declval<ra::SmallBase<ra::SmallArray, ra::Dim, mp::int_list<3>, mp::int_list<1>>>()))
            (true, true, false, false, false);
        TEST_PREDICATES(int)
            (false, false, false, true, false);
        TEST_PREDICATES(std::complex<double>)
            (false, false, false, true, false);
        TEST_PREDICATES(decltype(ra::Unique<int, 2>()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::View<int, 2>()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::Unique<int, 2>().iter()))
            (true, false, true, false, false);
        static_assert(ra::RaIterator<decltype(ra::Unique<int, 2>().iter())>);
        {
            ra::Unique<int, 1> A= {1, 2, 3};
            auto i = A.iter();
            auto & ii = i;
            TEST_PREDICATES(decltype(ii))
                (true, false, true, false, false);
        }
        TEST_PREDICATES(ra::Iota<int>)
            (true, false, true, false, false);
        TEST_PREDICATES(ra::TensorIndex<0>)
            (true, false, true, false, false);
// FIXME in type.hh, prevents replacing is_iterator by RaIterator. Perhaps an additional concept is needed.
        TEST_PREDICATES(ra::TensorIndex<0> const)
            (true, false, false, false, false);
        TEST_PREDICATES(ra::TensorIndex<0> &)
            (true, false, true, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2>()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2>().iter()))
            (true, false, true, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2, 2>()()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2, 2>().iter()))
            (true, false, true, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2>()+3))
            (true, false, true, false, false);
        TEST_PREDICATES(decltype(3+ra::Big<int>()))
            (true, false, true, false, false);
        TEST_PREDICATES(std::vector<int>)
            (false, false, false, false, true);
        TEST_PREDICATES(decltype(ra::start(std::vector<int> {})))
            (true, false, true, false, false);
        TEST_PREDICATES(int *)
            (false, false, false, false, false);
    }
    tr.section("establish meaning of selectors (TODO / convert to TestRecorder)");
    {
        static_assert(ra::is_slice<ra::View<int, 0>>);
        static_assert(ra::is_slice<ra::View<int, 2>>);
        static_assert(ra::is_slice<ra::SmallView<int, mp::int_list<>, mp::int_list<>>>);

        static_assert(ra::is_ra<ra::Small<int>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallView<int, mp::nil, mp::nil>>, "bad is_ra SmallView");
        static_assert(ra::is_ra<ra::Unique<int, 0>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::View<int, 0>>, "bad is_ra View");

        static_assert(ra::is_ra<ra::Small<int, 1>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallView<int, mp::int_list<1>, mp::int_list<1>>>, "bad is_ra SmallView");
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
        static_assert(ra::is_ra_pos_rank<ra::Expr<ra::plus, std::tuple<ra::TensorIndex<0, int>, ra::Scalar<int>>>>, "bad");
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
    }
    tr.section("adaptors");
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
    tr.section("adaptors");
    {
        static_assert(ra::is_ra_vector<decltype(ra::vector(std::array<int, 2> { 1, 2 }))>);
    }
    return tr.summary();
}
