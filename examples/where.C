
// Daniel Llorens - 2015
// Adapted from blitz++/examples/where.cpp

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"

using std::cout; using std::endl; using std::flush;

int main()
{
    ra::Owned<int, 1> x = ra::iota(7, -3);   // [ -3 -2 -1  0  1  2  3 ]

    // The where(X,Y,Z) function is similar to the X ? Y : Z operator.
    // If X is logical true, then Y is returned; otherwise, Z is
    // returned.

    ra::Owned<int, 1> y = where(abs(x) > 2, x+10, x-10);

    // The above statement is transformed into something resembling:
    //
    // for (unsigned i=0; i < 7; ++i)
    //     y[i] = (abs(x[i]) > 2) ? (x[i]+10) : (x[i]-10);
    //



    // The first expression (abs(x) > 2) can involve the usual
    // comparison and logical operators: < > <= >= == != && ||

    cout << x << endl
         << y << endl;

    TestRecorder tr;
    tr.test_eq(ra::vector({7, -12, -11, -10, -9, -8, 13}), y);
    return tr.summary();
}
