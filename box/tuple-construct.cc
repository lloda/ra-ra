// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - If I were to construct Small from tuples

// (c) Daniel Llorens - 2018
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <cassert>
#include <iostream>
#include "ra/base.hh"

using std::tuple, std::cout, std::endl;

template <class T, class sizes> struct nested_tuple;

template <class T>
struct nested_tuple<T, ra::ilist_t<>>
{
    constexpr static int rank = 0;
    using type = T;
};

template <class T, class sizes>
struct nested_tuple
{
    constexpr static int rank = ra::mp::len<sizes>;
    using sub = typename nested_tuple<T, ra::mp::drop1<sizes>>::type;
    using type = ra::mp::makelist<ra::mp::ref<sizes, 0>::value, sub>;
    using atype = sub[ra::mp::ref<sizes, 0>::value];
};

struct foo
{
    int x = true;
    // foo(nested_tuple<int, ra::ilist_t<2, 3>>::type const & a) { cout << "A" << endl; }
    foo(nested_tuple<int, ra::ilist_t<2, 3>>::atype const & a) { cout << "A" << endl; }
};

int main()
{
    using sizes0 = ra::ilist_t<>;
    using sizes1 = ra::ilist_t<3>;
    using sizes2 = ra::ilist_t<3, 4>;
    using sizes3 = ra::ilist_t<3, 4, 5>;
    {
        std::cout << nested_tuple<int, sizes0>::rank << std::endl;
        std::cout << nested_tuple<int, sizes1>::rank << std::endl;
        std::cout << nested_tuple<int, sizes2>::rank << std::endl;
        std::cout << nested_tuple<int, sizes3>::rank << std::endl;
    }
// extra pair is required since it's not an initializer_list constructor.
    foo f( {{1, 2, 3}, {4, 5, 6}} );
    foo g{ {{1, 2, 3}, {4, 5, 6}} };
    foo h = { {{1, 2, 3}, {4, 5, 6}} };
    cout << f.x << endl;
    cout << g.x << endl;
    cout << h.x << endl;
// extra pair isn't required for the tuples themselves though :-/
    // nested_tuple<int, ra::ilist_t<2, 3>>::type i = {{1, 2, 3}, {4, 5, 6}};
    // cout << foo(i).x << endl;
    return 0;
}
