// -*- mode: c++; coding: utf-8 -*-
/// @file macros.cc
/// @brief test type list library based on tuples.

// (c) Daniel Llorens - 2010
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/macros.hh"

using std::cout, std::endl;

int main()
{
    int errors = 0;
    int a = 0;
#define ADDTHIS(x) a += x;
    FOR_EACH(ADDTHIS, 1, 2, 3);
    errors += (6!=a);
    a = 0;
    FOR_EACH(ADDTHIS, 7, 0);
    errors += (7!=a);
    a = 0;
    FOR_EACH(ADDTHIS, 3);
    errors += (3!=a);
#undef ADDTHIS
    std::cout << errors << " errors" << endl;
    return errors;
}
