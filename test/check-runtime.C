
// (c) Daniel Llorens - 2018

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file check-runtime.C
/// @brief Check the mechanism for abort/throw choice in size & rank checks.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/test.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/pick.H"
#include <typeinfo>
#include <string>

using std::cout; using std::endl; using std::flush;

namespace ra {

// FIXME ...
template <class E>
struct Check
{
};

} // namespace ra

int main()
{
    TestRecorder tr(std::cout);

    return tr.summary();
}
