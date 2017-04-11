
// (c) Daniel Llorens - 2014-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-concrete.C
/// @brief Tests for concrete_type.

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/concrete.H"
#include "ra/test.H"
#include "ra/mpdebug.H"
#include <memory>

using std::cout; using std::endl;

int main()
{
    TestRecorder tr(std::cout);

    tr.section("concrete");
    {
        ra::Small<int, 3> a = {1, 2, 3};
        ra::Small<int, 3> b = {4, 5, 6};
        std::cout << "... " << mp::type_name<decltype(a+b)>() << std::endl;
        using K = typename ra::concrete_type_def<decltype(a+b)>::type;
        std::cout << "... " << mp::type_name<K>() << std::endl;
    }
    return tr.summary();
}
