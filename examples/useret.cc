// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - User defined expression templates

// Daniel Llorens - 2015
// Adapted from blitz++/examples/useret.cpp

// Blitz++'s ETs have a static applicator, so declaring new operations requires
// a new class. Thanks to C++14, declaring new ET operations is trivial in
// ra:: — just declare a function.

#include "ra/ra.hh"
#include <iostream>

using std::cout, std::endl, std::flush;

double myFunction(double x)
{
    return 1.0 / (1 + x);
}

double foobar(double x, double y)
{
    return x*y;
}

template <class A>
auto myFunction_ra(A && a)
{
    return ra::map(myFunction, std::forward<A>(a));
}

template <class A, class B>
auto foobar_ra(A && a, B && b)
{
    return ra::map(foobar, std::forward<A>(a), std::forward<B>(b));
}

// the functions above will work with scalars as well as arrays, but you can overload the name of the scalar function by defining the overload in ra:: like so. See 'New array operations' in the manual.

namespace ra {

template <class A, class B>
inline auto
foobar(A && a, B && b)
{
    return map(::foobar, std::forward<A>(a), std::forward<B>(b));
}

} // namespace ra

int main()
{
    ra::Big<double,2> A({4, 4}, 0.), B({4, 4}, 0.), C({4, 4}, 0.);

    A = { 0,  1,  2,  3,
          4,  5,  6,  7,
          8,  9, 10, 11,
          12, 13, 14, 15 };
    C = 3;

    cout << "A = " << A << endl;
    cout << "C = " << C << endl;

    B = myFunction_ra(A);
    cout << "B = myFunction(A) = " << B << endl;

    B = foobar_ra(A, C);
    cout << "B = foobar(A, C) = " << B << endl;

    B = foobar(A, C);
    cout << "B = foobar(A, C) = " << B << endl;

    B = foobar_ra(A, 1.);
    cout << "B = foobar(A, 1.) = " << B << endl;

    B = foobar_ra(ra::_0, ra::_1);
    cout << "B = foobar(tensor::i, tensor::j) = " << B << endl;
}
