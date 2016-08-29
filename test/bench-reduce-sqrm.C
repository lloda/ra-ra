
// (c) Daniel Llorens - 2011

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-reduce-sqrm.H
/// @brief Benchmark for reduce_sqrm with various array types.

#include <iostream>
#include <iomanip>
#include "ra/timer.H"
#include "ra/large.H"
#include "ra/small.H"
#include "ra/operators.H"
#include "ra/real.H"

using std::cout; using std::endl; using std::setw; using std::setprecision;

int const N = 2000000;
ra::Small<ra::dim_t, 1> S1 { 24*24 };
ra::Small<ra::dim_t, 2> S2 { 24, 24 };
ra::Small<ra::dim_t, 3> S3 { 8, 8, 9 };
using real = double;
using real4 = ra::Small<real, 4>;

template <class T>
void report(std::ostream & o, std::string const & type,
            std::string const & method, Timer const & t, T const & val)
{
    o << setw(20) << type << setw(20) << method
      << " usr " << setprecision(4) << setw(6) << t.usr()/N*1e6
      << " wall " << setprecision(4) << setw(6) << t.wall()/N*1e6
      << " val " << val << endl;
}

template <class A, class B>
void by_expr(std::string const & s, Timer & t, A const & a, B const & b)
{
    t.start();
    real x(0.);
    for (int i=0; i<N; ++i) {
        x += reduce_sqrm(a-b);
    }
    t.stop();
    report(cout, s, "expr", t, x);
}

// sqrm+reduction in one op.
template <class A, class B>
void by_traversal(std::string const & s, Timer & t, A const & a, B const & b)
{
    t.start();
    real x(0.);
    for (int i=0; i<N; ++i) {
        real y(0.);
        ra::ply_either(ra::expr([&y](real const a, real const b) { y += sqrm(a, b); },
                                   ra::start(a), ra::start(b)));
        x += y;
    }
    t.stop();
    report(cout, s, "ply nested 1", t, x);
}

// separate reduction: compare abstraction penalty with by_traversal.
template <class A, class B>
void by_traversal2(std::string const & s, Timer & t, A const & a, B const & b)
{
    t.start();
    real x(0.);
    for (int i=0; i<N; ++i) {
        real y(0.);
        ra::ply_either(ra::expr([&y](real const a) { y += a; },
                                   ra::expr([](real const a, real const b) { return sqrm(a, b); },
                                               ra::start(a), ra::start(b))));
        x += y;
    }
    t.stop();
    report(cout, s, "ply nested 2", t, x);
}

template <class A, class B>
std::enable_if_t<A::rank()==1, void>
by_raw(std::string const & s, Timer & t, A const & a, B const & b)
{
    t.start();
    real x(0.);
    for (int i=0; i<N; ++i) {
        real y(0.);
        for (int j=0; j<S1[0]; ++j) {
            y += sqrm(a(j)-b(j));
        }
        x += y;
    }
    t.stop();
    report(cout, s, "raw", t, x);
}

template <class A, class B>
std::enable_if_t<A::rank()==2, void>
by_raw(std::string const & s, Timer & t, A const & a, B const & b)
{
    t.start();
    real x(0.);
    for (int i=0; i<N; ++i) {
        real y(0.);
        for (int j=0; j<S2[0]; ++j) {
            for (int k=0; k<S2[1]; ++k) {
                y += sqrm(a(j, k)-b(j, k));
            }
        }
        x += y;
    }
    t.stop();
    report(cout, s, "raw", t, x);
}

template <class A, class B>
std::enable_if_t<A::rank()==3, void>
by_raw(std::string const & s, Timer & t, A const & a, B const & b)
{
    t.start();
    real x(0.);
    for (int i=0; i<N; ++i) {
        real y(0.);
        for (int j=0; j<S3[0]; ++j) {
            for (int k=0; k<S3[1]; ++k) {
                for (int l=0; l<S3[2]; ++l) {
                    y += sqrm(a(j, k, l)-b(j, k, l));
                }
            }
        }
        x += y;
    }
    t.stop();
    report(cout, s, "raw", t, x);
}

int main()
{
    Timer t;
    {
        real4 A(7.);
        real4 B(3.);
        t.start();
        real x(0.);
        for (int i=0; i<N*S1[0]/4; ++i) {
            real y(0.);
            for (int j=0; j!=4; ++j) {
                y += sqrm(A(j)-B(j));
            }
            x += y;
        }
        t.stop();
        report(cout, "real4", "raw", t, x);
    }
    {
        real4 A(7.);
        real4 B(3.);
        t.start();
        real x(0.);
        for (int i=0; i<N*S1[0]/4; ++i) {
            x += reduce_sqrm(A-B);
        }
        t.stop();
        report(cout, "real4", "expr", t, x);
    }
    {
        ra::Unique<real, 1> A(S1, 7.);
        ra::Unique<real, 1> B(S1, 3.);
        t.start();
        real x(0.);
        for (int i=0; i<N; ++i) {
            real y(0.);
            real const * a = &A[0];
            real const * b = &B[0];
            for (int j=0; j<S1[0]; ++j) {
                y += sqrm(a[j]-b[j]);
            }
            x += y;
        }
        t.stop();
        report(cout, "C array", "raw", t, x);
    }
    {
        ra::Unique<real, 1> A(S1, 7.);
        ra::Unique<real, 1> B(S1, 3.);
        by_traversal("ra::Unique<1>", t, A, B);
        by_traversal2("ra::Unique<1>", t, A, B);
    }
    {
        ra::Unique<real, 2> A(S2, 7.);
        ra::Unique<real, 2> B(S2, 3.);
        by_traversal("ra::Unique<2>", t, A, B);
        by_traversal2("ra::Unique<2>", t, A, B);
    }
    {
        ra::Unique<real, 3> A(S3, 7.);
        ra::Unique<real, 3> B(S3, 3.);
        by_traversal("ra::Unique<3>", t, A, B);
        by_traversal2("ra::Unique<3>", t, A, B);
    }
    cout << "ok\n" << endl;
    return 0;
}
