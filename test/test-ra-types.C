
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-types.C
/// @brief Show what types conform to what type predicates.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/test.H"
#include "ra/ra-type.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"
#include "ra/ra-io.H"

using std::cout; using std::endl; using std::flush;

template <class A>
struct type_preds
{
    static void test(TestRecorder & tr, bool ra, bool slice, bool array_iterator, bool scalar, bool foreign_vector)
    {
// maybe https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54483 for the need of bool() here
        tr.test_eq(ra, bool(ra::is_ra<A>));
        tr.test_eq(slice, bool(ra::is_slice<A>));
        tr.test_eq(array_iterator, bool(ra::is_array_iterator<A>::value));
        tr.test_eq(scalar, bool(ra::is_scalar<A>));
        tr.test_eq(foreign_vector, bool(ra::is_foreign_vector<A>));
    }
};

int main()
{
    TestRecorder tr(std::cout);
    {
        type_preds<int>
            ::test(tr.info("int"),
                   false, false, false, true, false);
        type_preds<std::complex<double> >
            ::test(tr.info("int"),
                   false, false, false, true, false);
        type_preds<ra::Unique<int, 2> >
            ::test(tr.info("Unique<..., 2>"),
                   true, true, false, false, false);
        type_preds<decltype(ra::Unique<int, 2>({2, 2}, ra::unspecified).iter())>
            ::test(tr.info("Unique<..., 2>.iter()"),
                   true, false, true, false, false);
        type_preds<ra::Iota<int> >
            ::test(tr.info("Iota<int>"),
                   true, false, true, false, false);
        type_preds<ra::TensorIndex<0> >
            ::test(tr.info("TensorIndex<>"),
                   true, false, true, false, false);
        type_preds<ra::Small<int, 2> >
            ::test(tr.info("Small<..., 2>"),
                   true, true, false, false, false);
        type_preds<decltype(ra::Small<int, 2, 2>()(0))>
            ::test(tr.info("Small<..., 2, 2>(0)"),
                   true, true, false, false, false);
        type_preds<decltype(ra::Small<int, 2> {1, 2}+3)>
            ::test(tr.info("ET with Small, scalar"),
                   true, false, true, false, false);
        type_preds<decltype(3+ra::Owned<int>({2, 2}, {1, 2, 3, 4}))>
            ::test(tr.info("ET with scalar, Owned"),
                   true, false, true, false, false);
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
