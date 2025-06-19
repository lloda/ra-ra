// -*- mode: c++; coding: utf-8 -*-
// ek/box - A properly general version of view(...)

// (c) Daniel Llorens - 2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Atm view(i ...) only beats i of rank ≤1 and only when all i are beatable. This is a sandbox for a version that beats any combination of scalar or view<Seq> of any rank, and only returns an expr for the unbeatable subscripts.
// There is an additional issue that eg A2(unbeatable-i1) (indicating ranks) returns a nested expression, that is, a rank-1 expr where each element is a rank-1 view. It should instead be rank 2 and not nested, iow, the value type of the result should always be the same as that of A.
// 1) Need to split the static and the dynamic parts so some/most of the routine can be used for Small or Big
// 2) Need to find a way to use len like in Ptr-based iota.

#include "ra/test.hh"

using std::cout, std::endl, std::flush;

namespace ra {

template <int rank>
constexpr auto ii(dim_t (&&s)[rank])
{
    return ViewBig<Seq<dim_t>, rank>(s, Seq<dim_t>{0});
}

template <std::integral auto ... i>
constexpr auto ii(ra::ilist_t<i ...>)
{
    return ViewSmall<Seq<dim_t>, ra::ic_t<ra::default_dims(std::array<ra::dim_t, sizeof...(i)>{i...})>>(Seq<dim_t>{0});
}

template <class A> concept is_iota_static = requires (A a)
{
    []<class I, class Dimv>(ViewSmall<Seq<I>, Dimv> const &){}(a);
    requires UNB!=ra::size(a); // exclude UNB from beating to let B=A(... i ...) use B's len. FIXME
};

template <class A> concept is_iota_dynamic = requires (A a)
{
    []<class I, rank_t RANK>(ViewBig<Seq<I>, RANK> const &){}(a);
};

template <class A> concept is_iota_any = is_iota_dynamic<A> || is_iota_static<A>;

template <class A, class ... I>
constexpr auto
genfrom(A && a, I && ...  i)
{
    std::vector<int> dest;
    std::vector<Dim> dimv;
    std::vector<int> unbeateni;
    dim_t place = 0;
    int k = 0;
    for (int j=0; j<sizeof...(i);  ++j) {
        if (i is scalar) {
            place += i*a.dimv[k].step;
            ++k;
        } else if (i is iota) {
            for (int q=0; q<rank(i); ++q) {
                dimv.push_back(Dim {i.dimv[q].len, i.dimv[q].step*a.dimv[k].step});
                place += i.i * a.dimv[k].step;
            }
            ++k;
        } else if (i is dots) {
            int n = (i.n==UNB) ? a.rank() - ...;
            for (int q=0; q<n; ++q) {
                dimv.push_back(a.dimv[k]);
                ++k;
            }
        } else if (i is insert) {
            for (q=0; q<i.n; ++q) {
                dimv.push_back(Dim {UNB, 0});
            }
        } else if (i is indices) {
            dest.push_back(k);
            add i to unbeateni;
        }
    }
}

}; // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
    cout << ra::ii({3, 4}) << endl;
    cout << ra::ii(ra::ilist<3, 4>) << endl;
    return tr.summary();
}
