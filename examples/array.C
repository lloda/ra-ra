
// (c) Daniel Llorens - 2015
// Converted from blitz++/examples/array.cpp

// @TODO For this example I need linear expressions of ra::Iota to
// yield ra::Iota so that selectors can beat on them. Right now,
// ra::Iota + 1 is ra::Expr that the selector doesn't know how to beat
// on. It will still work through the regular selector-of-expr
// mechanism, but will be slower.
// @TODO Better traversal...

#include "ra/ra-operators.H"
#include "ra/format.H"
#include <iomanip>

using std::cout; using std::endl;

int main()
{
    int N = 64;
    ra::Owned<float, 3> A({N, N, N}, ra::default_init);
    ra::Owned<float, 3> B({N, N, N}, ra::default_init);

// Set up initial conditions: +30 C over an interior block, and +22 C elsewhere
    A = 22.;

    ra::Iota<int> interior(N/2, N/4);
    A(interior, interior, interior) = 30.;

    int numIters = 301;

    ra::Iota<int> I(N-2, 1), J(N-2, 1), K(N-2, 1);

// The views A(...) can be precomputed, but that's only useful if the subscripts are beatable.
    for (int i=0; i<numIters; ++i) {
        double c = 1/6.5;

        B(I, J, K) = c * (.5 * A(I, J, K) + A(I+1, J, K) + A(I-1, J, K)
                          + A(I, J+1, K) + A(I, J-1, K) + A(I, J, K+1) + A(I, J, K-1));
        A(I, J, K) = c * (.5 * B(I, J, K) + B(I+1, J, K) + B(I-1, J, K)
                          + B(I, J+1, K) + B(I, J-1, K) + B(I, J, K+1) + B(I, J, K-1));

// Output the result along a line through the centre
        cout << std::setprecision(4) << rawp(A(N/2, N/2, ra::jvec(8, 0, N/8))) << endl;
        cout.flush();
    }
}
