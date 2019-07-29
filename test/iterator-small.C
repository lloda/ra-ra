// -*- mode: c++; coding: utf-8 -*-
/// @file iterator-small.C
/// @brief Higher-rank iterator for SmallArray/SmallView.

// (c) Daniel Llorens - 2014, 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/small.H"
#include "ra/ra.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/mpdebug.H"

using std::cout, std::endl, std::flush;

int main()
{
    TestRecorder tr;
    {
        tr.section("fiddling");
        {
            using A = ra::Small<int, 2, 3>;
            A a = {{1, 2, 3}, {4, 5, 6}};
            cout << a << endl;

            using AI0 = typename A::iterator<0>;
            cout << "AI0 " << std::is_same_v<int, AI0> << endl;
            cout << AI0::rank() << endl;

            using AI1 = typename A::iterator<1>;
            cout << "AI1 " << std::is_same_v<int, AI1> << endl;
            cout << AI1::rank() << endl;

            AI0 bi(a.data());
            cout << bi.c.p << endl;
        }
        tr.section("STL style, should be a pointer for default strides");
        {
            using A = ra::Small<int, 2, 3>;
            A a = {{1, 2, 3}, {4, 5, 6}};
            tr.test(std::is_same_v<int *, decltype(a.begin())>);
            int i = 0;
            for (auto && ai: a) {
                tr.test_eq(i+1, ai);
                ++i;
            }
        }
        tr.section("STL style with non-default strides, which keeps indices (as internal detail)");
        {
            ra::Small<int, 2, 3> a = {{1, 2, 3}, {4, 5, 6}};
            auto b = transpose<1, 0>(a);
            tr.test_eq(ra::start({0, 0}), ra::start(b.begin().i));
            tr.test_eq(ra::start({0, 1}), ra::start((++b.begin()).i));
            tr.test(!std::is_same_v<int *, decltype(b.begin())>);
            int bref[6] = {1, 4, 2, 5, 3, 6};
            int i = 0;
            for (auto && bi: b) {
                tr.test_eq(bref[i], bi);
                ++i;
            }
        }
        tr.section("rank>0");
        {
            auto test_over
                = [&](auto && a)
                  {
                      auto test = [&](std::string ref, auto cr)
                                  {
                                      std::ostringstream o;
                                      for_each([&o](auto && a) { o << a << "|"; }, iter<decltype(cr)::value>(a));
                                      tr.test_eq(ra::scalar(ref), ra::scalar(o.str()));
                                  };
                      test("1|2|3|4|5|6|", mp::int_t<-2>());
                      test("1 2 3|4 5 6|", mp::int_t<-1>());
                      test("1|2|3|4|5|6|", mp::int_t<0>());
                      test("1 2 3|4 5 6|", mp::int_t<1>());
                      test("1 2 3\n4 5 6|", mp::int_t<2>());
                  };
            tr.section("default strides");
            test_over(ra::Small<int, 2, 3> {{1, 2, 3}, {4, 5, 6}});
            tr.section("non-default strides");
            test_over(ra::transpose<1, 0>(ra::Small<int, 3, 2> {{1, 4}, {2, 5}, {3, 6}}));
        }
    }
    return tr.summary();
}
