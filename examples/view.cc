// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - Operations with views, slicing

// Daniel Llorens - 2015
// Adapted from blitz++/examples/array.cpp
// TODO Better traversal...

#include "ra/ra.hh"
#include "ra/test.hh"
#include "ra/format.hh"
#include <iomanip>
#include <chrono>

using std::cout, std::endl, std::flush, ra::TestRecorder;
auto now() { return std::chrono::high_resolution_clock::now(); }
template <class DT> auto ms(DT && dt) { return std::chrono::duration_cast<std::chrono::milliseconds>(dt).count(); }

int main()
{
    int numIters = 301;

    int N = 64;
    ra::Big<float, 3> A({N, N, N}, ra::none);
    ra::Big<float, 3> B({N, N, N}, ra::none);

    auto interior = ra::iota(N/2, N/4);
// Set up initial conditions: +30 C over an interior block, and +22 C elsewhere
    A = 22.;
    A(interior, interior, interior) = 30.;

// Note that you don't really need separate I, J, K. You could just use I for every subscript.
    auto I = ra::iota(N-2, 1);
    auto J = ra::iota(N-2, 1);
    auto K = ra::iota(N-2, 1);
// The views A(...) can be precomputed, but that's only useful if the subscripts are beatable.
    {
        std::chrono::duration<float> dt(0);
        double c = 1/6.5;
        for (int i=0; i<numIters; ++i) {

            auto t0 = now();
            B(I, J, K) = c * (.5 * A(I, J, K) + A(I+1, J, K) + A(I-1, J, K)
                              + A(I, J+1, K) + A(I, J-1, K) + A(I, J, K+1) + A(I, J, K-1));
            A(I, J, K) = c * (.5 * B(I, J, K) + B(I+1, J, K) + B(I-1, J, K)
                              + B(I, J+1, K) + B(I, J-1, K) + B(I, J, K+1) + B(I, J, K-1));
            dt += (now()-t0);
// Output the result along a line through the centre
            cout << std::setprecision(4) << std::fixed << ra::noshape << A(N/2, N/2, ra::iota(8, 0, N/8)) << endl;
        }
        cout << std::setw(10) << std::fixed << (ms(dt)/double(numIters)) << " ms / iter " << endl;
    }
    ra::Big<float, 3> first_A = A;

    A = 22.;
    A(interior, interior, interior) = 30.;
// These are always beatable. I+1 and I-1 are also beatable if RA_DO_OPT is #defined to 1, which is the default.
    auto Im = ra::iota(N-2, 0);
    auto Ip = ra::iota(N-2, 2);
    {
        std::chrono::duration<float> dt(0);
        double c = 1/6.5;
        for (int i=0; i<numIters; ++i) {

            auto t0 = now();
            B(I, I, I) = c * (.5 * A(I, I, I) + A(Ip, I, I) + A(Im, I, I)
                              + A(I, Ip, I) + A(I, Im, I) + A(I, I, Ip) + A(I, I, Im));
            A(I, I, I) = c * (.5 * B(I, I, I) + B(Ip, I, I) + B(Im, I, I)
                              + B(I, Ip, I) + B(I, Im, I) + B(I, I, Ip) + B(I, I, Im));
            dt += (now()-t0);
// Output the result along a line through the centre
            cout << std::setprecision(4) << std::fixed << ra::noshape << A(N/2, N/2, ra::iota(8, 0, N/8)) << endl;
        }
        cout << std::setw(10) << std::fixed << (ms(dt)/double(numIters)) << " ms / iter " << endl;
    }

    TestRecorder tr(std::cout);
    tr.quiet().test_rel_error(first_A, A, 0.);
    return tr.summary();
}
