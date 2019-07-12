// -*- mode: c++; coding: utf-8 -*-
// @file readme.C
// @brief Examples used in top-level README.md

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Generate README.md and/or these examples.

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"
#include <iostream>

using std::cout; using std::endl;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("dynamic/static shape");
// Dynamic or static array rank. Dynamic or static array shape (all dimensions or none).
    {
        ra::Big<char> A({2, 3}, 'a');     // dynamic rank = 2, dynamic shape = {2, 3}
        ra::Big<char, 2> B({2, 3}, 'b');  // static rank = 2, dynamic shape = {2, 3}
        ra::Small<char, 2, 3> C('c');     // static rank = 2, static shape = {2, 3}
        cout << "A: " << A << "\n\n";
        cout << "B: " << B << "\n\n";
        cout << "C: " << C << "\n\n";
    }
    tr.section("storage");
// Memory-owning types and views. You can make array views over any piece of memory.
    {
        // memory-owning types
        ra::Big<char, 2> A({2, 3}, 'a');     // storage is std::vector inside A
        ra::Unique<char, 2> B({2, 3}, 'b');  // storage is owned by std::unique_ptr inside B
        ra::Small<char, 2, 3> C('c');        // storage is owned by C itself, on the stack

        cout << "A: " << A << "\n\n";
        cout << "B: " << B << "\n\n";
        cout << "C: " << C << "\n\n";

        // view types
        char cs[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
        ra::View<char, 2> D1({2, 3}, cs);            // dynamic sizes and strides, C order
        ra::View<char, 2> D2({{2, 1}, {3, 2}}, cs);  // dynamic sizes and strides, Fortran order.
        ra::SmallView<char, mp::int_list<2, 3>, mp::int_list<3, 1>> D3(cs); // static sizes & strides, C order.
        ra::SmallView<char, mp::int_list<2, 3>, mp::int_list<1, 2>> D4(cs); // static sizes & strides, Fortran order.

        cout << "D1: " << D1 << "\n\n";
        cout << "D2: " << D2 << "\n\n";
        cout << "D3: " << D3 << "\n\n";
        cout << "D4: " << D4 << "\n\n";
    }
    tr.section("shape agreement");
// Shape agreement rules and rank extension (broadcasting) for rank-0 operations of any arity
// and operands of any rank, any of which can a reference (so you can write on them). These
// rules are taken from the array language, J.
// (See examples/agreement.C for more examples.)
    {
        ra::Big<float, 2> A {{1, 2, 3}, {1, 2, 3}};
        ra::Big<float, 1> B {-1, +1};

        ra::Big<float, 2> C({2, 3}, 99.);
        C = A * B;   // C(i, j) = A(i, j) * C(i)
        cout << "C: " << C << "\n\n";

        ra::Big<float, 1> D({2}, 0.);
        D += A * B;  // D(i) += A(i, j) * C(i)
        cout << "D: " << D << "\n\n";
    }
    tr.section("rank iterators");
// Iterators over cells of arbitrary rank.
    {
        ra::TensorIndex<0> i;
        ra::TensorIndex<1> j;
        ra::TensorIndex<2> k;
        ra::Big<float, 3> A({2, 3, 4}, i+j+k);
        ra::Big<float, 2> B({2, 3}, 0);
        cout << "A: " << A << "\n\n";

// store the sum of A(i, j, ...) in B(i, j). All these are equivalent.
        B = 0; B += A;                                                           // default agreement matches prefixes
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A);            // default agreement matches prefixes
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A.iter<1>());  // give cell rank
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A.iter<-2>()); // give frame rank
        cout << "B: " << B << "\n\n";

// store the sum of A(i, ...) in B(i, j). The op is re-executed for each j, so don't do it this way.
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A.iter<2>()); // give cell rank
        cout << "B: " << B << "\n\n";
    }
// A rank conjunction (only for static rank and somewhat fragile).
    tr.section("rank conjuction");
    {
// This is a translation of J: A = (i.3) -"(0 1) i.4, that is: A(i, j) = b(i)-c(j).
        ra::Big<float, 2> A = map(ra::wrank<0, 1>(std::minus<float>()), ra::iota(3), ra::iota(4));
        cout << "A: " << A << "\n\n";
    }
// A proper selection operator with 'beating' of range or scalar subscripts.
// See examples/slicing.C for more examples.
    tr.section("selector");
    {
// TODO do implicit reshape in constructors?? so I can accept any 1-array and not only an initializer_list.
        ra::Big<char, 3> A({2, 2, 2}, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'});
        cout << "A: " << A << "\n\n";

// these are all equivalent to e.g. A(:, 0, :) in Octave.
        cout << "A1: " << A(ra::all, 0) << "\n\n";
        cout << "A2: " << A(ra::all, 0, ra::all) << "\n\n";
        cout << "A3: " << A(ra::all, 0, ra::dots<1>) << "\n\n";

// an inverted range.
        cout << "A4: " << A(ra::iota(2, 1, -1)) << "\n\n";

// indices can be arrays of any rank.
        ra::Big<int, 2> I {{0, 3}, {1, 2}};
        ra::Big<char, 1> B {'a', 'b', 'c', 'd'};
        cout << "B(I): " << B(I) << "\n\n";

// multiple indexing performs an implicit outer product.  this results in a rank
// 4 array X = A(J, 1, J) -> X(i, j, k, l) = A(J(i, j), 1, J(k, l))
        ra::Big<int, 2> J {{1, 0}, {0, 1}};
        cout << "A(J, 1, J): " << A(J, 1, J) << "\n\n";

// explicit indices do not result in a View view (= pointer + strides), but the
// resulting expression can still be written on.
        B(I) = ra::Big<char, 2> {{'x', 'y'}, {'z', 'w'}};
        cout << "B: " << B << endl;
    }
    // A tensor-index object as in Blitz++ (with some differences).
    tr.section("tensorindex");
    {
// as shown above.
    }
    tr.section("STL compat");
    {
        ra::Big<char, 1> A = {'x', 'z', 'y'};
        std::sort(A.begin(), A.end());
        cout << "A: " << A << "\n\n";

        ra::Big<float, 2> B {{1, 2}, {3, 4}};
        B += std::vector<float> {10, 20};
        cout << "B: " << B << "\n\n";
    }
    tr.section("example from the manual [ma100]");
    {
        ra::Small<int, 3> s {2, 1, 0};
        ra::Small<double, 3> z = pick(s, s*s, s+s, sqrt(s));
        cout << "z: " << z << endl;
    }
    tr.section("example from the manual [ma101]");
    {
        ra::Big<char, 2> A({2, 5}, "helloworld");
        std::cout << ra::noshape << format_array(transpose<1, 0>(A), "|") << std::endl;
    }
    {
        ra::Big<char const *, 1> A = {"hello", "array", "world"};
        std::cout << ra::noshape << format_array(A, "|") << std::endl;
    }
    tr.section("example from the manual [ma102]");
    {
        // ra::Big<char const *, 1> A({3}, "hello"); // ERROR b/c of pointer constructor
        ra::Big<char const *, 1> A({3}, ra::scalar("hello"));
        std::cout << ra::noshape << format_array(A, "|") << std::endl;
    }
    tr.section("example from the manual [ma103]");
    {
        ra::Big<int, 2> A {{1, 2}, {3, 4}, {5, 6}};
        ra::Big<int, 2> B {{7, 8, 9}, {10, 11, 12}};
        ra::Big<int, 2> C({3, 3}, 0.);
        for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })), C, A, B);
/* 3 3
   27 30 33
   61 68 75
   95 106 117 */
        cout << C << endl;
    }
    tr.section("example from the manual [ma104] - dynamic size");
    {
        ra::Big<int, 3> c({3, 2, 2}, ra::_0 - ra::_1 - 2*ra::_2);
        cout << "c: " << c << endl;
        cout << "s: " << map([](auto && a) { return sum(diag(a)); }, iter<-1>(c)) << endl;
    }
    tr.section("example from the manual [ma104] - static size");
    {
        ra::Small<int, 3, 2, 2> c = ra::_0 - ra::_1 - 2*ra::_2;
        cout << "c: " << c << endl;
        cout << "s: " << map([](auto && a) { return sum(diag(a)); }, iter<-1>(c)) << endl;
    }
    tr.section("example from the manual [ma105]");
    {
        ra::Big<double, 2> a {{1, 2, 3}, {4, 5, 6}};
        ra::Big<double, 1> b {10, 20, 30};
        ra::Big<double, 2> c({2, 3}, 0);
        iter<1>(c) = iter<1>(a) * iter<1>(b); // multiply each item of a by b
        cout << c << endl;
    }
// example from the manual [ma109]. This is a rare case where I need explicit ply.
    {
        ra::Big<int, 1> o = {};
        ra::Big<int, 1> e = {};
        ra::Big<int, 1> n = {1, 2, 7, 9, 12};
        ply(where(odd(n), map([&o](auto && x) { o.push_back(x); }, n), map([&e](auto && x) { e.push_back(x); }, n)));
        cout << "o: " << ra::noshape << o << ", e: " << ra::noshape << e << endl;
    }
    tr.section("example from manual [ma110]");
    {
        std::cout << exp(ra::Small<double, 3> {4, 5, 6}) << std::endl;
    }
    tr.section("example from manual [ma111]");
    {
        ra::Small<int, 2, 2> a = {{1, 2}, {3, 4}};  // explicit contents
        ra::Small<int, 2, 2> b = {1, 2, 3, 4};  // ravel of content
        cout << "a: " << a << ", b: " << b << endl;
    }
    tr.section("example from manual [ma112]");
    {
        double bx[6] = {1, 2, 3, 4, 5, 6};
        ra::Big<double, 2> b({3, 2}, bx); // {{1, 2}, {3, 4}, {5, 6}}
        cout << "b: " << b << endl;
    }
    tr.section("example from manual [ma114]");
    {
        using sizes = mp::int_list<2, 3>;
        using strides = mp::int_list<1, 2>;
        ra::SmallArray<int, sizes, strides> a {{1, 2, 3}, {4, 5, 6}}; //  stored column-major
        cout << "a: " << a << endl;
        cout << ra::Small<int, 6>(ra::ptr(a.data())) << endl;
    }
    tr.section("example from manual [ma116]");
    {
        ra::Big<int, 2> a({3, 2}, {1, 2, 3, 4, 5, 6});
        ra::Big<int, 1> x = {1, 10};
        cout << (x(ra::all, ra::insert<2>) * a(ra::insert<1>)) << endl;
        cout << (x * a(ra::insert<1>)) << endl; // same thing
    }
    return 0;
}
