// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Checks for index selectors, esp. delayed. See fromb.cc.

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;

using real = double;
template <int rank=ra::ANY> using Ureal = ra::Unique<real, rank>;
using Vint = ra::Unique<int, 1>;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("len in unbeatable subscript");
    {
        ra::Big<int, 1> a({10}, ra::_0);
        tr.test_eq(ra::start({9, 8, 7, 6}), a(ra::len - std::array {1, 2, 3, 4}));
        ra::Big<int, 2> b({10, 10}, ra::_0-ra::_1);
        b(ra::len - std::array {1, 3, 4}) += 100;
        tr.test_eq(9 - ra::_0 + 100, b(9));
        tr.test_eq(7 - ra::_0 + 100, b(7));
        tr.test_eq(6 - ra::_0 + 100, b(6));
    }
    tr.section("len in unbeatable subscript");
    {
        ra::Small<int, 10> a = ra::_0;
        tr.test_eq(ra::start({9, 8, 7, 6}), a(ra::len - std::array {1, 2, 3, 4}));
        ra::Small<int, 10, 10> b = ra::_0-ra::_1;
        b(ra::len - std::array {1, 3, 4}) += 100;
        tr.test_eq(9 - ra::_0 + 100, b(9));
        tr.test_eq(7 - ra::_0 + 100, b(7));
        tr.test_eq(6 - ra::_0 + 100, b(6));
    }
    tr.section("unbeatable, 1D");
    {
        auto check_selection_unbeatable_1 = [&tr](auto && a)
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
            };
        check_selection_unbeatable_1(Ureal<1> {7, 9, 3, 4});
        check_selection_unbeatable_1(ra::Small<real, 4> {7, 9, 3, 4});
        check_selection_unbeatable_1(Ureal<>({4}, {7, 9, 3, 4}));
    }
    tr.section("unbeatable, 2D");
    {
        auto check_selection_unbeatable_2 = [&tr](auto && a)
            {
                using CT22 = ra::Small<real, 2, 2>;
                using CT2 = ra::Small<real, 2>;

                tr.info("a([0 1], [0 1])").test_eq(CT22 {a(0, 0), a(0, 1), a(1, 0), a(1, 1)},
                                                   from(a, Vint {0, 1}, Vint {0, 1}));
                tr.info("a([0 1], [1 0])").test_eq(CT22 {a(0, 1), a(0, 0), a(1, 1), a(1, 0)},
                                                   from(a, Vint {0, 1}, Vint {1, 0}));
                tr.info("a([1 0], [0 1])").test_eq(CT22 {a(1, 0), a(1, 1), a(0, 0), a(0, 1)},
                                                   from(a, Vint {1, 0}, Vint {0, 1}));
                tr.info("a([1 0], [1 0])").test_eq(CT22 {a(1, 1), a(1, 0), a(0, 1), a(0, 0)},
                                                   from(a, Vint {1, 0}, Vint {1, 0}));

                // TODO This is a nested array, which is a problem, we would use it just as from(a, [0 1], [0 1]).
                std::cout << "TODO [" << from(a, Vint {0, 1}) << "]" << std::endl;

                a = 0.;
                from(a, Vint {1, 0}, Vint {1, 0}) = CT22 {9, 7, 1, 4};
                tr.info("a([1 0], [1 0]) as lvalue").test_eq(CT22 {4, 1, 7, 9}, a);
                from(a, Vint {1, 0}, Vint {1, 0}) *= CT22 {9, 7, 1, 4};
                tr.info("a([1 0], [1 0]) as lvalue, *=").test_eq(CT22 {16, 1, 49, 81}, a);
// Note the difference with J amend, which requires x in (x m} y) ~ (y[m] = x) to be a suffix of y[m]; but we apply the general mechanism which is prefix matching.
                from(a, Vint {1, 0}, Vint {1, 0}) = CT2 {9, 7};
                tr.info("a([1 0], [1 0]) as lvalue, rank extend of right hand").test_eq(CT22 {7, 7, 9, 9}, a);
// TODO Test cases with rank!=1, starting with this couple which should work the same.
                std::cout << "-> " << from(a, Vint{1, 0}, 0) << std::endl;
                a = CT22 {4, 1, 7, 9};
                tr.info("a(rank1, rank0)").test_eq(ra::Small<real, 2>{9, 1}, from(a, Vint{1, 0}, ra::Small<int>(1).iter()));
                tr.info("a(rank0, rank1)").test_eq(ra::Small<real, 2>{9, 7}, from(a, ra::Small<int>(1).iter(), Vint{1, 0}));
            };
        check_selection_unbeatable_2(Ureal<2>({2, 2}, {1, 2, 3, 4}));
        check_selection_unbeatable_2(ra::Small<real, 2, 2>({1, 2, 3, 4}));
        check_selection_unbeatable_2(Ureal<>({2, 2}, {1, 2, 3, 4}));
    }
    tr.section("mixed scalar/unbeatable, 2D -> 1D");
    {
        auto check_selection_unbeatable_mixed = [&tr](auto && a)
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
            };
        check_selection_unbeatable_mixed(Ureal<2>({2, 2}, {1, 2, 3, 4}));
        check_selection_unbeatable_mixed(ra::Small<real, 2, 2>({1, 2, 3, 4}));
    }
    tr.section("mixed unbeatable/dots, 2D -> 2D (TODO)");
    {
        // auto check_selection_unbeatable_dots = [&tr](auto && a)
        //     {
        //         using CT2 = ra::Small<real, 2>;
        //         tr.info("a({0, 0}, ra::all)").test_eq(a(CT2 {0, 0}, ra::all), a(CT2 {0, 0}, CT2 {0, 1}));
        //         tr.info("a({0, 1}, ra::all)").test_eq(a(CT2 {0, 1}, ra::all), a(CT2 {0, 1}, CT2 {0, 1}));
        //         tr.info("a({1, 0}, ra::all)").test_eq(a(CT2 {1, 0}, ra::all), a(CT2 {1, 0}, CT2 {0, 1}));
        //         tr.info("a({1, 1}, ra::all)").test_eq(a(CT2 {1, 1}, ra::all), a(CT2 {1, 1}, CT2 {0, 1}));
        //     };
// TODO doesn't work because dots_t<> can only be beaten on, not iterated on, and the beating cases are missing.
        // check_selection_unbeatable_dots(Ureal<2>({2, 2}, {1, 2, 3, 4}));
        // check_selection_unbeatable_dots(ra::Small<real, 2, 2>({1, 2, 3, 4}));
    }
    tr.section("unbeatable, 3D & higher");
    {
// see src/test/bench-from.cc for examples of higher-D.
    }
    tr.section("undef-len-iota / where TODO elsewhere");
    {
        Ureal<2> a({4, 4}, 1.);
        a(3, 3) = 7.;
        tr.test(every(ra::map([](auto a, int i, int j)
                              {
                                  return a==(i==3 && j==3 ? 7. : 1.);
                              },
                    a, ra::_0, ra::_1)));
        tr.test_eq(where(ra::_0==3 && ra::_1==3, 7., 1.), a);
    }
// The implementation of from() uses FrameMatch / Reframe and can't handle this yet.
    tr.section("undef-len-iota<i> as subscript, using ra::Map directly.");
    {
        auto i = ra::_0;
        auto j = ra::_1;
        Ureal<2> a({4, 3}, i-j);
        Ureal<2> b({3, 4}, 0.);
        b = map([&a](int i, int j) { return a(i, j); }, j, i);
        tr.test_eq(i-j, a);
        tr.test_eq(j-i, b);
    }
    tr.section("undef-len-iota<i> as subscripts, 1 subscript TODO elsewhere");
    {
        Ureal<1> a {1, 4, 2, 3};
        Ureal<1> b({4}, 0.);
// these work bc there's another term to drive the expr.
        b = a(3-ra::_0);
        tr.test_eq(Ureal<1> {3, 2, 4, 1}, b);
        b(3-ra::_0) = a;
        tr.test_eq(Ureal<1> {3, 2, 4, 1}, b);
    }
    tr.section("TODO undef-len-iota<i> as subscripts, 2 subscript (case I)");
    {
        Ureal<2> a({4, 4}, ra::_0-ra::_1);
        Ureal<2> b({4, 4}, -99.);
        cout << a << endl;
        cout << b << endl;
        // b = a(ra::_0, ra::_0);
    }
    tr.section("TODO undef-len-iota<i> as subscripts, 2 subscript (case II)");
    {
        Ureal<2> a({4, 4}, ra::_0-ra::_1);
        Ureal<2> b({4, 4}, 0.);
        cout << a << endl;
        cout << b << endl;
// TODO these used to instantiate flat() when they should not (FIXME was for old OldTensorIndex; recheck)
        // tr.info("by_index I").test(ra::by_index<decltype(a(ra::_1, ra::_0))>);
        // cout << ra::mp::ref<decltype(a(ra::_1, ra::_0))>::rank_s() << endl;
// these don't work because a(j, i) has rank 3 = [(w=1)+1 + (w=0)+1] and so it drives, but undef len exprs shouldn't ever drive.
        // tr.info("by_index II").test(ra::by_index<decltype(b+a(ra::_1, ra::_0))>);
        // cout << ra::mp::ref<decltype(b+a(ra::_1, ra::_0))::T, 0>::rank_s() << endl;
        // cout << ra::mp::ref<decltype(b+a(ra::_1, ra::_0))::T, 1>::rank_s() << endl;
        cout << ra::rank_s<ra::mp::ref<decltype(ra::_1)>>() << endl;
        // b = a(ra::_1, ra::_0);
    }
// Small(iota) isn't beaten because the the output type cannot depend on argument values. So we treat it as a common expr.
    tr.section("ra::Small(iota)");
    {
        ra::Small<real, 4> a = ra::_0;
        tr.test_eq(a(ra::iota(2, 1)), Ureal<1> { 1, 2 });
    }
// Indirection operator using list of coordinates.
    tr.section("at() indirection");
    {
        ra::Big<int, 2> A({4, 4}, 0), B({4, 4}, 10*ra::_0 + ra::_1);
        using coord = ra::Small<int, 2>;
        ra::Big<coord, 1> I = { {1, 1}, {2, 2} };

        at(A, I) = at(B, I);
        tr.test_eq(ra::Big<int>({4, 4}, {0, 0, 0, 0, /**/ 0, 11, 0, 0,  /**/ 0, 0, 22, 0,  /**/  0, 0, 0, 0}), A);

// TODO this is why we need ops to have explicit rank.
        at(A, ra::scalar(coord{3, 2})) = 99.;
        tr.test_eq(ra::Big<int>({4, 4}, {0, 0, 0, 0, /**/ 0, 11, 0, 0,  /**/ 0, 0, 22, 0,  /**/  0, 0, 99, 0}), A);
    }
// From the manual [ra30]
    {
        ra::Big<int, 2> A = {{100, 101}, {110, 111}, {120, 121}};
        ra::Big<ra::Small<int, 2>, 2> i = {{{0, 1}, {2, 0}}, {{1, 0}, {2, 1}}};
        ra::Big<int, 2> B = at(A, i);
        tr.test_eq(ra::Big<int, 2> {{101, 120}, {110, 121}}, at(A, i));
    }
    return tr.summary();
}
