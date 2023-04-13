// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Check bad cell ranks

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// "ra/format.hh" doesn't depend directly on RA_ASSERT, so the following override is able to use ra::format.

#include "ra/format.hh"

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
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "] " __VA_OPT__(,) __VA_ARGS__); }

// The override will be in effect for the rest of ra::.

#include "ra/test.hh"
#include <iostream>

int main()
{
    ra::TestRecorder tr;
    tr.section("Bad cell rank");
    {
        bool yes = false;
        ra::Big<int> a0 = 0;
        std::string msg;
        try {
            std::cout << ra::iter<1>(a0) << std::endl;
        } catch (ra_error & e) {
            msg = e.what();
            yes = true;
        }
        tr.info(msg).test(yes);
    }
    return tr.summary();
}
