
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-explode-collapse.C
/// @brief Tests for explode() and collapse().

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/large.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush;

using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);

    section("explode");
    {
        ra::Owned<int, 2> A({2, 3}, ra::_0 - ra::_1);
        auto B = ra::explode<ra::Small<int, 3> >(A);
        tr.test_eq(3, B(0).size_s());
        tr.test_eq(ra::Small<int, 3> {0, -1, -2}, B(0));
        tr.test_eq(ra::Small<int, 3> {1, 0, -1}, B(1));
        B(1) = 9;
        tr.test_eq(ra::Small<int, 3> {0, -1, -2}, B(0));
        tr.test_eq(ra::Small<int, 3> {9, 9, 9}, B(1));
    }
// note that dynamic-rank operator() returns a rank 0 array (since the rank
// cannot be known at compile time). So we have to peel that back.
    {
        ra::Owned<int> A({2, 3}, ra::_0 - ra::_1);
        auto B = ra::explode<ra::Small<int, 3> >(A);
        tr.test_eq(3, (B(0).data())->size_s());
        tr.test_eq(ra::scalar(ra::Small<int, 3> {0, -1, -2}), B(0));
        tr.test_eq(ra::scalar(ra::Small<int, 3> {1, 0, -1}), B(1));
        B(1) = 9;
        tr.test_eq(ra::scalar(ra::Small<int, 3> {0, -1, -2}), B(0));
        tr.test_eq(ra::scalar(ra::Small<int, 3> {9, 9, 9}), B(1));
    }
    section("explode<complex>");
    {
        ra::Owned<real, 3> A({2, 3, 2}, ra::_0 - ra::_1 + ra::_2);
        auto B = ra::explode<complex>(A);
        tr.test_eq(2, B.rank());
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, real_part(B));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  2, 1, 0}, imag_part(B));
        imag_part(B(1)) = 9;
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, A(ra::all, ra::all, 0));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  9, 9, 9}, A(ra::all, ra::all, 1));
    }
    {
        ra::Owned<real> A({2, 3, 2}, ra::_0 - ra::_1 + ra::_2);
        auto B = ra::explode<complex>(A);
        tr.test_eq(2, B.rank());
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, real_part(B));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  2, 1, 0}, imag_part(B));
        imag_part(B(1)) = 9;
        tr.test_eq(ra::Small<real, 2, 3> {0, -1, -2,  1, 0, -1}, A(ra::all, ra::all, 0));
        tr.test_eq(ra::Small<real, 2, 3> {1, 0, -1,  9, 9, 9}, A(ra::all, ra::all, 1));
    }
    section("collapse");
    {
        section("sub is real to super complex");
        {
            auto test_sub_real = [&tr](auto && A)
                {
                    A = ra::cast<double>(ra::_0)*complex(4, 1) + ra::cast<double>(ra::_1)*complex(1, 4);
                    auto B = ra::collapse<double>(A);
                    tr.test_eq(real_part(A), B(ra::all, ra::all, 0));
                    tr.test_eq(imag_part(A), B(ra::all, ra::all, 1));
                };
            test_sub_real(ra::Unique<complex, 2>({4, 4}, ra::unspecified));
            test_sub_real(ra::Unique<complex>({4, 4}, ra::unspecified));
        }
        section("sub is int to super Small of rank 1");
        {
            using r2 = ra::Small<int, 2>;
            auto test_sub_small2 = [&tr](auto && A)
                {
                    A = map([](int i, int j) { return r2 {i+j, i-j}; }, ra::_0, ra::_1);
                    auto B = ra::collapse<int>(A);
                    tr.test_eq(B(ra::all, ra::all, 0), map([](auto && a) { return a(0); }, A));
                    tr.test_eq(B(ra::all, ra::all, 1), map([](auto && a) { return a(1); }, A));
                };
            test_sub_small2(ra::Unique<r2, 2>({4, 4}, ra::unspecified));
            test_sub_small2(ra::Unique<r2>({4, 4}, ra::unspecified));
        }
        section("sub is int to super Small of rank 2");
        {
            using super = ra::Small<int, 2, 3>;
            auto test_sub_small23 = [&tr](auto && A)
                {
                    A = map([](int i, int j) { return super(i-j+ra::_0-ra::_1); }, ra::_0, ra::_1);
                    auto B = ra::collapse<int>(A);
                    for (int i=0; i<super::size(0); ++i) {
                        for (int j=0; j<super::size(1); ++j) {
                            tr.test_eq(B(ra::all, ra::all, i, j), map([i, j](auto && a) { return a(i, j); }, A));
                        }
                    }
                };
            test_sub_small23(ra::Unique<super, 2>({2, 2}, ra::unspecified));
            test_sub_small23(ra::Unique<super>({2, 2}, ra::unspecified));
        }
        section("sub is Small of rank 1 to super Small of rank 2");
        {
            using super = ra::Small<int, 2, 3>;
            auto test_sub_small23 = [&tr](auto && A)
                {
                    A = map([](int i, int j) { return super(i-j+ra::_0-ra::_1); }, ra::_0, ra::_1);
                    using sub = ra::Small<int, 3>;
                    auto B = ra::collapse<sub>(A);
// @TODO sub() is used to cover a problem with where() and SmallSlice/SmallArray, since they convert to each other
                    tr.test_eq(B(ra::all, ra::all, 0), map([](auto && a) { return sub(a(0)); }, A));
                    tr.test_eq(B(ra::all, ra::all, 1), map([](auto && a) { return sub(a(1)); }, A));
                };
            test_sub_small23(ra::Unique<super, 2>({2, 2}, ra::unspecified));
            test_sub_small23(ra::Unique<super>({2, 2}, ra::unspecified));
        }
        section("sub is real to super complex Small of rank 2");
        {
            using super = ra::Small<complex, 2, 2>;
            auto test_sub_real = [&tr](auto && A)
                {
                    A = map([](complex a) { return super { a, conj(a), -conj(a), -a }; },
                            ra::cast<double>(ra::_0)*complex(4, 1) + ra::cast<double>(ra::_1)*complex(1, 4));
                    auto B = ra::collapse<double>(A);
                    for (int i=0; i<super::size(0); ++i) {
                        for (int j=0; j<super::size(1); ++j) {
                            tr.test_eq(B(ra::all, ra::all, i, j, 0), map([i, j](auto && a) { return real_part(a(i, j)); }, A));
                            tr.test_eq(B(ra::all, ra::all, i, j, 1), map([i, j](auto && a) { return imag_part(a(i, j)); }, A));
                        }
                    }
                };
            test_sub_real(ra::Unique<super, 2>({4, 4}, ra::unspecified));
            test_sub_real(ra::Unique<super>({4, 4}, ra::unspecified));
        }
    }
    section("old tests from test-ra-1.C (@TODO remove if redundant)");
    {
        section("super rank 1");
        {
            auto test = [&tr](auto && A)
            {
                auto B = ra::explode<ra::Small<double, 2>>(A);
                for (int i=0; i<3; ++i) {
                    tr.test_eq(i*2, B[i](0));
                    tr.test_eq(i*2+1, B[i](1));
                }
            };
            test(ra::Unique<double, 2>({4, 2}, ra::_0*2 + ra::_1));
            test(ra::Unique<double>({4, 2}, ra::_0*2 + ra::_1));
        }
        section("super rank 0");
        {
#define TEST(CHECK_RANK_S)                                              \
            [&tr](auto && A)                                            \
            {                                                           \
                auto B = ra::explode_<complex, 1>(A);                   \
                static_assert(ra::ra_traits<decltype(B)>::rank_s()==CHECK_RANK_S, "bad static rank"); \
                cout << B << endl;                                      \
                /* @TODO B(0) etc. doesn't get converted to r2x2 & for RANK_ANY, and it should. */ \
                for (int i=0; i<3; ++i) {                               \
                    tr.test_eq(i*2, B[i].real());                       \
                    tr.test_eq(i*2+1, B[i].imag());                     \
                }                                                       \
            }
            TEST(ra::RANK_ANY)(ra::Unique<double>({4, 2}, ra::_0*2 + ra::_1));
            TEST(1)(ra::Unique<double, 2>({4, 2}, ra::_0*2 + ra::_1));
        }
        section("super rank 2");
        {
            using r2x2 = ra::Small<double, 2, 2>;
            auto test = [&tr](auto && A)
            {
                auto B = ra::explode<r2x2>(A);
                tr.test_eq(1, B.rank());
// @TODO B(0) etc. doesn't get converted to r2x2 & for RANK_ANY, and it should.
                tr.test_eq(r2x2 { 0, 1, 2, 3 }, B[0]);
                tr.test_eq(r2x2 { 4, 5, 6, 7 }, B[1]);
                tr.test_eq(r2x2 { 8, 9, 10, 11 }, B[2]);
                tr.test_eq(r2x2 { 12, 13, 14, 15}, B[3]);
            };
            test(ra::Unique<double, 3>({4, 2, 2}, ra::_0*4 + ra::_1*2 + ra::_2));
            test(ra::Unique<double>({4, 2, 2}, ra::_0*4 + ra::_1*2 + ra::_2));
        }
    }
    return tr.summary();
}
