
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-types.C
/// @brief Show what types conform to what type predicates.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/test.H"
#include "ra/type.H"
#include "ra/large.H"
#include "ra/operators.H"
#include "ra/io.H"
#include <typeinfo>
#include <string>

using std::cout; using std::endl; using std::flush;

#define TEST_PREDICATES(A)                                              \
    [&tr](bool ra, bool slice, bool array_iterator, bool scalar, bool foreign_vector) \
    {                                                                   \
        /* maybe https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54483 for the need of bool() here with -O0 and -O1. */ \
        tr.info(STRINGIZE(A)).info("bool").test_eq(ra, bool(ra::is_ra<A>)); \
        tr.info(STRINGIZE(A)).info("slice").test_eq(slice, bool(ra::is_slice<A>)); \
        tr.info(STRINGIZE(A)).info("array_iterator").test_eq(array_iterator, bool(ra::is_array_iterator<A>)); \
        tr.info(STRINGIZE(A)).info("scalar").test_eq(scalar, bool(ra::is_scalar<A>)); \
        tr.info(STRINGIZE(A)).info("foreign_vector").test_eq(foreign_vector, bool(ra::is_foreign_vector<A>)); \
    }

int main()
{
    TestRecorder tr(std::cout);
    {
        TEST_PREDICATES(int)
            (false, false, false, true, false);
        TEST_PREDICATES(std::complex<double>)
            (false, false, false, true, false);
        TEST_PREDICATES(decltype(ra::Unique<int, 2>()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::Unique<int, 2>().iter()))
            (true, false, true, false, false);
        TEST_PREDICATES(ra::Iota<int>)
            (true, false, true, false, false);
        TEST_PREDICATES(ra::TensorIndex<0>)
            (true, false, true, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2>()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2, 2>()()))
            (true, true, false, false, false);
        TEST_PREDICATES(decltype(ra::Small<int, 2>()+3))
            (true, false, true, false, false);
        TEST_PREDICATES(decltype(3+ra::Owned<int>()))
            (true, false, true, false, false);
        TEST_PREDICATES(std::vector<int>)
            (false, false, false, false, true);
        TEST_PREDICATES(decltype(ra::start(std::vector<int> {})))
            (true, false, true, false, false);
        TEST_PREDICATES(int *)
            (false, false, false, false, false);
    }
    section("establish meaning of selectors (@TODO / convert to TestRecorder)");
    {
// rank 0 containers/slices are not is_slice (and therefore not is_ra) so that their conversions to scalar are used instead.
        static_assert(ra::is_ra<ra::Small<int>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallSlice<int, mp::nil, mp::nil>>, "bad is_ra SmallSlice");
        static_assert(ra::is_ra<ra::Unique<int, 0>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::Raw<int, 0>>, "bad is_ra Raw");

        static_assert(ra::is_ra<ra::Small<int, 1>>, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallSlice<int, mp::int_list<1>, mp::int_list<1>>>, "bad is_ra SmallSlice");
        static_assert(ra::is_ra<ra::Unique<int, 1>>, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::Raw<int, 1>>, "bad is_ra Raw");
        static_assert(ra::is_ra<ra::Raw<int>>, "bad is_ra Raw");

        static_assert(ra::is_ra<decltype(ra::scalar(3))>, "bad is_ra Scalar");
        static_assert(ra::is_ra<decltype(ra::vector({1, 2, 3}))>, "bad is_ra Vector");
        static_assert(!ra::is_ra<int *>, "bad is_ra int *");

        static_assert(ra::is_scalar<double>, "bad is_scalar real");
        static_assert(ra::is_scalar<std::complex<double> >, "bad is_scalar complex");
        static_assert(ra::is_scalar<int>, "bad is_scalar int");

        static_assert(!ra::is_scalar<decltype(ra::scalar(3))>, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<decltype(ra::vector({1, 2, 3}))>, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<decltype(ra::start(3))>, "bad is_scalar Scalar");
        int a = 3;
        static_assert(!ra::is_scalar<decltype(ra::start(a))>, "bad is_scalar Scalar");
// a regression.
        static_assert(ra::is_ra_zero_rank<ra::Scalar<int>>, "bad");
        static_assert(!ra::is_ra_pos_rank<ra::Scalar<int>>, "bad");
        static_assert(!ra::is_ra_zero_rank<ra::TensorIndex<0>>, "bad");
        static_assert(ra::is_ra_pos_rank<ra::TensorIndex<0>>, "bad");
        static_assert(!ra::ra_zero<ra::TensorIndex<0>>, "bad");
        static_assert(ra::is_ra_pos_rank<ra::Expr<ra::plus, std::tuple<ra::TensorIndex<0, int>, ra::Scalar<int> > > >, "bad");
    }
    return tr.summary();
}
