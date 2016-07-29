
// Daniel Llorens - 2015
// Adapted from blitz++/examples/indirect.cpp

// ra:: doesn't support ad hoc sparse selectors, so here I show
// alternatives. The whole point of this example in Blitz++ seems to be the
// compact notation, so the alternatives look more verbose, but I don't know if
// it's worthwhile to clog every expr type with additional variants of
// operator() or operator[] (which on C++14 still need to be members). The
// sparse selectors should be defined as standalone functions with separate
// names depending on the kind of indexing they do.

#include "ra/ra-operators.H"
#include "ra/ra-io.H"
#include <vector>
#include <iostream>

using std::cout; using std::endl;

void example1()
{
    // Indirection using a list of coordinates

    ra::Owned<int, 2> A({4, 4}, 0), B({4, 4}, 10*ra::_0 + ra::_1);
    using coord = ra::Small<int, 2>;
    ra::Owned<coord, 1> I = { {1, 1}, {2, 2} };

    // Blitz++ had A[I] = B.
    // In ra::, both sides of = must agree in shape.
    // Also, the selector () is outer product (to index two axes, you need two arguments). The 'coord' selector is at().
    // So this is the most direct translation. Note the -> decltype(auto) to construct a reference expr.
    map([&A](auto && c) -> decltype(auto) { return A.at(c); }, I)
        = map([&B](auto && c) { return B.at(c); }, I);

    // More reasonably
    for_each([&A, &B](auto && c) { A.at(c) = B.at(c); }, I);

    // There is an array op for at(). See also example5 below.
    at(A, I) = at(B, I);

    cout << "B = " << B << endl << "A = " << A << endl;

    // B = 4 x 4
    //     0         1         2         3
    //    10        11        12        13
    //    20        21        22        23
    //    30        31        32        33

    // A = 4 x 4
    //     0         0         0         0
    //     0        11         0         0
    //     0         0        22         0
    //     0         0         0         0
}

void example2()
{
    // Cartesian-product indirection

    ra::Owned<int, 2> A({6, 6}, 0), B({6, 6}, 10*ra::_0 + ra::_1);
    ra::Owned<int, 1> I { 1, 2, 4 }, J { 2, 0, 5 };

    // The normal selector () already has Cartesian-product behavior. As before, both sides must agree in shape.
    A(I, J) = B(I, J);

    cout << "B = " << B << endl << "A = " << A << endl;

    // B = 6 x 6
    //     0         1         2         3         4         5
    //    10        11        12        13        14        15
    //    20        21        22        23        24        25
    //    30        31        32        33        34        35
    //    40        41        42        43        44        45
    //    50        51        52        53        54        55

    // A = 6 x 6
    //     0         0         0         0         0         0
    //    10         0        12         0         0        15
    //    20         0        22         0         0        25
    //     0         0         0         0         0         0
    //    40         0        42         0         0        45
    //     0         0         0         0         0         0

}

void example3()
{
    // Simple 1-D indirection, using a STL container of int

    ra::Owned<int, 1> A({5}, 0), B({ 1, 2, 3, 4, 5 });
    ra::Owned<int, 1> I {2, 4, 1};

    // As before, both sides must agree in shape.
    A(I) = B(I);

    cout << "B = " << B << endl << "A = " << A << endl;

    // B = [          1         2         3         4         5 ]
    // A = [          0         2         3         0         5 ]
}

void example4()
{
    // Indirection using a list of rect domains (RectDomain<N> objects in Blitz++).
    // ra:: doesn't have those, so we fake it.

    const int N = 7;
    ra::Owned<int, 2> A({N, N}, 0.), B({N, N}, 1.);

    double centre_i = (N-1)/2.0;
    double centre_j = (N-1)/2.0;
    double radius = 0.8 * N/2.0;

    // circle will contain a list of strips which represent a circular subdomain.
    ra::Owned<std::tuple<int, ra::Iota<int> >, 1> circle; // [y x0 x1; ...]
    for (int i=0; i < N; ++i)
    {
        double jdist2 = sqr(radius) - sqr(i-centre_i);
        if (jdist2 < 0.0)
            continue;

        int jdist = int(sqrt(jdist2));
        int begin = int(centre_j - jdist);
        int end = int(centre_j + jdist);
        circle.push_back(std::make_tuple(i, ra::Iota<int>(end-begin+1, begin)));
    }

    // Set only those points in the circle subdomain to 1
    map([&A](auto && c) -> decltype(auto) { return A(std::get<0>(c), std::get<1>(c)); }, circle)
        = map([&B](auto && c) { return B(std::get<0>(c), std::get<1>(c)); }, circle);

    // or more reasonably
    for_each([&A, &B](auto && c) { A(std::get<0>(c), std::get<1>(c)) = B(std::get<0>(c), std::get<1>(c)); }, circle);

    // but it would be easier to just do
    A  = 0.;
    B  = 1.;
    A = where(sqr(ra::_0-centre_i)+sqr(ra::_1-centre_j)<sqr(radius), B, A);

    cout << "A = " << A << endl;

    // A = 7 x 7
    //  0  0  0  0  0  0  0
    //  0  0  1  1  1  0  0
    //  0  1  1  1  1  1  0
    //  0  1  1  1  1  1  0
    //  0  1  1  1  1  1  0
    //  0  0  1  1  1  0  0
    //  0  0  0  0  0  0  0
}

void example5()
{
    // suppose you have the x coordinates in one array and the y coordinates in another array.
    ra::Owned<int, 2> x({4, 4}, {0, 1, 2, 0, /* */ 0, 1, 2, 0, /*  */ 0, 1, 2, 0, /* */ 0, 1, 2, 0});
    ra::Owned<int, 2> y({4, 4}, {1, 2, 0, 1, /* */ 1, 2, 0, 1, /*  */ 1, 2, 0, 1, /* */ 1, 2, 0, 1});
    cout << "coordinates: " << format_array(ra::pack<ra::Small<int, 2> >(x, y), "|") << endl;

    // you can use these for indirect access without creating temporaries.
    ra::Owned<int, 2> a({3, 3}, {0, 1, 2, 3, 4, 5, 6, 7, 8});
    ra::Owned<int, 2> b = at(a, ra::pack<ra::Small<int, 2> >(x, y));
    cout << "sampling of a using coordinates: " << b << endl;

    // cf the default selection operator, which creates an outer product a(x(i, j), y(k, l)) (a 4x4x4x4 array).
    cout << "outer product selection: " << a(x, y) << endl;
}

int main()
{
    example1();
    example2();
    example3();
    example4();
    example5();
}
