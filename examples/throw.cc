// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - Customize ra:: reaction to errors.

// (c) Daniel Llorens - 2019-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifdef RA_DO_CHECK
  #undef RA_DO_CHECK
#endif
#define RA_DO_CHECK 1 // kind of the point here

// "ra/base.hh" doesn't depend directly on RA_ASSERT, so the following override is able to use ra::format.

#include "ra/base.hh"
#include <exception>

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
    { if (!( cond )) throw ra_error("ra::", std::source_location::current(), " (" STRINGIZE(cond) ") " __VA_OPT__(,) __VA_ARGS__); }

// The override will be in effect for the rest of ra::.

#include "ra/ra.hh"
#include "ra/test.hh"
#include <iostream>

using std::cout, std::endl, ra::TestRecorder;

int main()
{
    TestRecorder tr(cout);
    bool yes = false;
    ra::Big<int> a({2, 3}, 9);
    std::string msg;
    try {
        cout << a(2, 3) << endl;
    } catch (ra_error & e) {
        msg = e.what();
        yes = true;
    }
    tr.info(msg).test(yes);
    return tr.summary();
}
