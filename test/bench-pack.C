
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-pack.C
/// @brief Benchmark pack() operator vs explode/collapse.

#include <iostream>
#include <iomanip>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/large.H"
#include "ra/wrank.H"
#include "ra/operators.H"
#include "ra/io.H"
#include <chrono>

using std::cout; using std::endl; using std::flush;
auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::nanoseconds;
std::string tunit = "ns";
using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(cout);
    cout.precision(4);

    auto bench = [&tr](auto && f, auto A_, char const * tag, int size, int n)
        {
            using A = decltype(A_);
            time_unit dt(0);
            A a({size}, ra::unspecified);
            auto t0 = now();
            for (int i=0; i<n; ++i) {
                f(a, size);
            }
            dt += now()-t0;
            cout << std::setw(10) << std::fixed << (dt.count()/double(n*size)) << " " << tunit << " " << tag << endl;
            tr.quiet().test_eq(ra::pack<complex>(ra::iota<real>(size), size-ra::iota<real>(size)), a);
        };

    auto f_raw = [](auto & a, int size)
        {
            real * p = reinterpret_cast<real *>(a.data());
            for (ra::dim_t i=0; i!=size; ++i, p+=2) {
                p[0] = i;
                p[1] = size-i;
            }
        };
    auto f_reim = [](auto & a, int size)
        {
            real_part(a) = ra::iota(size);
            imag_part(a) = size-ra::iota(size);
        };
    auto f_collapse = [](auto & a, int size)
        {
            auto areim = ra::collapse<real>(a);
            areim(ra::all, 0) = ra::iota(size);
            areim(ra::all, 1) = size-ra::iota(size);
        };
    auto f_pack = [](auto & a, int size)
        {
            a = ra::pack<complex>(ra::iota<real>(size), size-ra::iota<real>(size));
        };
    auto f_xI = [](auto & a, int size)
        {
            a = ra::iota<real>(size) + xI(size-ra::iota<real>(size));
        };

    auto bench_all = [&](auto A_, int size, int n)
        {
            section("size ", size, ", n ", n);

            bench(f_raw, A_, "raw", size, n);
            bench(f_reim, A_, "re/im", size, n);
            bench(f_collapse, A_, "collapse", size, n);
            bench(f_pack, A_, "pack", size, n);
            bench(f_xI, A_, "xI", size, n);
        };

    bench_all(ra::Owned<complex, 1>(), 10, 1000000);
    bench_all(ra::Owned<complex, 1>(), 100, 100000);
    bench_all(ra::Owned<complex, 1>(), 1000, 10000);
    bench_all(ra::Owned<complex, 1>(), 10000, 1000);
    bench_all(ra::Owned<complex, 1>(), 100000, 100);
    bench_all(ra::Owned<complex, 1>(), 1000000, 10);

    return tr.summary();
}
