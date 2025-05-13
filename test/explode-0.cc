// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for explode() and collapse().

// (c) Daniel Llorens - 2013-2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;
using complex = std::complex<double>;
using ra::real_part, ra::imag_part;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("explode");
    {
        ra::Big<int, 2> A({2, 3}, ra::_0 - ra::_1);
        ra::ViewBig<ra::Small<int, 3>, 1> B = ra::explode<ra::Small<int, 3>>(A);
        // std::println(cout, "A:\n{:p}\nB:\n{:p:l}", A, B);
        // A *= -1;
        // std::println(cout, "A:\n{:p}\nB:\n{:p:l}", A, B);
        tr.test_eq(3, ra::size_s<decltype(B(0))>());
        tr.test_eq(ra::Small<int, 3> {0, -1, -2}, B(0));
        tr.test_eq(ra::Small<int, 3> {1, 0, -1}, B(1));
        B(1) = 9;
        tr.test_eq(ra::Small<int, 3> {0, -1, -2}, B(0));
        tr.test_eq(ra::Small<int, 3> {9, 9, 9}, B(1));
    }
// note that dynamic-rank operator() returns a rank 0 array (since the rank
// cannot be known at compile time). So we have to peel that back.
    {
        ra::Big<int> A({2, 3}, ra::_0 - ra::_1);
        auto B = ra::explode<ra::Small<int, 3>>(A);
        tr.test_eq(3, ra::size_s<decltype(*(B(0).data()))>());
        tr.test_eq(ra::scalar(ra::Small<int, 3> {0, -1, -2}), B(0));
        tr.test_eq(ra::scalar(ra::Small<int, 3> {1, 0, -1}), B(1));
        B(1) = 9;
        tr.test_eq(ra::scalar(ra::Small<int, 3> {0, -1, -2}), B(0));
        tr.test_eq(ra::scalar(ra::Small<int, 3> {9, 9, 9}), B(1));
    }
    tr.section("explode<complex>");
    {
        ra::Big<real, 3> A({2, 3, 2}, ra::_0 - ra::_1 + ra::_2);
        ra::ViewBig<complex, 2> B = ra::explode<complex>(A);
        // std::println(cout, "A:\n{:p}\nB:\n{:p}", A, B);
        tr.test_eq(2, B.rank());
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, real_part(B));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  2, 1, 0}, imag_part(B));
        imag_part(B(1)) = 9;
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, A(ra::all, ra::all, 0));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  9, 9, 9}, A(ra::all, ra::all, 1));
    }
    {
        ra::Big<real> A({2, 3, 2}, ra::_0 - ra::_1 + ra::_2);
        auto B = ra::explode<complex>(A);
        tr.test_eq(2, B.rank());
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, real_part(B));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  2, 1, 0}, imag_part(B));
        imag_part(B(1)) = 9;
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, A(ra::all, ra::all, 0));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  9, 9, 9}, A(ra::all, ra::all, 1));
    }
    tr.section("collapse");
    {
        tr.section("sub is real to super complex");
        {
            auto test_sub_real = [&tr](auto && A)
                {
                    A = ra::cast<double>(ra::_0)*complex(4, 1) + ra::cast<double>(ra::_1)*complex(1, 4);
                    auto B = ra::collapse<double>(A);
                    tr.test_eq(real_part(A), B(ra::all, ra::all, 0));
                    tr.test_eq(imag_part(A), B(ra::all, ra::all, 1));
                };
            test_sub_real(ra::Unique<complex, 2>({4, 4}, ra::none));
            test_sub_real(ra::Unique<complex>({4, 4}, ra::none));
        }
        tr.section("sub is int to super Small of rank 1");
        {
            using r2 = ra::Small<int, 2>;
            auto test_sub_small2 = [&tr](auto && A)
                {
                    A = map([](int i, int j) { return r2 {i+j, i-j}; }, ra::_0, ra::_1);
                    auto B = ra::collapse<int>(A);
                    tr.test_eq(B(ra::all, ra::all, 0), map([](auto && a) { return a(0); }, A));
                    tr.test_eq(B(ra::all, ra::all, 1), map([](auto && a) { return a(1); }, A));
                };
            test_sub_small2(ra::Unique<r2, 2>({4, 4}, ra::none));
            test_sub_small2(ra::Unique<r2>({4, 4}, ra::none));
        }
        tr.section("sub is int to super Small of rank 2");
        {
            using super = ra::Small<int, 2, 3>;
            auto test_sub_small23 = [&tr](auto && A)
                {
                    A = map([](int i, int j) { return super(i-j+ra::_0-ra::_1); }, ra::_0, ra::_1);
                    auto B = ra::collapse<int>(A);
                    for (int i=0; i<super::len(0); ++i) {
                        for (int j=0; j<super::len(1); ++j) {
                            tr.test_eq(B(ra::all, ra::all, i, j), map([i, j](auto && a) { return a(i, j); }, A));
                        }
                    }
                };
            test_sub_small23(ra::Unique<super, 2>({2, 2}, ra::none));
            test_sub_small23(ra::Unique<super>({2, 2}, ra::none));
        }
        tr.section("sub is Small of rank 1 to super Small of rank 2");
        {
            using super = ra::Small<int, 2, 3>;
            auto test_sub_small23 = [&tr](auto && A)
                {
                    A = map([](int i, int j) { return super(i-j+ra::_0-ra::_1); }, ra::_0, ra::_1);
                    using sub = ra::Small<int, 3>;
                    auto B = ra::collapse<sub>(A);
// TODO sub() is used to cover a problem with where() and ViewSmall/SmallArray, since they convert to each other
                    tr.test_eq(B(ra::all, ra::all, 0), map([](auto && a) { return sub(a(0)); }, A));
                    tr.test_eq(B(ra::all, ra::all, 1), map([](auto && a) { return sub(a(1)); }, A));
                };
            test_sub_small23(ra::Unique<super, 2>({2, 2}, ra::none));
            test_sub_small23(ra::Unique<super>({2, 2}, ra::none));
        }
        tr.section("sub is real to super complex Small of rank 2");
        {
            using super = ra::Small<complex, 2, 2>;
            auto test_sub_real = [&tr](auto && A)
                {
                    A = map([](complex a) { return super { a, conj(a), -conj(a), -a }; },
                            ra::cast<double>(ra::_0)*complex(4, 1) + ra::cast<double>(ra::_1)*complex(1, 4));
                    auto B = ra::collapse<double>(A);
                    for (int i=0; i<super::len(0); ++i) {
                        for (int j=0; j<super::len(1); ++j) {
                            tr.test_eq(B(ra::all, ra::all, i, j, 0), map([i, j](auto && a) { return real_part(a(i, j)); }, A));
                            tr.test_eq(B(ra::all, ra::all, i, j, 1), map([i, j](auto && a) { return imag_part(a(i, j)); }, A));
                        }
                    }
                };
            test_sub_real(ra::Unique<super, 2>({4, 4}, ra::none));
            test_sub_real(ra::Unique<super>({4, 4}, ra::none));
        }
    }
    tr.section("super rank 1");
    {
        auto test = [&tr](auto && A)
        {
            using T = ra::Small<double, 2>;
            auto B = ra::explode<T>(A);
            for (int i=0; i<3; ++i) {
                tr.test_eq(i*2, ((T &)(B(i)))(0));
                tr.test_eq(i*2+1, ((T &)(B(i)))(1));
            }
        };
        test(ra::Unique<double, 2>({4, 2}, ra::_0*2 + ra::_1));
        test(ra::Unique<double>({4, 2}, ra::_0*2 + ra::_1));
    }
    tr.section("super rank 0");
    {
#define TEST(CHECK_RANK_S)                                              \
        [&tr](auto && A)                                                \
            {                                                           \
                using T = complex;                                      \
                auto convtest = [](T & x) -> T & { return x; };         \
                auto B = ra::explode<T>(A);                             \
                static_assert(rank_s(B)==CHECK_RANK_S, "bad static rank"); \
                cout << B << endl;                                      \
                for (int i=0; i<3; ++i) {                               \
                    tr.test_eq(i*2, real_part((T &)(B(i))));            \
                    tr.test_eq(i*2+1, imag_part((T &)(B(i))));          \
                    tr.test_eq(i*2, convtest(B(i)).real());             \
                    tr.test_eq(i*2+1, convtest(B(i)).imag());           \
                }                                                       \
            }
        TEST(ra::ANY)(ra::Unique<double>({4, 2}, ra::_0*2 + ra::_1));
        TEST(1)(ra::Unique<double, 2>({4, 2}, ra::_0*2 + ra::_1));
    }
    tr.section("super rank 2");
    {
        auto test = [&tr](auto && A)
        {
            using T = ra::Small<double, 2, 2>;
            auto B = ra::explode<T>(A);
            tr.test_eq(1, B.rank());
            tr.test_eq(T { 0, 1, 2, 3 }, (T &)(B[0]));
            tr.test_eq(T { 4, 5, 6, 7 }, (T &)(B[1]));
            tr.test_eq(T { 8, 9, 10, 11 }, (T &)(B[2]));
            tr.test_eq(T { 12, 13, 14, 15}, (T &)(B[3]));
        };
        test(ra::Unique<double, 3>({4, 2, 2}, ra::_0*4 + ra::_1*2 + ra::_2));
        test(ra::Unique<double>({4, 2, 2}, ra::_0*4 + ra::_1*2 + ra::_2));
    }
    tr.section("explode for Small/T");
    {
        ra::Small<double, 2, 3> a(ra::_0 + 10*ra::_1);
        auto c = ra::explode<ra::Small<double, 3>>(a);
        static_assert(1==ssize(c.dimv));
        tr.test_eq(2, c.dimv[0].len);
        tr.test_eq(1, c.dimv[0].step);
        tr.test_eq(ra::scalar(a[0].data()), ra::scalar(c[0].data()));
        tr.test_eq(ra::scalar(a[1].data()), ra::scalar(c[1].data()));
        c[1] = { 3, 2, 1 };
        tr.test_eq(ra::Small<double, 3> { 0, 10, 20 }, c[0]);
        tr.test_eq(ra::Small<double, 3> { 0, 10, 20 }, a[0]);
        tr.test_eq(ra::Small<double, 3> { 3, 2, 1 }, a[1]);
    }
    tr.section("explode for Small/complex<T>");
    {
        ra::Small<double, 3, 2> a(ra::_0 + 10*ra::_1);
        auto c = ra::explode<complex>(a);
        static_assert(1==ssize(c.dimv));
        tr.test_eq(3, c.dimv[0].len);
        tr.test_eq(1, c.dimv[0].step);
        tr.test_eq(a(ra::all, 0), real_part(c));
        tr.test_eq(a(ra::all, 1), imag_part(c));
        c = { {3, 0}, {2, 1}, {1, 2} };
        tr.test_eq(ra::Small<double, 3> { 3, 2, 1 }, a(ra::all, 0));
        tr.test_eq(ra::Small<double, 3> { 0, 1, 2 }, a(ra::all, 1));
    }
    tr.section("explode for Small/rank/complex<T>");
    {
        ra::Small<double, 2, 3, 2> a(ra::_0 + 10*ra::_1 + 100*ra::_2);
        auto c = ra::explode<ra::Small<complex, 3>>(a);
        static_assert(1==ssize(c.dimv));
        tr.test_eq(2, c.dimv[0].len);
        tr.test_eq(1, c.dimv[0].step);
        tr.test_eq(a(0, ra::all, 0), real_part(c(0)));
        tr.test_eq(a(1, ra::all, 0), real_part(c(1)));
        tr.test_eq(a(0, ra::all, 1), imag_part(c(0)));
        tr.test_eq(a(1, ra::all, 1), imag_part(c(1)));
        c[0] = { {3, 0}, {2, 1}, {1, 2} };
        c[1] = { {6, 7}, {4, 3}, {5, 9} };
        tr.test_eq(ra::Small<double, 2, 3, 2> { { {3, 0}, {2, 1}, {1, 2} }, { {6, 7}, {4, 3}, {5, 9} } }, a);
    }
    return tr.summary();
}
