
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-from.C
/// @brief Checks for index selectors, both immediate and delayed.

#include <iostream>
#include <iterator>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush; using std::tuple;

template <int rank=ra::RANK_ANY> using Ureal = ra::Unique<real, rank>;
using Vint = ra::Unique<int, 1>;

template <class AA>
void check_selection_shortcuts(TestRecorder & tr, AA && a)
{
    tr.info("a()").test_eq(Ureal<2>({4, 4}, ra::_0-ra::_1), a());
    tr.info("a(2, :)").test_eq(Ureal<1>({4}, 2-ra::_0), a(2, ra::all));
    tr.info("a(2)").test_eq(Ureal<1>({4}, 2-ra::_0), a(2));
    tr.info("a(:, 3)").test_eq(Ureal<1>({4}, ra::_0-3), a(ra::all, 3));
    tr.info("a(:, :)").test_eq(Ureal<2>({4, 4}, ra::_0-ra::_1), a(ra::all, ra::all));
    tr.info("a(:)").test_eq(Ureal<2>({4, 4}, ra::_0-ra::_1), a(ra::all));
    tr.info("a(1)").test_eq(Ureal<1>({4}, 1-ra::_0), a(1));
    tr.info("a(2, 2)").test_eq(0, a(2, 2));
    tr.info("a(0:2:, 0:2:)").test_eq(Ureal<2>({2, 2}, 2*(ra::_0-ra::_1)), a(ra::jvec(2, 0, 2), ra::jvec(2, 0, 2)));
    tr.info("a(1:2:, 0:2:)").test_eq(Ureal<2>({2, 2}, 2*ra::_0+1-2*ra::_1), a(ra::jvec(2, 1, 2), ra::jvec(2, 0, 2)));
    tr.info("a(0:2:, :)").test_eq(Ureal<2>({2, 4}, 2*ra::_0-ra::_1), a(ra::jvec(2, 0, 2), ra::all));
    tr.info("a(0:2:)").test_eq(a(ra::jvec(2, 0, 2), ra::all), a(ra::jvec(2, 0, 2)));
}

template <class AA>
void check_selection_unbeatable_1(TestRecorder & tr, AA && a)
{
    using CT = ra::Small<real, 4>;

    tr.info("a(i ...)").test_eq(CT {a[3], a[2], a[0], a[1]}, a(Vint {3, 2, 0, 1}));
    tr.info("a(i ...)").test_eq(CT {a[3], a[2], a[0], a[1]}, from(a, Vint {3, 2, 0, 1}));

    a = 0.;
    a(Vint {3, 2, 0, 1}) = CT {9, 7, 1, 4};
    tr.info("a(i ...) as lvalue").test_eq(CT {1, 4, 7, 9}, a);

    a = 0.;
    from(a, Vint {3, 2, 0, 1}) = CT {9, 7, 1, 4};
    tr.info("from(a i ...) as lvalue").test_eq(CT {1, 4, 7, 9}, a);

    a = 0.;
    from(a, Vint {3, 2, 0, 1}) = 77.;
    tr.info("from(a i ...) as lvalue, rank extend of right hand").test_eq(a, 77.);

    ra::Small<real, 2, 2> c = from(a, ra::Small<int, 2, 2> {3, 2, 0, 1});
    tr.info("a([x y; z w])").test_eq(ra::Small<real, 2, 2> {a[3], a[2], a[0], a[1]}, c);
}

template <class AA>
void check_selection_unbeatable_2(TestRecorder & tr, AA && a)
{
    using CT22 = ra::Small<real, 2, 2>;
    using CT2 = ra::Small<real, 2>;

    tr.info("a([0 1], [0 1])").test_eq(CT22 {a(0, 0), a(0, 1), a(1, 0), a(1, 1)}, from(a, Vint {0, 1}, Vint {0, 1}));
    tr.info("a([0 1], [1 0])").test_eq(CT22 {a(0, 1), a(0, 0), a(1, 1), a(1, 0)}, from(a, Vint {0, 1}, Vint {1, 0}));
    tr.info("a([1 0], [0 1])").test_eq(CT22 {a(1, 0), a(1, 1), a(0, 0), a(0, 1)}, from(a, Vint {1, 0}, Vint {0, 1}));
    tr.info("a([1 0], [1 0])").test_eq(CT22 {a(1, 1), a(1, 0), a(0, 1), a(0, 0)}, from(a, Vint {1, 0}, Vint {1, 0}));

    a = 0.;
    from(a, Vint {1, 0}, Vint {1, 0}) = CT22 {9, 7, 1, 4};
    tr.info("a([1 0], [1 0]) as lvalue").test_eq(CT22 {4, 1, 7, 9}, a);
    from(a, Vint {1, 0}, Vint {1, 0}) *= CT22 {9, 7, 1, 4};
    tr.info("a([1 0], [1 0]) as lvalue, *=").test_eq(CT22 {16, 1, 49, 81}, a);

// Note the difference with J amend, which requires x in (x m} y) ~ (y[m] = x) to be a suffix of y[m]; but we apply the general mechanism which is prefix matching.
    from(a, Vint {1, 0}, Vint {1, 0}) = CT2 {9, 7};
    tr.info("a([1 0], [1 0]) as lvalue, rank extend of right hand").test_eq(CT22 {7, 7, 9, 9}, a);

// @TODO Test cases with rank!=1, starting with this couple which should work the same.
    // std::cout << "-> " << from(a, Vint{1, 0}, 0) << std::endl;
    a = CT22 {4, 1, 7, 9};
    tr.info("a(rank1, rank0)").test_eq(ra::Small<real, 2>{9, 1}, from(a, Vint{1, 0}, ra::Small<int>(1).iter()));
    tr.info("a(rank0, rank1)").test_eq(ra::Small<real, 2>{9, 7}, from(a, ra::Small<int>(1).iter(), Vint{1, 0}));
}

template <class AA>
void check_selection_unbeatable_mixed(TestRecorder & tr, AA && a)
{
    using CT2 = ra::Small<real, 2>;
    tr.info("from(a [0 1], 1)").test_eq(CT2 {a(0, 1), a(1, 1)}, from(a, Vint {0, 1}, 1));
    tr.info("from(a [1 0], 1)").test_eq(CT2 {a(1, 1), a(0, 1)}, from(a, Vint {1, 0}, 1));
    tr.info("from(a 1, [0 1])").test_eq(CT2 {a(1, 0), a(1, 1)}, from(a, 1, Vint {0, 1}));
    tr.info("from(a 1, [1 0])").test_eq(CT2 {a(1, 1), a(1, 0)}, from(a, 1, Vint {1, 0}));
    tr.info("a([0 1], 1)").test_eq(CT2 {a(0, 1), a(1, 1)}, a(Vint {0, 1}, 1));
    tr.info("a([1 0], 1)").test_eq(CT2 {a(1, 1), a(0, 1)}, a(Vint {1, 0}, 1));
    tr.info("a(1, [0 1])").test_eq(CT2 {a(1, 0), a(1, 1)}, a(1, Vint {0, 1}));
    tr.info("a(1, [1 0])").test_eq(CT2 {a(1, 1), a(1, 0)}, a(1, Vint {1, 0}));
}

int main()
{
    TestRecorder tr(std::cout);
    section("shortcuts");
    {
        check_selection_shortcuts(tr, Ureal<2>({4, 4}, ra::_0-ra::_1));
        check_selection_shortcuts(tr, Ureal<>({4, 4}, ra::_0-ra::_1));
    }
    section("ra::Iota<int> or ra::Iota<ra::dim_t> are both beatable");
    {
        Ureal<2> a({4, 4}, 0.);
        {
            ra::Iota<int> i(2, 1);
            auto b = a(i);
            tr.test_eq(2, b.dim[0].size);
            tr.test_eq(4, b.dim[1].size);
            tr.test_eq(4, b.dim[0].stride);
            tr.test_eq(1, b.dim[1].stride);
        }
        {
            ra::Iota<ra::dim_t> i(2, 1);
            auto b = a(i);
            tr.test_eq(2, b.dim[0].size);
            tr.test_eq(4, b.dim[1].size);
            tr.test_eq(4, b.dim[0].stride);
            tr.test_eq(1, b.dim[1].stride);
        }
    }
    section("beatable multi-axis selectors, var size");
    {
        static_assert(ra::is_beatable<ra::dots_t<0> >::value, "dots_t<0> is beatable");
        ra::Owned<int, 3> a({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2);
        tr.info("a(ra::dots<0> ...)").test_eq(a(0), a(ra::dots<0>, 0));
        tr.info("a(ra::dots<0> ...)").test_eq(a(1), a(ra::dots<0>, 1));
        tr.info("a(ra::dots<1> ...)").test_eq(a(ra::all, 0), a(ra::dots<1>, 0));
        tr.info("a(ra::dots<1> ...)").test_eq(a(ra::all, 1), a(ra::dots<1>, 1));
        tr.info("a(ra::dots<2> ...)").test_eq(a(ra::all, ra::all, 0), a(ra::dots<2>, 0));
        tr.info("a(ra::dots<2> ...)").test_eq(a(ra::all, ra::all, 1), a(ra::dots<2>, 1));
        tr.info("a(0)").test_eq(a(0, ra::all, ra::all), a(0));
        tr.info("a(1)").test_eq(a(1, ra::all, ra::all), a(1));
        tr.info("a(0, ra::dots<2>)").test_eq(a(0, ra::all, ra::all), a(0, ra::dots<2>));
        tr.info("a(1, ra::dots<2>)").test_eq(a(1, ra::all, ra::all), a(1, ra::dots<2>));
    }
    section("beatable multi-axis selectors, fixed size");
    {
        static_assert(ra::is_beatable<ra::dots_t<0> >::value, "dots_t<0> is beatable");
        ra::Small<int, 2, 3, 4> a = ra::_0*100 + ra::_1*10 + ra::_2;
        tr.info("a(ra::dots<0> ...)").test_eq(a(0), a(ra::dots<0>, 0));
        tr.info("a(ra::dots<0> ...)").test_eq(a(1), a(ra::dots<0>, 1));
        tr.info("a(ra::dots<1> ...)").test_eq(a(ra::all, 0), a(ra::dots<1>, 0));
        tr.info("a(ra::dots<1> ...)").test_eq(a(ra::all, 1), a(ra::dots<1>, 1));
        tr.info("a(ra::dots<2> ...)").test_eq(a(ra::all, ra::all, 0), a(ra::dots<2>, 0));
        tr.info("a(ra::dots<2> ...)").test_eq(a(ra::all, ra::all, 1), a(ra::dots<2>, 1));
        tr.info("a(0)").test_eq(a(0, ra::all, ra::all), a(0));
        tr.info("a(1)").test_eq(a(1, ra::all, ra::all), a(1));
        tr.info("a(0, ra::dots<2>)").test_eq(a(0, ra::all, ra::all), a(0, ra::dots<2>));
        tr.info("a(1, ra::dots<2>)").test_eq(a(1, ra::all, ra::all), a(1, ra::dots<2>));
    }
    section("unbeatable, 1D");
    {
        check_selection_unbeatable_1(tr, Ureal<1> {7, 9, 3, 4});
        check_selection_unbeatable_1(tr, ra::Small<real, 4> {7, 9, 3, 4});
    }
    // section("unbeatable, dynamic rank 1D"); // @TODO should work.
    // {
    //     cout << Ureal<>({4}, {7, 9, 3, 4}) << endl;
    //     check_selection_unbeatable_1(tr, Ureal<>({4}, {7, 9, 3, 4}));
    // }
    section("unbeatable, 2D");
    {
        check_selection_unbeatable_2(tr, Ureal<2>({2, 2}, {1, 2, 3, 4}));
        check_selection_unbeatable_2(tr, ra::Small<real, 2, 2>({1, 2, 3, 4}));
    }
    section("mixed scalar/unbeatable, 2D -> 1D");
    {
        check_selection_unbeatable_mixed(tr, Ureal<2>({2, 2}, {1, 2, 3, 4}));
        check_selection_unbeatable_mixed(tr, ra::Small<real, 2, 2>({1, 2, 3, 4}));
    }
    section("unbeatable, 3D & higher");
    {
// see src/test/bench-ra-from.C for examples of higher-D.
    }
    section("TensorIndex / where @TODO elsewhere");
    {
        Ureal<2> a({4, 4}, 1.);
        a(3, 3) = 7.;
        tr.test(every(ra::expr([](auto a, int i, int j) { return a==(i==3 && j==3 ? 7. : 1.); }, ra::start(a), ra::_0, ra::_1)));
        tr.test_eq(where(ra::_0==3 && ra::_1==3, 7., 1.), a);
    }
// The implementation of from() uses FrameMatch / ApplyFrames and can't handle this yet.
    section("TensorIndex<i> as subscript, using ra::Expr directly.");
    {
        auto i = ra::_0;
        auto j = ra::_1;
        Ureal<2> a({4, 3}, i-j);
        Ureal<2> b({3, 4}, 0.);
        b = map([&a](int i, int j) { return a(i, j); }, j, i);
        tr.test_eq(i-j, a);
        tr.test_eq(j-i, b);
    }
    section("TensorIndex<i> as subscripts, 1 subscript @TODO elsewhere");
    {
        Ureal<1> a {1, 4, 2, 3};
        Ureal<1> b({4}, 0.);
// these work b/c there's another term to drive the expr.
        b = a(3-ra::_0);
        tr.test_eq(Ureal<1> {3, 2, 4, 1}, b);
        b(3-ra::_0) = a;
        tr.test_eq(Ureal<1> {3, 2, 4, 1}, b);
    }
    section("@TODO TensorIndex<i> as subscripts, 2 subscript (case I)");
    {
        Ureal<2> a({4, 4}, ra::_0-ra::_1);
        Ureal<2> b({4, 4}, -99.);
        cout << a << endl;
        cout << b << endl;
        // b = a(ra::_0, ra::_0);
    }
    section("@TODO TensorIndex<i> as subscripts, 2 subscript (case II)");
    {
        Ureal<2> a({4, 4}, ra::_0-ra::_1);
        Ureal<2> b({4, 4}, 0.);
        cout << a << endl;
        cout << b << endl;
        tr.info("has_tensorindex(TensorIndex)").test(ra::has_tensorindex<decltype(ra::_1)>::value);
        tr.info("has_tensorindex(Expr)").test(ra::has_tensorindex<decltype(ra::_1+ra::_0)>::value);
// @TODO these instantiate flat() when they should not
        // tr.info("has_tensorindex(Ryn)").test(ra::has_tensorindex<decltype(a(ra::_1, ra::_0))>::value);
        // cout << mp::Ref_<decltype(a(ra::_1, ra::_0))>::rank_s() << endl;
// these don't work because a(j, i) has rank 3 = [(w=1)+1 + (w=0)+1] and so it drives, but tensorindex exprs shouldn't ever drive.
        // tr.info("has_tensorindex(Ryn)").test(ra::has_tensorindex<decltype(b+a(ra::_1, ra::_0))>::value);
        // cout << mp::Ref_<decltype(b+a(ra::_1, ra::_0))::T, 0>::rank_s() << endl;
        // cout << mp::Ref_<decltype(b+a(ra::_1, ra::_0))::T, 1>::rank_s() << endl;
        cout << mp::Ref_<decltype(ra::_1)>::rank_s() << endl;
        // b = a(ra::_1, ra::_0);
    }
// Small(Iota) isn't beaten because the the output type cannot depend on argument values. So we treat it as a common expr.
    section("ra::Small(Iota)");
    {
        ra::Small<real, 4> a = ra::_0;
        tr.test_eq(a(ra::iota(2, 1)), Ureal<1> { 1, 2 });
    }
    return tr.summary();
}
