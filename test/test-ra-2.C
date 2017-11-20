
// (c) Daniel Llorens - 2013, 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-2.C
/// @brief Checks for ra:: arrays, especially cell rank > 0 operations.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush;
using real = double;

int main()
{
    TestRecorder tr;
    tr.section("iterators on cell rank > 0");
    {
        ra::Unique<real, 2> a({4, 3}, ra::unspecified);
        std::iota(a.begin(), a.end(), 1);
        {
            ra::ra_iterator<decltype(a), 1> i;
            tr.test_eq(1, i.rank());
        }
        {
            ra::ra_iterator<decltype(a), 1> i(a.dim, a.p);
            tr.test_eq(1, i.rank());
            ply_ravel(expr([](ra::View<real, 1> const & x) { cout << x << endl; }, i));
        }
    }
#define ARGa ra::ra_iterator<decltype(a), 1>(a.dim, a.p)
#define ARGi ra::Small<int, 4> {1, 2, 3, 4}.iter()
#define ARGd dump.iter()
    tr.section("ply on cell rank > 0");
    {
        ra::Unique<real, 2> a({4, 3}, ra::unspecified);
        std::iota(a.begin(), a.end(), 1);
        real check[4] = {6, 15, 24, 33};
        tr.section("not driving");
        {
            auto f = [](int i, ra::View<real, 1> const & a, real & d)
                {
                    cout << i << ": " << a << endl;
                    d = a[0] + a[1] + a[2];
                };
            ra::Small<real, 4> dump;
            //             plier(ra::expr(f, a.iter<0>(), ARGa, ARGd));  // how the hell does this work,
#define TEST(plier)                                             \
            dump = 0;                                           \
            cout << "-> explicit iterator" << endl;             \
            plier(ra::expr(f, ARGi, ARGa, ARGd));               \
            tr.test(std::equal(check, check+4, dump.begin()));  \
            dump = 0;                                           \
            cout << "-> iter<cell rank>()" << endl;             \
            plier(ra::expr(f, ARGi, a.iter<1>(), ARGd));        \
            tr.test(std::equal(check, check+4, dump.begin()));  \
            dump = 0;                                           \
            cout << "-> iter<frame rank>()" << endl;            \
            plier(ra::expr(f, ARGi, a.iter<-1>(), ARGd));       \
            tr.test(std::equal(check, check+4, dump.begin()));
            TEST(ply_index);
            TEST(ply_ravel);
            TEST(plyf_index);
            TEST(plyf);
#undef TEST
        }
// TODO Use explicit DRIVER arg to ra::expr; the fixed size ARGi should always drive.
        tr.section("driving");
        {
            auto f = [](ra::View<real, 1> const & a, int i, real & d)
                {
                    cout << i << ": " << a << endl;
                    d = a[0] + a[1] + a[2];
                };
            ra::Small<real, 4> dump;
#define TEST(plier)                                             \
            dump = 0;                                           \
            plier(ra::expr(f, ARGa, ARGi, ARGd));            \
            tr.test(std::equal(check, check+4, dump.begin())); \
            dump = 0;                                           \
            plier(ra::expr(f, a.iter<1>(), ARGi, ARGd));     \
            tr.test(std::equal(check, check+4, dump.begin())); \
            dump = 0;                                           \
            plier(ra::expr(f, a.iter<-1>(), ARGi, ARGd));    \
            tr.test(std::equal(check, check+4, dump.begin()));
            TEST(ply_index);
            TEST(ply_ravel);
            TEST(plyf_index);
            TEST(plyf);
#undef TEST
        }
    }
    tr.section("ply on cell rank > 0, ref argument");
    {
        ra::Small<real, 4> dump { 1, 2, 3, 4 };
        ra::Unique<real, 2> a({4, 3}, ra::unspecified);
        real check[12] = {1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6};
        tr.section("not driving");
        {
            auto f = [](int i, ra::View<real, 1> a, real & d) { std::iota(a.begin(), a.end(), d); };
            std::fill(a.begin(), a.end(), 0);
            ply_index(ra::expr(f, ARGi, ARGa, ARGd));
            tr.test(std::equal(check, check+12, a.begin()));

            std::fill(a.begin(), a.end(), 0);
            ply_ravel(ra::expr(f, ARGi, ARGa, ARGd));
            tr.test(std::equal(check, check+12, a.begin()));
        }
        tr.section("driving");
        {
            auto f = [](ra::View<real, 1> a, int i, real & d) { std::iota(a.begin(), a.end(), d); };
            std::fill(a.begin(), a.end(), 0);
            ply_index(ra::expr(f, ARGa, ARGi, ARGd));
            tr.test(std::equal(check, check+12, a.begin()));

            std::fill(a.begin(), a.end(), 0);
            ply_ravel(ra::expr(f, ARGa, ARGi, ARGd));
            tr.test(std::equal(check, check+12, a.begin()));
        }
    }
    tr.section("ply on cell rank = 0 using iter<-1>, ref argument");
    {
        ra::Small<real, 3> dump { 1, 2, 3 };
        ra::Unique<real, 1> a({3}, ra::unspecified);
        real check[3] = {1, 2, 3};
        tr.section("driving");
        {
            auto f = [](real & a, real d) { a = d; };
            std::fill(a.begin(), a.end(), 0);
            ply_index(map(f, a.iter<-1>(), dump.iter<0>()));
            tr.test(std::equal(check, check+3, a.begin()));

            std::fill(a.begin(), a.end(), 0);
            ply_ravel(map(f, a.iter<-1>(), dump.iter<0>()));
            tr.test(std::equal(check, check+3, a.begin()));
        }
    }
    tr.section("ply on cell rank > 0, dynamic rank");
    {
        ra::Big<int> ad({5, 2}, ra::_0 - ra::_1);
        ra::Big<int, 2> as({5, 2}, ra::_0 - ra::_1);
        ra::Big<int, 2> b({5, 2}, (ra::_0 - ra::_1)*2);

        tr.test_eq(1, as.iter<-1>().rank());
        auto cellr = as.iter<-1>().cellr; tr.test_eq(1, cellr);
        tr.test_eq(1, ad.iter<-1>().rank());
        tr.test_eq(ra::RANK_ANY, ad.iter<-1>().rank_s());
        auto e = ra::expr([](auto const & a, auto const & b) { cout << (b-2*a) << endl; },
                          ad.iter<-1>(), b.iter<-1>());
        tr.test_eq(1, e.rank());

        tr.test_eq(0., map([](auto const & a, auto const & b) { return sum(abs(b-2*a)); },
                              as.iter<-1>(), b.iter<-1>()));
        tr.test_eq(0., map([](auto const & a, auto const & b) { return sum(abs(b-2*a)); },
                              ad.iter<-1>(), b.iter<-1>()));
    }
    tr.section("ply on cell rank > 0, static sizes (TODO)");
    {
        ra::Small<int, 2, 3, 4> a(ra::_0 - ra::_1 + ra::_2);
        cout << a << endl;
        cout << "iter " << a.iter() << endl;
        // auto ai = a.iter<1>();
        // cout << "iter<1> " << sizeof(a) << endl;
    }
    tr.section("FYI");
    {
        cout << "..." << sizeof(ra::View<real, 0>) << endl;
        cout << "..." << sizeof(ra::View<real, 1>) << endl;
    }
    return tr.summary();
}
