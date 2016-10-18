
// (c) Daniel Llorens - 2014, 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-small.C
/// @brief WIP: creating a higher-rank iterator for SmallArray/SmallView.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/small.H"
#include "ra/iterator.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/large.H"
#include "ra/small.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/mpdebug.H"

using std::cout; using std::endl; using std::flush;

template <int i, class V> struct small_iterator_def;

template <int i_, template <class, class, class> class Child, class T, class sizes, class strides>
struct small_iterator_def<i_, ra::SmallBase<Child, T, sizes, strides>>
{
    constexpr static int i = i_;
    using V = ra::SmallBase<Child, T, sizes, strides>;
    using type = ra::ra_iterator<V, i>;
};

template <int i, class V> using small_iterator = small_iterator_def<i, typename std::decay_t<V>::Base>;

int main()
{
    TestRecorder tr;
    {
        using A = ra::Small<int, 2, 3>;
        A a(0.);
        cout << a << endl;

        using AI = typename small_iterator<0, A>::type;
        cout << std::is_same<int, AI>::value << endl;

        cout << small_iterator<0, A>::i << endl;

        AI ai {}; // @TODO start fixing cell_iterator here
        cout << ai.p << endl;
    }
    return tr.summary();
}
