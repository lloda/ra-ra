// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - IO, formatting.

// (c) Daniel Llorens - 2013-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

using int3 = ra::Small<int, 3>;
using int2 = ra::Small<int, 2>;

template <class AA, class CC>
void
iocheck(TestRecorder & tr, AA && a, CC && check)
{
    std::ostringstream o;
    o << a;
    cout << "\nwritten: " << o.str() << endl;
    std::istringstream i(o.str());
    std::decay_t<CC> c;
    i >> c;
    cout << "\nread: " << c << endl;
    tr.test_eq(shape(check), shape(c));
    tr.test_eq(check, c);
}

struct Q
{
    int x = 1;
};

std::ostream & operator<<(std::ostream & o, Q && q)
{
    return (o << q.x);
}

int main()
{
    TestRecorder tr;
    tr.section("common arrays or slices");
    {
        ra::Unique<int, 2> a({5, 3}, ra::_0 - ra::_1);
        ra::Unique<int, 2> ref({5, 3}, a); // TODO how about an explicit copy() function?
        iocheck(tr.info("output of Unique (1)"), a, ref);
        iocheck(tr.info("output of Unique (1)"), a, ref);
    }
    {
        ra::Big<int, 1> a = {1};
        ra::Big<int, 1> ref = {1};
        iocheck(tr.info("Big of rank 1"), a, ref);
    }
    tr.section("IO format parameters against default (I)");
    {
        ra::Small<int, 2, 2> A {1, 2, 3, 4};
        std::ostringstream o;
        o << format_array(A, { .shape=ra::withshape, .sep0="|", .sepn="-" } );
        tr.test_eq(std::string("2 2\n1|2-3|4"), o.str());
    }
    tr.section("IO format parameters against default (II)");
    {
        ra::Big<int, 2> A({2, 2}, {1, 2, 3, 4});
        std::ostringstream o;
        o << format_array(A, { .shape=ra::noshape, .sep0="|", .sepn="-" });
        tr.test_eq(std::string("1|2-3|4"), o.str());
    }
    tr.section("IO format parameters against default (IIb)");
    {
        ra::Big<int, 2> A({2, 2}, {1, 2, 3, 4});
        std::ostringstream o;
        o << ra::format_t { .shape=ra::noshape, .sep0="|", .sepn="-" } << A;
        tr.test_eq(std::string("1|2-3|4"), o.str());
    }
    tr.section("IO format parameters against default (IIc)");
    {
        ra::Big<int, 2> A({2, 2}, {1, 2, 3, 4});
        std::ostringstream o;
        o << ra::format_t { .shape=ra::noshape, .sep0="|", .sepn="-" } << A;
        tr.test_eq(std::string("1|2-3|4"), o.str());
    }
    tr.section("IO format parameters against default (III)");
    {
        ra::Big<int, 4> A({2, 2, 2, 2}, ra::_0 + ra::_1 + ra::_2 + ra::_3);
        {
            std::ostringstream o;
            o << "\n" << format_array(A, ra::cstyle) << endl;
            tr.test_seq(
                R"---(
{{{{0, 1},
   {1, 2}},
  {{1, 2},
   {2, 3}}},
 {{{1, 2},
   {2, 3}},
  {{2, 3},
   {3, 4}}}}
)---"
                , o.str());
        }
        {
            std::ostringstream o;
            auto style = ra::cstyle;
            style.shape = ra::withshape;
            o << "\n" << style << A << endl;
            tr.test_seq(
                R"---(
2 2 2 2
{{{{0, 1},
   {1, 2}},
  {{1, 2},
   {2, 3}}},
 {{{1, 2},
   {2, 3}},
  {{2, 3},
   {3, 4}}}}
)---"
                , o.str());
        }
        {
            std::ostringstream o;
            o << "\n" << format_array(A, ra::jstyle) << endl;
            tr.test_seq(
                R"---(
2 2 2 2
0 1
1 2

1 2
2 3


1 2
2 3

2 3
3 4
)---"
                , o.str());
        }
        {
            std::ostringstream o;
            o << "\n" << format_array(A, ra::lstyle) << endl;
            tr.test_seq(
                R"---(
((((0 1)
   (1 2))
  ((1 2)
   (2 3)))
 (((1 2)
   (2 3))
  ((2 3)
   (3 4))))
)---"
                , o.str());
        }
        {
            std::ostringstream o;
            auto style = ra::lstyle;
            style.align = false;
            o << "\n" << format_array(A, style) << endl;
            tr.test_seq(
                R"---(
((((0 1)
(1 2))
((1 2)
(2 3)))
(((1 2)
(2 3))
((2 3)
(3 4))))
)---"
                , o.str());
        }
        {
            std::ostringstream o;
            o << "\n" << format_array(A, ra::pstyle) << endl;
            tr.test_seq(
                R"---(
[[[[0, 1],
   [1, 2]],

  [[1, 2],
   [2, 3]]],


 [[[1, 2],
   [2, 3]],

  [[2, 3],
   [3, 4]]]]
)---"
                , o.str());
        }
    }
    tr.section("IO manip without FormatArray");
    {
        ra::Small<int, 2, 2> A {1, 2, 3, 4};
        std::ostringstream o;
        auto style = ra::jstyle;
        style.shape = ra::withshape;
        o << style << A;
        tr.test_eq(std::string("2 2\n1 2\n3 4"), o.str());
    }
    tr.section("IO manip without FormatArray");
    {
        ra::Big<int, 2> A({2, 2}, {1, 2, 3, 4});
        std::ostringstream o;
        o << ra::nstyle << A;
        tr.test_eq(std::string("1 2\n3 4"), o.str());
    }
    tr.section("[ra02a] printing Expr");
    {
        iocheck(tr.info("output of expr (1)"),
                ra::expr([](double i) { return -i; }, start(ra::Small<double, 3>{0, 1, 2})),
                ra::Small<double, 3>{0, -1, -2});
        iocheck(tr.info("output of expr (2)"),
                ra::expr([](double i) { return -i; }, start(ra::Small<double, 3, 2, 3> (ra::_0 - ra::_1 + ra::_2))),
                (ra::Small<double, 3, 2, 3> (-(ra::_0 - ra::_1 + ra::_2))));
    }
    {
        ra::Unique<int, 2> a({2, 3}, { 1, 2, 3, 4, 5, 6 });
        iocheck(tr.info("output of expr (3)"),
                ra::expr([](int i) { return -i; }, a.iter()),
                ra::Unique<int, 2>({2, 3}, { -1, -2, -3, -4, -5, -6 }));
    }
    tr.section("[ra02b] printing array iterators");
    {
        ra::Unique<int, 2> a({3, 2}, { 1, 2, 3, 4, 5, 6 });
        iocheck(tr.info("output of array through its iterator"), a.iter(), a);
// note that transpose({1, 0}, ...) will have dynamic rank, so the type expected from read must also.
        iocheck(tr.info("output of transposed array through its iterator"),
                transpose({1, 0}, a).iter(),
                ra::Unique<int>({2, 3}, { 1, 3, 5, 2, 4, 6 }));
    }
    tr.section("[ra02c] printing array iterators");
    {
        ra::Small<int, 3, 2> a { 1, 2, 3, 4, 5, 6 };
        iocheck(tr.info("output of array through its iterator"), a.iter(), a);
        iocheck(tr.info("output of transposed array through its iterator"),
                transpose<1, 0>(a).iter(),
                ra::Small<int, 2, 3> { 1, 3, 5, 2, 4, 6 });
    }
    tr.section("IO can handle undef len iota, too");
    {
        iocheck(tr.info("output of expr (1)"),
                ra::expr([](double i, auto j) { return -i*double(j); }, ra::Small<double, 3>{0, 1, 2}.iter(), ra::iota<0>()),
                ra::Small<double, 3>{0, -1, -4});
    }
    tr.section("IO of var rank expression");
    {
        ra::Small<int, 2, 2> A {1, 2, 3, 4};
        ra::Unique<int> B({2, 2}, {1, 2, 3, 4});
        iocheck(tr.info("var rank expr"), A+B, ra::Unique<int>({2, 2}, { 2, 4, 6, 8 }));
    }
    tr.section("IO of nested type");
    {
        ra::Small<ra::Big<double, 1>, 3> g = { { 1 }, { 1, 2 }, { 1, 2, 3 } };
        iocheck(tr.info("nested type"), g, g);
    }
// this behavior depends on [ra13] (std::string is scalar) and the specializations of ostream<< in ply.hh.
    tr.section("Non-ra types in format()");
    {
        std::ostringstream o;
        o << ra::format(std::string("once"), " ", std::array {1, 2, 3});
        tr.info(o.str()).test_eq(std::string("once 1 2 3"), o.str());
    }
    tr.section("regression against lack of forwarding in ra::format(...)");
    {
        std::ostringstream o;
        o << ra::format(Q { 33 });
        tr.test_eq(std::string("33"), o.str());
    }
    tr.section("empty array");
    {
        ra::Big<int, 1> pick;
        std::ostringstream o;
        o << format_array(pick, ra::cstyle);
        tr.test_eq(std::string("{}"), o.str());
    }
    tr.section("rank 0");
    {
        ra::Big<int, 0> pick = 7;
        std::ostringstream o;
        o << format_array(pick);
        tr.test_eq(std::string("7"), o.str());
    }
    return tr.summary();
}
