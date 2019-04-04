
// (c) Daniel Llorens - 2019

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Replace ra:: asserts with customs ones.

#include <exception>
#include "ra/format.H"

struct ra_error: public std::exception
{
    std::string s;
    ra_error(std::string const & s_=""): s(s_) {}
    virtual char const * what() const throw ()
    {
        return s.c_str();
    }
};

// RA_ASSERT has to be defined before any "ra/" header to override the default definition of RA_ASSERT ("ra/format.H" is an independent header and doesn't count).

#define RA_ASSERT( cond ) \
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "]"); }

#include <iostream>
#include <thread>
#include <string>
#include "ra/test.H"
#include "ra/io.H"
#include "ra/operators.H"
#include "ra/format.H"

using std::cout, std::endl;

int main()
{
    TestRecorder tr(cout);
    bool yes = false;
    ra::Big<int> a({2, 3}, 9);
    try {
        cout << a(2, 3) << endl;
    } catch (ra_error & e) {
        cout << e.what() << endl;
        yes = true;
    }
    tr.test(yes);
    return tr.summary();
}
