// -*- mode: c++; coding: utf-8 -*-
// ek/box - A properly general version of view(...)

// (c) Daniel Llorens - 2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Atm view(i ...) only beats i of rank ≤1 and only when all i are beatable. This is a sandbox for a version that beats any combination of scalar or view<Seq> of any rank, and only returns an expr for the unbeatable subscripts.
// There is an additional issue that eg A2(i1) (indicating ranks) returns a nested expression, that is, a rank-1 expr where each element is a rank-1 view. We'd prefer if the result was rank 2 and not nested, iow, the value type of the result should always be the same as that of A.

#include "ra/test.hh"
#include <iomanip>
#include <chrono>
#include <span>

using std::cout, std::endl, std::flush;

namespace ra {

template <int rank>
constexpr auto ii(dim_t (&&s)[rank])
{
    return ViewBig<Seq<dim_t>, rank>(s, Seq<dim_t>{0});
}

template <class A, class ... I>
constexpr auto
genfrom(A && a, I && ...  i)
{


}

}; // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
    cout << ra::ii({3, 4}) << endl;
    return tr.summary();
}
x
