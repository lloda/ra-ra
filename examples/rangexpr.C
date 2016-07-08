
// Daniel Llorens - 2015
// Adapted from blitz++/examples/rangexpr.cpp

#include "ra/ra-operators.H"
#include "ra/ra-io.H"

using std::cout; using std::endl;

int main()
{
    ra::Owned<float, 1> x = cos(ra::jvec(8) * (2.0 * PI / 8));
    cout << x << endl;

    return 0;
}
