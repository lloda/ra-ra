// -*- mode: c++; coding: utf-8 -*-
// Adapted from blitz++/examples/deriv.cpp
// Daniel Llorens - 2015

#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

using std::cout; using std::endl; using std::flush;

using Array1D = ra::Big<double, 1>;

// "index placeholder" which represents the array index for the first axis in a multidimensional expression.
ra::TensorIndex<0> i;

int main()
{
    // In this example, the function cos(x)^2 and its second derivative
    // 2 (sin(x)^2 - cos(x)^2) are sampled over the range [0,1).
    // The second derivative is approximated numerically using a
    // [ 1 -2  1 ] mask, and the approximation error is computed.

    const int numSamples = 100;              // Number of samples
    double delta = 1. / numSamples;          // Spacing of samples
    ra::Iota<int> R(numSamples);             // Index set 0 .. (numSamples-1)
    cout << "R... " << R << endl;

    // Sample the function y = cos(x)^2 over [0,1)
    //
    // The initialization for y (below) will be translated via expression
    // templates into something of the flavour
    //
    // for (unsigned i=0; i < 99; ++i)
    // {
    //     double _t1 = cos(i * delta);
    //     y[i] = _t1 * _t1;
    // }

    // [ra] You need to give a size at construction because 'i' doesn't provide one; you could do instead
    // Array1D y = sqr(cos(R * delta));
    // since R does have a defined size.

    Array1D y({numSamples}, sqr(cos(i * delta)));

    // Sample the exact second derivative
    Array1D y2exact({numSamples}, 2.0 * (sqr(sin(i * delta)) - sqr(cos(i * delta))));

    // Approximate the 2nd derivative using a [ 1 -2  1 ] mask
    // We can only apply this mask to the elements 1 .. 98, since
    // we need one element on either side to apply the mask.
    // I-1 etc. are beatable if RA_OPTIMIZE is true.
    ra::Iota<int> I(numSamples-2, 1);
    Array1D y2({numSamples}, ra::none);
    y2(I) = (y(I-1) - 2 * y(I) + y(I+1)) / (delta*delta);

    // The above difference equation will be transformed into
    // something along the lines of
    //
    // double _t2 = delta*delta;
    // for (int i=1; i < 99; ++i)
    //     y2[i] = (y[i-1] - 2 * y[i] + y[i+1]) / _t2;

    // Now calculate the root mean square approximation error:
    // [ra] TODO don't have mean() yet
    double error = sqrt(sum(sqr(y2(I) - y2exact(I)))/I.size());

    // Display a few elements from the vectors.
    // This range constructor means elements 1 to 91 in increments
    // of 15.
    ra::Iota<int> displayRange(7, 1, 15);

    cout << "Exact derivative:" << y2exact(displayRange) << endl
         << "Approximation:   " << y2(displayRange) << endl
         << "RMS Error:       " << error << endl;

    return 0;
}
