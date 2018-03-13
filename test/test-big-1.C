
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-big-1.C
/// @brief Tests specific to WithStorage.

#include <iostream>
#include <iterator>
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/view-ops.H"
#include "ra/format.H"
#include "ra/test.H"

using std::cout; using std::endl; using std::flush;
template <class T, ra::rank_t RANK=ra::RANK_ANY> using BigValueInit = ra::WithStorage<std::vector<T>, RANK>;

int main()
{
    TestRecorder tr;
    tr.section("push_back");
    {
        using int2 = ra::Small<int, 2>;
        std::vector<int2> a;
        a.push_back({1, 2});
        ra::Big<int2, 1> b;
        b.push_back({1, 2});

        int2 check[1] = {{1, 2}};
        tr.test_eq(check, ra::start(a));
        tr.test_eq(check, b);
    }
    tr.section("behavior of resize with default WithStorage");
    {
        {
            ra::Big<int, 1> a = {1, 2, 3, 4, 5, 6};
            a.resize(3);
            a.resize(6);
            tr.test_eq(ra::iota(6, 1), a);
        }
        {
            BigValueInit<int, 1> a = {1, 2, 3, 4, 5, 6};
            a.resize(3);
            a.resize(6);
            tr.test_eq(ra::start({1, 2, 3, 0, 0, 0}), a);
        }
    }
    tr.section("operator=");
    {
        ra::Big<int, 2> a = {{0, 1, 2}, {3, 4, 5}};
        cout << "A0" << endl;
        a = {{4, 5, 6}, {7, 8, 9}};
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        cout << "A1" << endl;
        a = {{{4, 5, 6}, {7, 8, 9}}}; // would work if supported
        cout << "A2" << endl;
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        cout << "A3" << endl;
    }
    tr.section("behavior of forced Fortran array");
    {
        ra::Big<int, 2> a ({2, 3}, {0, 1, 2, 3, 4, 5});
        ra::Big<int, 2> b ({2, 3}, {0, 1, 2, 3, 4, 5});
        auto c = transpose({1, 0}, ra::View<int, 2>({3, 2}, a.data()));
        a.dim = c.dim;
        for (int k=0; k!=c.rank(); ++k) {
            std::cout << "CSTRIDE " << k << " " << c.stride(k) << std::endl;
            std::cout << "CSIZE " << k << " " << c.size(k) << std::endl;
        }
        cout << endl;
        for (int k=0; k!=a.rank(); ++k) {
            std::cout << "ASTRIDE " << k << " " << a.stride(k) << std::endl;
            std::cout << "ASIZE " << k << " " << a.size(k) << std::endl;
        }
        cout << endl;
        c = b;
// FIXME this clobbers the strides of a, which is surprising -> WithStorage should behave as View. Or, what happens to a shouldn't depend on the container vs view-ness of b.
        a = b;
        for (int k=0; k!=c.rank(); ++k) {
            std::cout << "CSTRIDE " << k << " " << c.stride(k) << std::endl;
            std::cout << "CSIZE " << k << " " << c.size(k) << std::endl;
        }
        cout << endl;
        for (int k=0; k!=a.rank(); ++k) {
            std::cout << "ASTRIDE " << k << " " << a.stride(k) << std::endl;
            std::cout << "ASIZE " << k << " " << a.size(k) << std::endl;
        }
        cout << endl;
        std::cout << "a: " << a << std::endl;
        std::cout << "b: " << b << std::endl;
        std::cout << "c: " << c << std::endl;
    }
    return tr.summary();
}
