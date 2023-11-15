// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Array reductions.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;
using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);

    tr.section("amax with different expr types");
    {
        auto test_amax_expr = [&tr](auto && a, auto && b)
            {
                a = ra::Small<real, 2, 2> {1, 2, 9, -10};
                tr.test_eq(amax(a), 9);
                b = ra::Small<real, 2, 2> {1, 1, 1, 1};
                tr.test_eq(amax(a+b), 10);
            };
        test_amax_expr(ra::Unique<real, 2>({2, 2}, 0.), ra::Unique<real, 2>({2, 2}, 0.));
        test_amax_expr(ra::Small<real, 2, 2>(), ra::Small<real, 2, 2>());
// failed in gcc 5.1 when amax() took its args by plain auto (now auto &&).
        test_amax_expr(ra::Unique<real, 2>({2, 2}, 0.), ra::Small<real, 2, 2>());
    }

    tr.section("every / any");
    {
        tr.test(every(ra::Unique<real, 2>({4, 4}, 10+ra::_0-ra::_1)));
        tr.test(any(ra::Unique<real, 2>({4, 4}, ra::_0-ra::_1)));
        tr.test(ra::every(true));
        tr.test(!ra::every(false));
        tr.test(ra::any(true));
        tr.test(!ra::any(false));

        tr.test(every(ra::Unique<int, 1> {5, 5}==5));
        tr.test(!every(ra::Unique<int, 1> {2, 5}==5));
        tr.test(!every(ra::Unique<int, 1> {5, 2}==5));
        tr.test(!every(ra::Unique<int, 1> {2, 3}==5));

        tr.test(any(ra::Unique<int, 1> {5, 5}==5));
        tr.test(any(ra::Unique<int, 1> {2, 5}==5));
        tr.test(any(ra::Unique<int, 1> {5, 2}==5));
        tr.test(!any(ra::Unique<int, 1> {2, 3}==5));
    }

    tr.section("norm2");
    {
        ra::Small<real, 2> a {1, 2};
        tr.test_abs(std::sqrt(5.), norm2(a), 1e-15);
        ra::Small<float, 2> b {1, 2};
        tr.test_abs(std::sqrt(5.f), norm2(b), 4e-8);
        tr.info("type of norm2(floats)").test(std::is_same_v<float, decltype(norm2(b))>);
        tr.info("type of reduce_sqrm(floats)").test(std::is_same_v<float, decltype(reduce_sqrm(b))>);
        tr.info("type of sqrm(floats)").test(std::is_same_v<float, decltype(sqrm(b[0]))>);
    }

    tr.section("normv");
    {
        ra::Small<real, 2> a {1, 2};
        ra::Small<real, 2> b;

        b = normv(a);
        cout << "normv of lvalue: " << b << endl;
        tr.test_eq(b[0], 1./sqrt(5));
        tr.test_eq(b[1], 2./sqrt(5));

        b = normv(ra::Small<real, 2> {2, 1});
        cout << "normv of rvalue: "<< b << endl;
        tr.test_eq(b[0], 2./sqrt(5));
        tr.test_eq(b[1], 1./sqrt(5));
    }

    tr.section("reductions");
    {
        auto test_dot = [](auto && test) // TODO Use this for other real reductions.
            {
                test(ra::Small<complex, 2>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Small<complex, 2>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Small<complex, 2>{1, 2}, ra::Small<complex, 2>{3, 4});

                test(ra::Big<complex, 1>{1, 2}, ra::Big<real, 1>{3, 4});
                test(ra::Big<real, 1>{1, 2}, ra::Big<complex, 1>{3, 4});
                test(ra::Big<real, 1>{1, 2}, ra::Big<real, 1>{3, 4});
                test(ra::Big<complex, 1>{1, 2}, ra::Big<complex, 1>{3, 4});

                test(ra::Small<complex, 2>{1, 2}, ra::Big<real, 1>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Big<complex, 1>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Big<real, 1>{3, 4});
                test(ra::Small<complex, 2>{1, 2}, ra::Big<complex, 1>{3, 4});

                test(ra::Big<complex, 1>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Big<real, 1>{1, 2}, ra::Small<complex, 2>{3, 4});
                test(ra::Big<real, 1>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Big<complex, 1>{1, 2}, ra::Small<complex, 2>{3, 4});
            };
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(11., dot(a, b)); });
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(11., cdot(a, b)); });
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(sqrt(8.), norm2(a-b)); });
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(8., reduce_sqrm(a-b)); });

        auto test_cdot = [](auto && test)
            {
                test(ra::Small<complex, 2>{1, complex(2, 3)}, ra::Small<complex, 2>{complex(4, 5), 6});
                test(ra::Big<complex, 1>{1, complex(2, 3)}, ra::Small<complex, 2>{complex(4, 5), 6});
                test(ra::Small<complex, 2>{1, complex(2, 3)}, ra::Big<complex, 1>{complex(4, 5), 6});
                test(ra::Big<complex, 1>{1, complex(2, 3)}, ra::Big<complex, 1>{complex(4, 5), 6});
            };
        complex value = conj(1.)*complex(4., 5.) + conj(complex(2., 3.))*6.;
        tr.test_eq(value, complex(16, -13));
        test_cdot([&tr](auto && a, auto && b) { tr.test_eq(complex(16., -13.), cdot(a, b)); });
        test_cdot([&tr](auto && a, auto && b) { tr.test_eq(sqrt(59.), norm2(a-b)); });
        test_cdot([&tr](auto && a, auto && b) { tr.test_eq(59., reduce_sqrm(a-b)); });

        auto test_sum = [](auto && test)
            {
                test(ra::Small<complex, 2>{complex(4, 5), 6});
                test(ra::Big<complex, 1>{complex(4, 5), 6});
            };
        test_sum([&tr](auto && a) { tr.test_eq(complex(10, 5), sum(a)); });
        test_sum([&tr](auto && a) { tr.test_eq(complex(24, 30), prod(a)); });
        test_sum([&tr](auto && a) { tr.test_eq(sqrt(41.), amax(abs(a))); });
        test_sum([&tr](auto && a) { tr.test_eq(6., amin(abs(a))); });
    }

    tr.section("amax/amin ignore NaN");
    {
        constexpr real QNAN = std::numeric_limits<real>::quiet_NaN();
        tr.test_eq(std::numeric_limits<real>::lowest(), std::max(std::numeric_limits<real>::lowest(), QNAN));
        tr.test_eq(-std::numeric_limits<real>::infinity(), amax(ra::Small<real, 3>(QNAN)));
        tr.test_eq(std::numeric_limits<real>::infinity(), amin(ra::Small<real, 3>(QNAN)));
    }

// TODO these reductions require a destination argument; there are no exprs really.
    tr.section("to sum columns in crude ways");
    {
        ra::Unique<real, 2> A({100, 111}, ra::_0 - ra::_1);

        ra::Unique<real, 1> B({100}, 0.);
        for (int i=0, iend=A.len(0); i<iend; ++i) {
            B(i) = sum(A(i));
        }

        {
            ra::Unique<real, 1> C({100}, 0.);
            for_each([](auto & c, auto a) { c += a; }, C, A);
            tr.test_eq(B, C);
        }
// This depends on matching frames for += just as for any other op, which is at odds with e.g. amend.
        {
            ra::Unique<real, 1> C({100}, 0.);
            C += A;
            tr.test_eq(B, C);
        }
// Same as above.
        {
            ra::Unique<real, 1> C({100}, 0.);
            C =  C + A;
            tr.test_eq(B, C);
        }
// It cannot work with a lhs scalar value since += must be a class member, but it will work with a rank 0 array or with ra::Scalar.
        {
            ra::Unique<real, 0> C({}, 0.);
            C += A(0);
            tr.test_eq(B(0), C);
            real c(0.);
            ra::scalar(c) += A(0);
            tr.test_eq(B(0), c);
        }
// This will fail because the assumed driver (ANY) has lower actual rank than the other argument. TODO check that it fails.
        // {
        //     ra::Unique<real, 2> A({2, 3}, {1, 2, 3, 4 ,5, 6});
        //     ra::Unique<real> C({}, 0.);
        //     C += A(0);
        // }
    }
    tr.section("to sum rows in crude ways");
    {
        ra::Unique<real, 2> A({100, 111}, ra::_0 - ra::_1);
        ra::Unique<real, 1> B({111}, 0.);
        for (int j=0, jend=A.len(1); j<jend; ++j) {
            B(j) = sum(A(ra::all, j));
        }

        {
            ra::Unique<real, 1> C({111}, 0.);
            for_each([&C](auto && a) { C += a; }, A.iter<1>());
            tr.info("rhs iterator of rank > 0").test_eq(B, C);
        }
        {
            ra::Unique<real, 1> C({111}, 0.);
            for_each(ra::wrank<1, 1>([](auto & c, auto && a) { c += a; }), C, A);
            tr.info("rank conjuction").test_eq(B, C);
        }
        {
            ra::Unique<real, 1> C({111}, 0.);
            for_each(ra::wrank<1, 1>(ra::wrank<0, 0>([](auto & c, auto a) { c += a; })), C, A);
            tr.info("double rank conjunction").test_eq(B, C);
        }
        {
            ra::Unique<real, 1> C({111}, 0.);
            ra::scalar(C) += A.iter<1>();
            tr.info("scalar() and iterators of rank > 0").test_eq(B, C);
        }
        {
            ra::Unique<real, 1> C({111}, 0.);
            C.iter<1>() += A.iter<1>();
            tr.info("assign to iterators of rank > 0").test_eq(B, C);
        }
    }
    tr.section("reductions with amax");
    {
        ra::Big<int, 2> c({2, 3}, {1, 3, 2, 7, 1, 3});
        tr.info("max of rows").test_eq(ra::Big<int, 1> {3, 7}, map([](auto && a) { return amax(a); }, iter<1>(c)));
        ra::Big<int, 1> m({3}, 0);
        scalar(m) = max(scalar(m), iter<1>(c)); // requires inner forward in ra.hh: DEF_NAME_OP
        tr.info("max of columns I").test_eq(ra::Big<int, 1> {7, 3, 3}, m);
        m = 0;
        iter<1>(m) = max(iter<1>(m), iter<1>(c)); // FIXME
        tr.info("max of columns III [ma113]").test_eq(ra::Big<int, 1> {7, 3, 3}, m);
        m = 0;
        for_each([&m](auto && a) { m = max(m, a); }, iter<1>(c));
        tr.info("max of columns II").test_eq(ra::Big<int, 1> {7, 3, 3}, m);
        ra::Big<double, 1> q({0}, {});
        tr.info("amax default").test_eq(std::numeric_limits<double>::infinity(), amin(q));
        tr.info("amin default").test_eq(-std::numeric_limits<double>::infinity(), amax(q));
    }
    tr.section("vector-matrix reductions");
    {
        auto test = [&tr](auto t, auto s, auto r)
            {
                using T = decltype(t);
                using S = decltype(s);
                using R = decltype(r);
                S x[4] = {1, 2, 3, 4};
                ra::Small<T, 3, 4> a = ra::_0 - ra::_1;
                R y[3] = {99, 99, 99};
                ra::start(y) = ra::gemv(a, x);
                auto z = ra::gemv(a, x);
                tr.test_eq(ra::Small<R, 3> {-20, -10, 0}, y);
                tr.test_eq(ra::Small<R, 3> {-20, -10, 0}, z);
            };
        test(double(0), double(0), double(0));
        test(std::complex<double>(0), std::complex<double>(0), std::complex<double>(0));
        test(int(0), int(0), int(0));
        test(int(0), double(0), double(0));
        test(double(0), int(0), double(0));
    }
    {
        auto test = [&tr](auto t, auto s, auto r)
            {
                using T = decltype(t);
                using S = decltype(s);
                using R = decltype(r);
                S x[4] = {1, 2, 3, 4};
                ra::Small<T, 4, 3> a = ra::_1 - ra::_0;
                R y[3] = {99, 99, 99};
                ra::start(y) = ra::gevm(x, a);
                auto z = ra::gevm(x, a);
                tr.test_eq(ra::Small<R, 3> {-20, -10, 0}, y);
                tr.test_eq(ra::Small<R, 3> {-20, -10, 0}, z);
            };
        test(double(0), double(0), double(0));
        test(std::complex<double>(0), std::complex<double>(0), std::complex<double>(0));
        test(int(0), int(0), int(0));
        test(int(0), double(0), double(0));
        test(double(0), int(0), double(0));
    }
    tr.section("matrix-matrix reductions");
    {
        ra::Big<double, 2> A({0, 0}, 0.);
        ra::Big<double, 2> B({0, 0}, 0.);
        auto C = gemm(A, B);
        tr.test_eq(0, C.len(0));
        tr.test_eq(0, C.len(1));
    }
    tr.section("reference reductions");
    {
        ra::Big<double, 2> A({2, 3}, ra::_1 - ra::_0);
        double & mn = refmin(A);
        tr.test_eq(-1, mn);
        mn = -99;
        ra::Big<double, 2> B({2, 3}, ra::_1 - ra::_0);
        B(1, 0) = -99;
        tr.test_eq(B, A);
        double & mx = refmin(A, std::greater<double>());
        tr.test_eq(2, mx);
        mx = 0;
        B(0, 2) = 0;
        tr.test_eq(B, A);
        double & my = refmax(A);
        tr.test_eq(1, my);
        my = 77;
        B(0, 1) = 77;
        tr.test_eq(B, A);
        // cout << refmin(A+B) << endl; // compile error
    }
    return tr.summary();
}
