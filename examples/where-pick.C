// -*- mode: c++; coding: utf-8 -*-
// Select an argument across an array expression.

// Adapted from blitz++/examples/where.cpp
// Daniel Llorens - 2015

#include "ra/ra.H"
#include "ra/test.H"

using std::cout, std::endl, std::flush;

int main()
{
    ra::Big<int, 1> x = ra::iota(7, -3);   // [ -3 -2 -1  0  1  2  3 ]

    // The where(X,Y,Z) function is similar to the X ? Y : Z operator.
    // If X is logical true, then Y is returned; otherwise, Z is
    // returned.

    ra::Big<int, 1> y = where(abs(x) > 2, x+10, x-10);

    // The above statement is transformed into something resembling:
    //
    // for (unsigned i=0; i < 7; ++i)
    //     y[i] = (abs(x[i]) > 2) ? (x[i]+10) : (x[i]-10);
    //

    // The first expression (abs(x) > 2) can involve the usual
    // comparison and logical operators: < > <= >= == != && ||

    cout << x << endl << y << endl;

    // In ra:: we can also pick() among more than two values. You can
    // put anything in the selector expression (the first argument) that will
    // evaluate to an argument index.

    ra::Big<int, 1> z = pick(where(x<0, 0, where(x==0, 1, 2)), x*3, 77, x*2);

    TestRecorder tr(std::cout, TestRecorder::NOISY);
    tr.test_eq(ra::start({7, -12, -11, -10, -9, -8, 13}), y);
    tr.test_eq(ra::start({-9, -6, -3, 77, 2, 4, 6}), z);
    return tr.summary();
}
