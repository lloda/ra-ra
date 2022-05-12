// -*- mode: c++; coding: utf-8 -*-
// ra/test - Bug or not?

// (c) Daniel Llorens - 2013-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/format.hh"


// -------------------------------------
// bit from example/throw.cc which FIXME should be easier. Maybe an option in ra/macros.hh.

struct ra_error: public std::exception
{
    std::string s;
    template <class ... A> ra_error(A && ... a): s(ra::format(std::forward<A>(a) ...)) {}
    virtual char const * what() const throw ()
    {
        return s.c_str();
    }
};

#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "]", ##__VA_ARGS__); }
// -------------------------------------

#include "ra/test.hh"
#include "ra/ra.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);

    tr.section("check rank errors with var rank");
    {
        int x = 0;
        try {
            ra::Big<int> a = 0;
            cout << a.len(0) << endl;
            x = 1;
        } catch (ra_error & e) {
            x = 2;
        }
        tr.info("caught error").test_eq(2, x);
#undef EXPR
    }
    return tr.summary();
}
