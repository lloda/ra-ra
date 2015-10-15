
// Daniel Llorens - 2015
// Adapted from blitz++/examples/useret.cpp

// This is cheating since Blitz++'s ETs have a static applicator vs ra::Expr
// where it's kept as a member. The other difference is the naming
// convenience. There's a method for naming array ops in ra-operators.H
// (DEFINE_BINARY_OP etc, [ref:examples/useret.C:0]) but it's undeveloped.

#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;

double myFunction(double x)
{ return 1.0 / (1 + x); }

double foobar(double x, double y)
{
    return x*y;
}

int main()
{
    ra::Owned<double,2> A({4, 4}, 0.), B({4, 4}, 0.), C({4, 4}, 0.);

    A = { 0,  1,  2,  3,
          4,  5,  6,  7,
          8,  9, 10, 11,
          12, 13, 14, 15 };
    C = 3;

    cout << "A = " << A << endl
         << "C = " << C << endl;

// This is not exactly the same

    B = map(myFunction, A);
    cout << "B = myFunction(A) = " << B << endl;

    B = map(foobar, A, C);
    cout << "B = foobar(A,C) = " << B << endl;

    B = map(foobar, ra::_0, ra::_1);
    cout << "B = foobar(tensor::i, tensor::j) = " << B << endl;
}
