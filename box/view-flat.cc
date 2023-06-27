// -*- mode: c++; coding: utf-8 -*-
// ek/box - Can I do View<Iota>, what does it look like?

// (c) Daniel Llorens - 2018
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

struct Len {};
constexpr Len len {};

template <> constexpr bool is_scalar_def<Len> = true;
template <> constexpr bool is_special_def<Len> = true;
template <> constexpr bool is_special_def<Scalar<Len>> = true;
template <> constexpr bool is_special_def<Scalar<Len const &>> = true;
template <> constexpr bool is_special_def<Scalar<Len &>> = true;

// FIXME remove the need for these stubs
template <class A> Len operator+(A a, Len b) { return len; }
template <class A> Len operator+(Len a, A b) { return len; }
Len operator+(Len a, Len b) { return len; }

std::ostream & operator<<(std::ostream & o, Len const a) { return o << "len"; }

template <class G, class E>
constexpr auto
inject(G && g, E && e)
{
    return std::apply([&](auto && ... x)
                      {
                          cout << "  X [";
                          (cout << ... << std::forward<decltype(x)>(x));
                          cout << "]" << endl;

                          return expr([g, &e](auto && ... y) {
                                  cout << "  G [";
                                  (cout << ... << g(std::forward<decltype(y)>(y)));
                                  cout << "]" << endl;
                                  return std::invoke(std::forward<E>(e).op, g(std::forward<decltype(y)>(y)) ...);
                              },
                              std::forward<decltype(x)>(x) ...);
                      },
        std::forward<E>(e).t);
}

struct WithSize;

dim_t with_size(dim_t e, dim_t len) { cout << "INT"; return e; }
dim_t with_size(Len e, dim_t len) { cout << "LEN"; return len; }
dim_t with_size(Scalar<Len> e, dim_t len) { cout << "SCA"; return len; }
template <IteratorConcept E> auto with_size(E && e, dim_t len)
{
    if constexpr (std::is_same_v<std::decay_t<E>, Scalar<Len>>) {
        cout << "ICS";
        return len;
    } else {
        cout << "ICE";
        return ra::inject(WithSize(len), std::forward<E>(e));
    }
}

struct WithSize
{
    dim_t len;
    WithSize(dim_t len_): len(len_) {}
    template <class E> auto operator()(E && e) const { cout << "RA" << endl; return with_size(std::forward<E>(e), len); }
};


} // namespace ra

int main()
{
    {
        auto a = ra::iota(12);
        cout << "a: " << a << endl;
    }
    {
        cout << ra::ra_irreducible<ra::Len, ra::Len> << endl;
        cout << ra::ra_reducible<ra::Len, ra::Len> << endl;
        cout << ra::ra_irreducible<ra::Scalar<ra::Len>, ra::Scalar<ra::Len>> << endl;
        cout << ra::ra_reducible<ra::Scalar<ra::Len>, ra::Scalar<ra::Len>> << endl;
        cout << ra::ra_reducible<int, int> << endl;
        cout << ra::ra_irreducible<int, int> << endl;
        cout << ra::ra_reducible<ra::Scalar<int>, ra::Scalar<int>> << endl;
        cout << ra::ra_irreducible<ra::Scalar<int>, ra::Scalar<int>> << endl;
        // cout << ">> X" << endl << (ra::len + ra::len).rank() << endl;
        // cout << ">> Y" << endl << (ra::scalar(ra::len) + ra::scalar(ra::len)).rank() << endl;
        // cout << ">> A" << endl << ra::map(std::plus(), ra::len, ra::len).rank() << endl;
        // cout << ">> X" << endl << with_size(ra::len + ra::len, 5) << endl;
        cout << ">>>>>>>>>>> A" << endl << ra::with_size(ra::map(std::plus(), 99, 1), 5) << endl;
        cout << ">>>>>>>>>>> B" << endl << ra::with_size(ra::map(std::plus(), ra::len, ra::len), 5) << endl;
        cout << ">>>>>>>>>>> C" << endl << with_size(ra::map(std::plus(), 99, ra::len), 5) << endl;
        cout << ">>>>>>>>>>> D" << endl << with_size(ra::map(std::plus(), ra::len, ra::len), 5) << endl;
// FIXME
        cout << ">>>>>>>>>>> E" << endl << with_size(ra::map(std::plus(), ra::len, ra::map(std::plus(), 3, ra::len)), 8) << endl;
        cout << ">>>>>>>>>>> F" << endl << with_size(ra::map(std::plus(), ra::len, with_size(ra::map(std::plus(), 3, ra::len), 8)), 8) << endl;
    }
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
    return 0;
}
