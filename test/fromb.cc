// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Checks for index selectors, esp. immediate. See fromu.cc.

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifdef RA_DO_CHECK
  #undef RA_DO_CHECK
  #define RA_DO_CHECK 1 // kind of the point here
#endif

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;

using real = double;
template <int rank=ra::RANK_ANY> using Ureal = ra::Unique<real, rank>;
using Vint = ra::Unique<int, 1>;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("beating Small with static iota");
    {
        ra::Small<int, 10> a = ra::_0;
        {
            auto b = a(ra::iota(ra::int_c<4>()));
            tr.test_eq(ra::Small<int, 4>(ra::_0), b);
            tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data()));
        }
        {
            auto b = a(ra::iota(4));
            tr.test_eq(ra::Small<int, 4>(ra::_0), b);
        }
    }
    {
        ra::Small<int, 10, 10> a = ra::_1 + 10*ra::_0;
        {
            auto b = a(3, ra::all);
            tr.test_eq(ra::Small<int, 10>(30+ra::_0), b);
            tr.test_eq(ra::scalar(a.data()+30), ra::scalar(b.data()));
        }
        {
            auto b = a(ra::iota(ra::int_c<4>()));
            tr.test_eq(ra::Small<int, 4, 10>(ra::_1 + 10*ra::_0), b);
            tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data()));
        }
        {
            auto b = a(ra::iota(ra::int_c<4>(), 4));
            tr.test_eq(ra::Small<int, 4, 10>(ra::_1 + 10*(ra::_0+4)), b);
            tr.test_eq(ra::scalar(a.data()+40), ra::scalar(b.data()));
        }
        {
            auto b = a(3, ra::iota(ra::int_c<5>(), 4));
            tr.test_eq(ra::Small<int, 5>(30+ra::_0+4), b);
            tr.test_eq(ra::scalar(a.data()+30+4), ra::scalar(b.data()));
        }
        {
            auto b = a(ra::all, ra::iota(ra::int_c<4>(), 2, ra::int_c<2>()));
            tr.test_eq(ra::Small<int, 10, 4>(10*ra::_0 + 2*(1+ra::_1)), b);
            tr.test_eq(ra::scalar(a.data()+2), ra::scalar(b.data()));
        }
        {
            auto b = a(ra::iota(ra::int_c<3>(), 1, ra::int_c<2>()),
                       ra::iota(ra::int_c<2>(), 2, ra::int_c<3>()));
            tr.test_eq(ra::Small<int, 3, 2>(10*(1+2*ra::_0) + 2+ra::_1*3), b);
            tr.test_eq(ra::scalar(a.data()+12), ra::scalar(b.data()));
        }
        {
            auto b = a(ra::iota(ra::int_c<3>(), 9, ra::int_c<-2>()),
                       ra::iota(ra::int_c<2>(), 2, ra::int_c<3>()));
            tr.test_eq(ra::Small<int, 3, 2>(10*(9-2*ra::_0) + 2+ra::_1*3), b);
            tr.test_eq(ra::scalar(a.data()+92), ra::scalar(b.data()));
        }
// FIXME the unbeaten path caused by rt iota results in a nested rank expr [ra33]
        {
            cout << a(ra::iota(4)) << endl;
            // tr.test_eq(ra::Small<int, 4, 10>(ra::_1 + 10*ra::_0), a(ra::iota(4)));
        }
// FIXME the unbeaten path caused by rt iota fails bc ra::all isn't an expr, just a 'special object' for subscripts. So we can't even print.
        {
            // cout << a(ra::all, ra::iota(4)) << endl;
            // tr.test_eq(ra::Small<int, 10, 4>(ra::_1 + 10*ra::_0), a(ra::all, ra::iota(4)));
        }
// FIXME static iota(expr(ra::len) ...)
    }
    tr.section("zero length iota");
    {
// 1-past is ok but 1-before is not, so these just leave the pointer unchanged.
        {
            ra::Small<int, 10> a = ra::_0;
            auto b = a(ra::iota(ra::int_c<0>(), 10));
            tr.test_eq(ra::Small<int, 0>(ra::_0+10), b);
            tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data()));
        }
        {
            ra::Small<int, 10> a = ra::_0;
            auto b = a(ra::iota(ra::int_c<0>(), 10, ra::int_c<-1>()));
            tr.test_eq(ra::Small<int, 0>(ra::_0-1), b);
            cout << "a " << a.data() << " b " << b.data() << endl;
            tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data()));
        }
    }
    tr.section("Iota<T> is beatable for any integral T");
    {
        Ureal<2> a({4, 4}, 0.);
        auto test = [&](auto org)
        {
            auto i = ra::iota(2, org);
            static_assert(std::is_same_v<decltype(i.i), decltype(org)>);
            auto b = a(i);
            tr.test_eq(2, b.dimv[0].len);
            tr.test_eq(4, b.dimv[1].len);
            tr.test_eq(4, b.dimv[0].step);
            tr.test_eq(1, b.dimv[1].step);
        };
        test(int(1));
        test(int16_t(1));
        test(ra::dim_t(1));
    }
    tr.section("trivial case");
    {
        ra::Big<int, 3> a({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2);
        tr.test_eq(ra::_0*100 + ra::_1*10 + ra::_2, from(a));
    }
    tr.section("scalar len (var size)");
    {
        ra::Big<int, 3> a({2, 3, 4}, ra::_0*100 + ra::_1*10 + (2 - ra::_2));
        tr.test_eq(a(1, 0, 0), a(ra::len-1, 0, 0));
        tr.test_eq(a(0, 2, 0), a(0, ra::len-1, 0));
        tr.test_eq(a(0, 0, 3), a(0, 0, ra::len-1));
    }
    tr.section("scalar len (static size)");
    {
        ra::Small<int, 4, 3, 2> a = ra::_0 - 10*ra::_1 + 100*ra::_2;
        tr.test_eq(a(3, 0, 0), a(ra::len-1, 0, 0));
        tr.test_eq(a(0, 2, 0), a(0, ra::len-1, 0));
        tr.test_eq(a(0, 0, 1), a(0, 0, ra::len-1));
        tr.test_eq(a(3, 2, 1), a(ra::len-1, ra::len-1, ra::len-1));
    }
    tr.section("iota len (var size)");
    {
        ra::Big<int, 3> a({2, 3, 4}, ra::_0*100 + ra::_1*10 + (2 - ra::_2));
// expr len is beatable and gives views.
        tr.test_eq(1, ra::size(a(ra::iota(ra::len), 0, 0).dimv));
        tr.test_eq(a(ra::iota(2), 0, 0), a(ra::iota(ra::len), 0, 0));
        tr.test_eq(a(0, ra::iota(3), 0), a(0, ra::iota(ra::len), 0));
        tr.test_eq(a(0, 0, ra::iota(4)), a(0, 0, ra::iota(ra::len)));
// expr org is beatable and gives views.
        tr.test_eq(1, ra::size(a(ra::iota(ra::len, ra::len*0), 0, 0).dimv));
        tr.test_eq(a(ra::iota(2), 0, 0), a(ra::iota(ra::len, ra::len*0), 0, 0));
        tr.test_eq(a(0, ra::iota(3), 0), a(0, ra::iota(ra::len, ra::len*0), 0));
        tr.test_eq(a(0, 0, ra::iota(4)), a(0, 0, ra::iota(ra::len, ra::len*0)));
// expr step is beatable.
        tr.test_eq(1, ra::size(a(0, 0, ra::iota(2, 0, ra::len/2)).dimv));
        tr.test_eq(a(0, 0, ra::iota(2, 0, 2)), a(0, 0, ra::iota(2, 0, ra::len/2)));
    }
    tr.section("iota len (static size) TBD");
    {
    }
    tr.section("beatable multi-axis selectors, var size");
    {
        static_assert(ra::beatable<ra::dots_t<0>>.rt, "dots_t<0> is beatable");
        auto test = [&tr](auto && a)
        {
            tr.info("a(ra::dots<0>, ...)").test_eq(a(0), a(ra::dots<0>, 0));
            tr.info("a(ra::dots<0>, ...)").test_eq(a(1), a(ra::dots<0>, 1));
            tr.info("a(ra::dots<1>, 0, ...)").test_eq(a(ra::all, 0), a(ra::dots<1>, 0));
            tr.info("a(ra::dots<1>, 1, ...)").test_eq(a(ra::all, 1), a(ra::dots<1>, 1));
            tr.info("a(ra::dots<2>, 0)").test_eq(a(ra::all, ra::all, 0), a(ra::dots<2>, 0));
            tr.info("a(ra::dots<2>, 1)").test_eq(a(ra::all, ra::all, 1), a(ra::dots<2>, 1));
            tr.info("a(ra::dots<2>, len-1)").test_eq(a(ra::all, ra::all, 3), a(ra::dots<2>, ra::len-1));
            tr.info("a(ra::dots<>, 1)").test_eq(a(ra::all, ra::all, 1), a(ra::dots<>, 1));
            tr.info("a(0)").test_eq(a(0, ra::all, ra::all), a(0));
            tr.info("a(1)").test_eq(a(1, ra::all, ra::all), a(1));
            tr.info("a(0, ra::dots<2>)").test_eq(a(0, ra::all, ra::all), a(0, ra::dots<2>));
            tr.info("a(1, ra::dots<2>)").test_eq(a(1, ra::all, ra::all), a(1, ra::dots<2>));
            tr.info("a(len-1, ra::dots<2>)").test_eq(a(1, ra::all, ra::all), a(ra::len-1, ra::dots<2>));
            tr.info("a(1, ra::dots<>)").test_eq(a(1, ra::all, ra::all), a(1, ra::dots<>));
            tr.info("a(0, ra::dots<>, 1)").test_eq(a(0, ra::all, 1), a(0, ra::dots<>, 1));
            tr.info("a(1, ra::dots<>, 0)").test_eq(a(1, ra::all, 0), a(1, ra::dots<>, 0));
            // cout << a(ra::dots<>, 1, ra::dots<>) << endl; // ct error
        };
        tr.section("fixed size");
        test(ra::Small<int, 2, 3, 4>(ra::_0*100 + ra::_1*10 + ra::_2));
        tr.section("fixed rank");
        test(ra::Big<int, 3>({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2));
        tr.section("var rank");
        test(ra::Big<int>({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2));
    }
    tr.section("insert, var size");
    {
        static_assert(ra::beatable<ra::insert_t<1>>.rt, "insert_t<1> is beatable");
        ra::Big<int, 3> a({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2);
        tr.info("a(ra::insert<0> ...)").test_eq(a(0), a(ra::insert<0>, 0));
        ra::Big<int, 4> a1({1, 2, 3, 4}, ra::_1*100 + ra::_2*10 + ra::_3);
        tr.info("a(ra::insert<1> ...)").test_eq(a1, a(ra::insert<1>));
        ra::Big<int, 4> a2({2, 1, 3, 4}, ra::_0*100 + ra::_2*10 + ra::_3);
        tr.info("a(ra::all, ra::insert<1>, ...)").test_eq(a2, a(ra::all, ra::insert<1>));
        ra::Big<int, 5> a3({2, 1, 1, 3, 4}, ra::_0*100 + ra::_3*10 + ra::_4);
        tr.info("a(ra::all, ra::insert<2>, ...)").test_eq(a3, a(ra::all, ra::insert<2>));
        tr.info("a(0, ra::insert<1>, ...)").test_eq(a1(ra::all, 0), a(0, ra::insert<1>));
        tr.info("a(ra::insert<1>, 0, ...)").test_eq(a1(ra::all, 0), a(ra::insert<1>, 0));
        ra::Big<int, 4> aa1({2, 2, 3, 4}, a(ra::insert<1>));
        tr.info("insert with undefined len 0").test_eq(a, aa1(0));
        tr.info("insert with undefined len 1").test_eq(a, aa1(1));
    }
    tr.section("insert, var rank");
    {
        static_assert(ra::beatable<ra::insert_t<1>>.rt, "insert_t<1> is beatable");
        ra::Big<int> a({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2);
        tr.info("a(ra::insert<0> ...)").test_eq(a(0), a(ra::insert<0>, 0));
        ra::Big<int> a1({1, 2, 3, 4}, ra::_1*100 + ra::_2*10 + ra::_3);
        tr.info("a(ra::insert<1> ...)").test_eq(a1, a(ra::insert<1>));
        ra::Big<int> a2({2, 1, 3, 4}, ra::_0*100 + ra::_2*10 + ra::_3);
        tr.info("a(ra::all, ra::insert<1>, ...)").test_eq(a2, a(ra::all, ra::insert<1>));
        ra::Big<int> a3({2, 1, 1, 3, 4}, ra::_0*100 + ra::_3*10 + ra::_4);
        tr.info("a(ra::all, ra::insert<2>, ...)").test_eq(a3, a(ra::all, ra::insert<2>));
        tr.info("a(0, ra::insert<1>, ...)").test_eq(a1(ra::all, 0), a(0, ra::insert<1>));
        tr.info("a(ra::insert<1>, 0, ...)").test_eq(a1(ra::all, 0), a(ra::insert<1>, 0));
    }
    tr.section("mix insert + dots");
    {
        static_assert(ra::beatable<ra::insert_t<1>>.rt, "insert_t<1> is beatable");
        auto test = [&tr](auto && a, auto && b)
        {
            tr.info("a(ra::insert<0>, ra::dots<3>)").test_eq(a(ra::insert<0>, ra::dots<3>), a(ra::insert<0>, ra::dots<>));
            tr.info("a(ra::insert<0>, ra::dots<1>, ...)").test_eq(a(ra::insert<0>, ra::all, ra::all, ra::all), a(ra::insert<0>, ra::dots<>));
            tr.info("a(ra::insert<0>, ra::dots<>)").test_eq(a(ra::insert<0>), a(ra::insert<0>, ra::dots<>));
// add to something else to establish the size of the inserted axis.
            tr.info("a(ra::insert<1>, ra::dots<3>)")
                .test_eq(a(ra::insert<1>, ra::dots<3>) + ra::iota(2), a(ra::insert<1>, ra::dots<>) + ra::iota(2));
            tr.info("a(ra::insert<1>, ra::dots<1>, ...)")
                .test_eq(a(ra::insert<1>, ra::all, ra::all, ra::all) + ra::iota(2), a(ra::insert<1>, ra::dots<>) + ra::iota(2));
            tr.info("a(ra::insert<1>, ra::dots<>)").test_eq(a(ra::insert<1>) + ra::iota(2),
                                                            a(ra::insert<1>, ra::dots<>) + ra::iota(2));
// same on the back.
            tr.info("a(ra::dots<3>, ra::insert<1>)")
                .test_eq(b + a(ra::dots<3>, ra::insert<1>), b + a(ra::dots<>, ra::insert<1>));
            tr.info("a(ra::dots<1>, ..., ra::insert<1>)")
                .test_eq(b + a(ra::all, ra::all, ra::all, ra::insert<1>), b + a(ra::dots<>, ra::insert<1>));
        };
        tr.section("fixed rank");
        test(ra::Big<int, 3>({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2),
             ra::Big<int, 4>({2, 3, 4, 2}, ra::_0*100 + ra::_1*10 + ra::_2 + (2-ra::_3)));
        tr.section("var rank");
        test(ra::Big<int>({2, 3, 4}, ra::_0*100 + ra::_1*10 + ra::_2),
             ra::Big<int>({2, 3, 4, 2}, ra::_0*100 + ra::_1*10 + ra::_2 + (2-ra::_3)));
    }
    tr.section("shortcuts");
    {
        auto check_selection_shortcuts = [&tr](auto && a)
            {
                tr.info("a()").test_eq(Ureal<2>({4, 4}, ra::_0-ra::_1), a());
                tr.info("a(2, :)").test_eq(Ureal<1>({4}, 2-ra::_0), a(2, ra::all));
                tr.info("a(2)").test_eq(Ureal<1>({4}, 2-ra::_0), a(2));
                tr.info("a(:, 3)").test_eq(Ureal<1>({4}, ra::_0-3), a(ra::all, 3));
                tr.info("a(:, :)").test_eq(Ureal<2>({4, 4}, ra::_0-ra::_1), a(ra::all, ra::all));
                tr.info("a(:)").test_eq(Ureal<2>({4, 4}, ra::_0-ra::_1), a(ra::all));
                tr.info("a(1)").test_eq(Ureal<1>({4}, 1-ra::_0), a(1));
                tr.info("a(2, 2)").test_eq(0, a(2, 2));
                tr.info("a(0:2:, 0:2:)").test_eq(Ureal<2>({2, 2}, 2*(ra::_0-ra::_1)),
                                                 a(ra::iota(2, 0, 2), ra::iota(2, 0, 2)));
                tr.info("a(1:2:, 0:2:)").test_eq(Ureal<2>({2, 2}, 2*ra::_0+1-2*ra::_1),
                                                 a(ra::iota(2, 1, 2), ra::iota(2, 0, 2)));
                tr.info("a(0:2:, :)").test_eq(Ureal<2>({2, 4}, 2*ra::_0-ra::_1),
                                              a(ra::iota(2, 0, 2), ra::all));
                tr.info("a(0:2:)").test_eq(a(ra::iota(2, 0, 2), ra::all), a(ra::iota(2, 0, 2)));
            };
        check_selection_shortcuts(Ureal<2>({4, 4}, ra::_0-ra::_1));
        check_selection_shortcuts(Ureal<>({4, 4}, ra::_0-ra::_1));
    }
    return tr.summary();
}
