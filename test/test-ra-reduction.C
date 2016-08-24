
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-reduction.C
/// @brief Test array reductions.

#include <iostream>
#include <iterator>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/large.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush; using std::tuple;
using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);

    section("amax with different expr types");
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

    section("every / any");
    {
        tr.test(every(ra::Unique<real, 2>({4, 4}, 10+ra::_0-ra::_1)));
        tr.test(any(ra::Unique<real, 2>({4, 4}, ra::_0-ra::_1)));
        tr.test(every(true));
        tr.test(!every(false));
        tr.test(any(true));
        tr.test(!any(false));

        tr.test(every(ra::Unique<int, 1> {5, 5}==5));
        tr.test(!every(ra::Unique<int, 1> {2, 5}==5));
        tr.test(!every(ra::Unique<int, 1> {5, 2}==5));
        tr.test(!every(ra::Unique<int, 1> {2, 3}==5));

        tr.test(any(ra::Unique<int, 1> {5, 5}==5));
        tr.test(any(ra::Unique<int, 1> {2, 5}==5));
        tr.test(any(ra::Unique<int, 1> {5, 2}==5));
        tr.test(!any(ra::Unique<int, 1> {2, 3}==5));
    }

    section("norm2");
    {
        ra::Small<real, 2> a {1, 2};
        tr.test_abs_error(std::sqrt(5.), norm2(a), 1e-15);
    }

    section("normv");
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

    section("reductions");
    {
        auto test_dot = [&tr](auto && test) // @TODO Use this for other real reductions.
            {
                test(ra::Small<complex, 2>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Small<complex, 2>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Small<complex, 2>{1, 2}, ra::Small<complex, 2>{3, 4});

                test(ra::Owned<complex, 1>{1, 2}, ra::Owned<real, 1>{3, 4});
                test(ra::Owned<real, 1>{1, 2}, ra::Owned<complex, 1>{3, 4});
                test(ra::Owned<real, 1>{1, 2}, ra::Owned<real, 1>{3, 4});
                test(ra::Owned<complex, 1>{1, 2}, ra::Owned<complex, 1>{3, 4});

                test(ra::Small<complex, 2>{1, 2}, ra::Owned<real, 1>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Owned<complex, 1>{3, 4});
                test(ra::Small<real, 2>{1, 2}, ra::Owned<real, 1>{3, 4});
                test(ra::Small<complex, 2>{1, 2}, ra::Owned<complex, 1>{3, 4});

                test(ra::Owned<complex, 1>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Owned<real, 1>{1, 2}, ra::Small<complex, 2>{3, 4});
                test(ra::Owned<real, 1>{1, 2}, ra::Small<real, 2>{3, 4});
                test(ra::Owned<complex, 1>{1, 2}, ra::Small<complex, 2>{3, 4});
            };
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(11., dot(a, b)); });
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(11., cdot(a, b)); });
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(sqrt(8.), norm2(a-b)); });
        test_dot([&tr](auto && a, auto && b) { tr.test_eq(8., reduce_sqrm(a-b)); });

        auto test_cdot = [&tr](auto && test)
            {
                test(ra::Small<complex, 2>{1, complex(2, 3)}, ra::Small<complex, 2>{complex(4, 5), 6});
                test(ra::Owned<complex, 1>{1, complex(2, 3)}, ra::Small<complex, 2>{complex(4, 5), 6});
                test(ra::Small<complex, 2>{1, complex(2, 3)}, ra::Owned<complex, 1>{complex(4, 5), 6});
                test(ra::Owned<complex, 1>{1, complex(2, 3)}, ra::Owned<complex, 1>{complex(4, 5), 6});
            };
        complex value = conj(1.)*complex(4., 5.) + conj(complex(2., 3.))*6.;
        tr.test_eq(value, complex(16, -13));
        test_cdot([&tr](auto && a, auto && b) { tr.test_eq(complex(16., -13.), cdot(a, b)); });
        test_cdot([&tr](auto && a, auto && b) { tr.test_eq(sqrt(59.), norm2(a-b)); });
        test_cdot([&tr](auto && a, auto && b) { tr.test_eq(59., reduce_sqrm(a-b)); });

        auto test_sum = [&tr](auto && test)
            {
                test(ra::Small<complex, 2>{complex(4, 5), 6});
                test(ra::Owned<complex, 1>{complex(4, 5), 6});
            };
        test_sum([&tr](auto && a) { tr.test_eq(complex(10, 5), sum(a)); });
        test_sum([&tr](auto && a) { tr.test_eq(complex(24, 30), prod(a)); });
        test_sum([&tr](auto && a) { tr.test_eq(sqrt(41.), amax(abs(a))); });
        test_sum([&tr](auto && a) { tr.test_eq(6., amin(abs(a))); });
    }

    section("amax/amin ignore NaN");
    {
        tr.test_eq(std::numeric_limits<real>::lowest(), std::max(std::numeric_limits<real>::lowest(), QNAN));
        tr.test_eq(std::numeric_limits<real>::lowest(), amax(ra::Small<real, 3>(QNAN)));
        tr.test_eq(std::numeric_limits<real>::max(), amin(ra::Small<real, 3>(QNAN)));
    }

// @TODO these reductions require a destination argument; there are no exprs really.
    section("to sum columns in crude ways");
    {
        ra::Unique<real, 2> A({100, 111}, ra::_0 - ra::_1);

        ra::Unique<real, 1> B({100}, 0.);
        for (int i=0, iend=A.size(0); i<iend; ++i) {
            B(i) = sum(A(i));
        }

        {
            ra::Unique<real, 1> C({100}, 0.);
            for_each([](auto & c, auto a) { c += a; }, C, A);
            tr.quiet().test_eq(B, C);
        }
// This depends on matching frames for += just as for any other op, which is at odds with
// e.g. amend.
        {
            ra::Unique<real, 1> C({100}, 0.);
            C += A;
            tr.quiet().test_eq(B, C);
        }
// It cannot work with a lhs scalar value since += must be a class member, but it will work
// with a rank 0 array.
        {
            ra::Unique<real, 0> C({}, 0.);
            C += A(0);
            tr.quiet().test_eq(B(0), C);
        }
// This will fail because the assumed driver (RANK_ANY) has lower actual rank than the other argument. @TODO check that it fails.
        // {
        //     ra::Unique<real, 2> A({2, 3}, {1, 2, 3, 4 ,5, 6});
        //     ra::Unique<real> C({}, 0.);
        //     C += A(0);
        // }
    }
    return tr.summary();

// @TODO as above, but also missing sugar. for_each(verb, C, A) should work.
    section("to sum rows in crude ways");
    {
        ra::Unique<real, 2> A({100, 111}, ra::_0 - ra::_1);

        ra::Unique<real, 1> B({111}, 0.);
        for (int j=0, jend=A.size(1); j<jend; ++j) {
            B(j) = sum(A(ra::all, j));
        }

        {
            ra::Unique<real, 1> C({111}, 0.);
            for_each([&C](auto && a) { C += a; }, A.iter<1>());
            tr.quiet().test_eq(B, C);
        }
        {
            ra::Unique<real, 1> C({111}, 0.);
            ply_either(ra::ryn(ra::verb<1, 1>::make([](auto & c, auto && a) { c += a; }), C.iter(), A.iter()));
            tr.quiet().test_eq(B, C);
        }
        {
            ra::Unique<real, 1> C({111}, 0.);
            ply_either(ra::ryn(ra::wrank<1, 1>::make(ra::verb<0, 0>::make([](auto & c, auto a) { c += a; })), C.iter(), A.iter()));
            tr.quiet().test_eq(B, C);
        }
    }

    return tr.summary();
}
