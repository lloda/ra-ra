
// (c) Daniel Llorens - 2016

/// @file explode-collapse.C
/// @brief Demo collapse and explode operations.

#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;
template <class T, int rank> using Array = ra::Owned<T, rank>;
template <class T, int ... sizes> using Small = ra::Small<T, sizes ...>;

int main()
{
// These operations let you view an array of rank n in an array of rank (n-k) of
// subarrays of rank k (explode) or the opposite (collapse). However, there is a
// caveat: the subarrays must be compact (stored contiguously) and sizes of the
// subarrays must be known statically. In effect, the subarray must be of type
// ra::Small. For example:
    cout << "\nexplode\n" << endl;
    {
        Array<int, 2> A({2, 3}, ra::_0 - ra::_1);
        auto B = ra::explode<Small<int, 3> >(A);

        cout << "B(0): " << B(0) << endl; // [0 -1 -2], note the static size
        cout << "B(1): " << B(1) << endl; // [1, 0, -1], note the static size
        B(1) = 9;
        cout << "A after: " << A << endl;
    }
// The collapsed/exploded arrays are views into the source. This is similar to a
// Blitz++ feature called 'multicomponents'. For instance, suppose we have an
// array of (static-sized) [x, y, z] vectors and wish to initialize the x, y, z
// components separately:
    cout << "\ncollapse\n" << endl;
    {
        Array<Small<int, 3>, 1> A = {{0, -1, -2}, {1, 0, -1}, {2, 1, -1}};
        auto B = ra::collapse<int>(A);

        cout << "B before: " << B << endl;
        B(ra::all, 0) = 9; // initialize all x components
        B(ra::all, 1) = 7; // all y
        B(ra::all, 2) = 3; // all z

        cout << "B after: " << B << endl;
        cout << "A after: " << A << endl;
    }
// collapse/explode can also be used to make views into the real and imag parts
// of a complex array. To use explode in this way, the last axis must be
// contiguous and of size 2, just as when using Small<>.
    cout << "\nreal/imag views\n" << endl;
    {
        using complex = std::complex<double>;
        Array<complex, 2> A({2, 2}, 0.);
        auto B = ra::collapse<double>(A);

        B(ra::all, ra::all, 0) = ra::_0-ra::_1; // set real part of A
        B(ra::all, ra::all, 1) = -3; // set imaginary part of A
        cout << "A after: " << A << endl;
// of course, you could also do
        imag_part(A) = 7.;
        cout << "A after: " << A << endl;
// which doesn't construct an array view as collapse() does, but relies on the
// expression template mechanism instead.
    }
    return 0;
}
