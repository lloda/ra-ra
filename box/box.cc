// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Random tests

// (c) Daniel Llorens - 2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "test/mpdebug.hh"

using std::cout, std::endl, std::flush;

// Would like to get rid of the dependence View(nested_args(SmallArray)) in arrays.hh, but not all conversions work the same way.

namespace ra {

template <class T, class I> struct btt;
template <class T, int i0, int ... i> struct btt<T, ilist_t<i0, i ...>> { using type = typename btt<T, ilist_t<i ...>>::type [i0]; };
template <class T> struct btt<T, ilist_t<>> { using type = T; };

template <class T, class Dimv> struct bttq;

template <class T, class Dimv> requires (is_ctype<Dimv>)
struct bttq<T, Dimv>
{
    using type = btt<T, decltype(std::apply([](auto ... i){ return ilist<Dimv::value[i].len ...>; }, mp::iota<Dimv::value.size()>{}))>::type;
};
template <class T, class Dimv> requires (!is_ctype<Dimv>)
struct bttq<T, Dimv>
{
    using type = noarg<Dimv>;
};

template <class T, class Dimv> using bttt = bttq<T, Dimv>::type;

} // namespace ra

auto f(int (&&a)[2][3][4])
{
    cout << "print f:\n" << ra::iter(a) << endl;
}

auto g(ra::bttt<int, ra::Small<int, 2, 3, 4>::Dimv> && a)
{
    cout << "print g:\n" << ra::iter(a) << endl;
}

int main()
{
    int a[2][3][4] = {};
    cout << "a " << ra::mp::type_name<decltype(a)>() << endl;
    f(std::move(a));

    ra::btt<int, ra::ilist_t<2, 3, 4>>::type b = {};
    cout << "b " << ra::mp::type_name<decltype(b)>() << endl;
    f(std::move(b));

    ra::bttt<int, ra::Small<int, 2, 3, 4>::Dimv> c = {};
    cout << "c " << ra::mp::type_name<decltype(c)>() << endl;
    f(std::move(c));

    f({{{0,1,2,3},{1,2,3,4},{2,3,4,5}},{{3,4,5,6},{4,5,6,7},{5,6,7,8}}});

    g({{{0,1,2,3},{1,2,3,4},{2,3,4,5}},{{3,4,5,6},{4,5,6,7},{5,6,7,8}}});

    return 0;
}
