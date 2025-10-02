// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Special Dimv to let Cell<> replace Ptr

// (c) Daniel Llorens - 2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "test/mpdebug.hh"

using std::cout, std::endl, std::flush;

namespace ra {

// both N & S can be has_len<N> || is_constant<N> || std::is_integral_v<N>.
// [ ] fix CellPtr len/step/keep to be appropriately static or not. Maybe have sdimv with ANYs where needed.

template <class N, class S>
struct SDim { [[no_unique_address]] N const len = {}; [[no_unique_address]] S const step = {}; };

template <class N, class S>
using SDimv = std::array<SDim<N, S>, 1>;

template <class P, class N, class S>
using CellPtr = Cell<P, SDimv<N, S>, ic_t<0>>;

} // namespace ra

int main()
{
    ra::TestRecorder tr;
    tr.section("dim/1");
    {
        using N = ra::dim_t;
        using S = ra::ic_t<1>;
        using P = ra::Seq<ra::dim_t>;
        ra::CellPtr<P, N, S> cptr(P {0}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> { .len=10 }});
        // tr.test_eq(1, decltype(cptr)::step(0));
        // tr.test_eq(ra::ANY, decltype(cptr)::len_s(0));
        // tr.test_eq(10, cptr.len(0));
        cout << sizeof(cptr) << endl;
        cout << cptr << endl;
    }
    tr.section("4/1");
    {
        using N = ra::ic_t<4>;
        using S = ra::ic_t<2>;
        using P = ra::Seq<ra::dim_t>;
        ra::CellPtr<P, N, S> cptr(P {-9}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> {}});
        // tr.test_eq(2, decltype(cptr)::step(0));
        // tr.test_eq(4, decltype(cptr)::len(0));
        cout << sizeof(cptr) << endl;
        cout << cptr << endl;
    }
    tr.section("len/1");
    {
        using N = decltype(ra::len);
        using S = ra::ic_t<1>;
        using P = ra::Seq<ra::dim_t>;
        ra::CellPtr<P, N, S> cptr(P {0}, std::array<ra::SDim<N, S>, 1> {ra::SDim<N, S> {}});
        // tr.test_eq(1, decltype(cptr)::step(0));
        cout << sizeof(cptr.dimv) << endl;
        cout << sizeof(cptr.c) << endl;
        cout << sizeof(cptr) << endl;
    }
    tr.section("misc");
    {
        cout << sizeof(ra::ViewBig<int *, 0>) << endl;
    }
    tr.section("lens");
    {
        cout << ra::mp::type_name<decltype(ra::map(std::plus<>(), ra::iota(ra::ic<4>), ra::iota(4, 0, 2)).len(ra::ic<0>))>() << endl;
// this is static 4 even though runtime check is still needed.
        cout << ra::mp::type_name<decltype(ra::clen(ra::map(std::plus<>(), ra::iota(ra::ic<4>), ra::iota(5, 0, 2)), ra::ic<0>))>() << endl;
    }
    return tr.summary();
}
