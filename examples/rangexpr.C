
// Daniel Llorens - 2015
// Adapted from blitz++/examples/rangexpr.cpp

#include "ra/operators.H"
#include "ra/io.H"

using std::cout, std::endl, ra::PI;

int main()
{
    ra::Big<float, 1> x = cos(ra::iota(8) * (2.0 * PI / 8));
    cout << x << endl;

    return 0;
}
