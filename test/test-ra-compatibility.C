
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-compatibility.C
/// @brief Using std:: and ra:: together.

#include <iostream>
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;

int main()
{
    TestRecorder tr;
    section("Tests for std:: types");
    {
        section("plain ra::vector()");
        {
            ply_ravel(expr([](int i) { std::cout << "Ai: " << i << std::endl; return i; },
                           ra::vector(std::array<int, 3> {{1, 2, 0}})));
// [1] @BUG Only on jastpc19 with gcc-5.2, haven't figured it out
            std::cout << expr([](int i) { std::cout << "Bi: " << i << std::endl; return i; },
                              ra::vector(std::vector<int> {1, 2, 0})).at(ra::Small<int, 1>{0}) << std::endl;

            ply_ravel(expr([](int i) { std::cout << "Ci: " << i << std::endl; return i; },
                           ra::vector(std::vector<int> {1, 2, 0})));
        }
        section("frame match ra::vector on 1st axis");
        {
            std::vector<int> const a = { 1, 2, 3 };
            ra::Owned<int, 2> b ({3, 2}, ra::vector(a));
            tr.test_equal(a[0], b(0));
            tr.test_equal(a[1], b(1));
            tr.test_equal(a[2], b(2));
         }
// @TODO actually whether unroll is avoided depends on ply_either, have a way to require it.
// Cf [tr0-01] in test-ra-0.C.
        section("[trc01] frame match ra::vector on 1st axis, forbid unroll");
        {
            std::vector<int> const a = { 1, 2, 3 };
            ra::Owned<int, 3> b ({3, 4, 2}, ra::default_init);
            transpose(b, {0, 2, 1}) = ra::vector(a);
            tr.test_equal(a[0], b(0));
            tr.test_equal(a[1], b(1));
            tr.test_equal(a[2], b(2));
        }
        section("frame match ra::vector on some axis other than 1st");
        {
            {
                ra::Owned<int, 1> const a = { 10, 20, 30 };
                ra::Owned<int, 1> const b = { 1, 2 };
                ra::Owned<int, 2> c = ra::ryn(ra::verb<0, 1>::make([](int a, int b) { return a + b; }),
                                              a.iter(), b.iter());
                tr.test_equal(ra::Owned<int, 2>({3, 2}, {11, 12, 21, 22, 31, 32}), c);
            }
            {
                std::vector<int> const a = { 10, 20, 30 };
                std::vector<int> const b = { 1, 2 };
                ra::Owned<int, 2> c = ra::ryn(ra::verb<0, 1>::make([](int a, int b) { return a + b; }),
                                              ra::vector(a), ra::vector(b));
                tr.test_equal(ra::Owned<int, 2>({3, 2}, {11, 12, 21, 22, 31, 32}), c);
            }
        }
        section("= operators on ra::vector");
        {
            std::vector<int> a { 1, 2, 3 };
            ra::vector(a) *= 3;
            tr.test_equal(ra::vector(std::vector<int> { 3, 6, 9 }), ra::vector(a));
        }
    }
    return tr.summary();
}
