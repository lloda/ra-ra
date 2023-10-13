// -*- mode: c++; coding: utf-8 -*-
// ek/box - Can I do View<Iota>, what does it look like?

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#include "ra/mpdebug.hh"
#include <iomanip>
#include <chrono>
#include <span>

using std::cout, std::endl, std::flush;

namespace ra {

}; // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
// TODO if dimv can be span/ptr, no need to allocate it in CellBig. But View can be standalone, so...
    // {
    //     using ptr_type = std::span<ra::Dim, 2>;
    //     ra::Big<int, 2> a({2, 3}, 0.);
    //     ptr_type x = a.dimv;
    //     ra::View<int, 2, ptr_type> b { x, a.data() };
    //     cout << "b.dimv " << b.dimv << endl;
    //     cout << "a.data() " << a.data() << endl;
    //     cout << "b.data() " << b.data() << endl;
    //     cout << "a " << a << endl;
    //     cout << "b " << b << endl;
    // }
    // {
    //     using ptr_type = ra::Ptr<decltype(std::declval<default_view::Dimv>().begin()), ra::DIM_ANY>;
    //     ra::Big<int, 2> a({2, 3}, 0.);
    //     ptr_type x = ra::ptr(a.dimv.begin(), 2);
    //     cout << "x: " << x << endl;
    //     ra::View<int, 2, ptr_type> b { x, a.data() };
    //     cout << "b.dimv " << b.dimv << endl;
    //     cout << "a.data() " << a.data() << endl;
    //     cout << "b.data() " << b.data() << endl;
    //     cout << "a " << a << endl;
    //     cout << "b " << b << endl;
    // }
// template <class T> struct ravel_init { T data; };
// template <class T> ravel_init(T && t) -> ravel_init<T>;
// template <class T> ravel_init(std::initializer_list<T> && t) -> ravel_init<std::initializer_list<T>>;
    return tr.summary();
}
