// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Higher-rank iterator for SmallArray/ViewSmall.

// (c) Daniel Llorens - 2014, 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::int_list;

int main()
{
    TestRecorder tr;
    {
        tr.section("fiddling");
        {
            using A = ra::Small<int, 2, 3>;
            A a = {{1, 2, 3}, {4, 5, 6}};
            cout << a << endl;

            using AI0 = decltype(a.iter<0>());
            cout << "AI0 " << std::is_same_v<int, AI0> << endl;
            cout << AI0::rank() << endl;

            using AI1 = decltype(a.iter<1>());
            cout << "AI1 " << std::is_same_v<int, AI1> << endl;
            cout << AI1::rank() << endl;

            AI0 bi(a.data());
            cout << bi.c.data() << endl;
        }
        tr.section("STL style, should be a pointer for default steps");
        {
            using A = ra::Small<int, 2, 3>;
            A a = {{1, 2, 3}, {4, 5, 6}};
            tr.test(std::is_same_v<int *, decltype(a.begin())>);
            for (int i=0; auto && ai: a) {
                tr.test_eq(i+1, ai);
                ++i;
            }
        }
        tr.section("STL style with non-default steps");
        {
            ra::Small<int, 2, 3> a = {{1, 2, 3}, {4, 5, 6}};
            auto b = transpose(a, int_list<1, 0>{});

            // we don't necessarily walk ind this way.
            // tr.test_eq(ra::start({0, 0}), ra::start(b.begin().ind));
            // tr.test_eq(ra::start({0, 1}), ra::start((++b.begin()).ind));

            tr.test(!std::is_same_v<int *, decltype(b.begin())>);
            int bref[6] = {1, 4, 2, 5, 3, 6};
            for (int i=0; auto && bi: b) {
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
                      test("1|2|3|4|5|6|", ra::int_c<-2>());
                      test("1 2 3|4 5 6|", ra::int_c<-1>());
                      test("1|2|3|4|5|6|", ra::int_c<0>());
                      test("1 2 3|4 5 6|", ra::int_c<1>());
                      test("1 2 3\n4 5 6|", ra::int_c<2>());
                  };
            tr.section("default steps");
            test_over(ra::Small<int, 2, 3> {{1, 2, 3}, {4, 5, 6}});
            tr.section("non-default steps");
            test_over(ra::transpose(ra::Small<int, 3, 2> {{1, 4}, {2, 5}, {3, 6}}, int_list<1, 0>{}));
        }
    }
    return tr.summary();
}
