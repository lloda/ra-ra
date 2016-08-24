
// Daniel Llorens - 2015
// Adapted from blitz++/examples/tiny.cpp

// The point of this example is to show the assembly; I will when it's not awful :-/
// $CXX -o small.s -S -O3 -DRA_CHECK_BOUNDS=0 -std=c++14 -Wall -Werror -Wno-unknown-pragmas -Wno-parentheses -Wno-error=strict-overflow -march=native -I.. small.C -funroll-loops -ffast-math -fno-exceptions

#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

using std::cout; using std::endl; using std::flush;

void
add_vectors(ra::Small<double, 4> & c, ra::Small<double, 4> const & a, ra::Small<double, 4> const & b)
{
    c = a+b;
}

void
reflect(ra::Small<double, 3> & reflection, ra::Small<double, 3> const & ray, ra::Small<double, 3> const & surfaceNormal)
{
    // The surface normal must be unit length to use this equation.
    reflection = ray - 2.7 * dot(ray, surfaceNormal) * surfaceNormal;
}

int main()
{
    {
        ra::Small<double, 3> x, y, z;

        // y will be the incident ray
        y = { 1, 0, -1 };

        // z is the surface normal
        z = { 0, 0, 1 };

        reflect(x, y, z);
        cout << "Reflected ray is: [ " << x[0] << " " << x[1] << " " << x[2]
             << " ]" << endl;
    }
    {
        ra::Small<double, 4> x, y { 1, 2, 3, 4 }, z { 10, 11, 12, 13 };

        add_vectors(x, y, z);
        cout << "x(" << x << ") = y (" << y << ") + z (" << z << ")" << endl;
    }
    return 0;
}
