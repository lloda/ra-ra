// -*- mode: c++; coding: utf-8 -*-
/// @file pick.H
/// @brief Expression template that picks one of several arguments.

// (c) Daniel Llorens - 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// This class is needed because Expr evaluates its arguments before calling its
// operator. Note that pick() is a normal function, so its *arguments* will always
// be evaluated; it is the individual array elements that will not be.

// FIXME Expr and Pick are mostly identical, so a refactor is in order...

#pragma once
#include "ra/ply.H"
#include "ra/opcheck.H"

#ifndef RA_CHECK_BOUNDS_PICK
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_PICK 1
  #else
    #define RA_CHECK_BOUNDS_PICK RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_PICK==0
  #define CHECK_BOUNDS( cond )
#else
  #define CHECK_BOUNDS( cond ) RA_ASSERT( cond, 0 )
#endif

namespace ra {

// -----------------
// return type of pick expression, otherwise compiler complains of ambiguity.
// TODO & is crude, maybe is_assignable?
// -----------------

template <class T, class Enable=void>
struct pick_type
{
    using type = mp::apply<std::common_type_t, T>;
};

// lvalue
template <class P0, class ... P>
struct pick_type<std::tuple<P0 &, P ...>,
                 std::enable_if_t<!std::is_const_v<P0> && (std::is_same_v<P0 &, P> && ...)>>
{
    using type = P0 &;
};

// const lvalue
template <class P0, class ... P>
struct pick_type<std::tuple<P0 &, P & ...>,
                 std::enable_if_t<(std::is_same_v<std::decay_t<P0>, std::decay_t<P>> && ...)
                                  && (std::is_const_v<P0> || (std::is_const_v<P> || ...))>>
{
    using type = P0 const &;
};

// -----------------
// runtime to compile time conversion for Pick::at() and PickFlat::operator*()
// -----------------

template <class T, class J> struct pick_at_type;
template <class ... P, class J> struct pick_at_type<std::tuple<P ...>, J>
{
    using type = typename pick_type<std::tuple<decltype(std::declval<P>().at(std::declval<J>())) ...>>::type;
};
template <class T, class J> using pick_at_t = typename pick_at_type<mp::drop1<std::decay_t<T>>, J>::type;

template <size_t I, class T, class J> inline constexpr
pick_at_t<T, J>
pick_at(size_t p0, T && t, J const & j)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return std::get<I+1>(t).at(j);
        } else {
            return pick_at<I+1>(p0, t, j);
        }
    } else {
        CHECK_BOUNDS(p0==I);
        return std::get<I+1>(t).at(j);
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = typename pick_type<std::tuple<decltype(*std::declval<P>()) ...>>::type;
};
template <class T> using pick_star_t = typename pick_star_type<mp::drop1<std::decay_t<T>>>::type;

template <size_t I, class T> inline constexpr
pick_star_t<T>
pick_star(size_t p0, T && t)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return *(std::get<I+1>(t));
        } else {
            return pick_star<I+1>(p0, t);
        }
    } else {
        CHECK_BOUNDS(p0==I);
        return *(std::get<I+1>(t));
    }
}

// -----------------
// follows closely Flat, Expr
// -----------------

// Manipulate ET through flat (raw pointer-like) iterators P ...
template <class T, class I=mp::iota<mp::len<T>>>
struct PickFlat;

template <class P0, class ... P, int ... I>
struct PickFlat<std::tuple<P0, P ...>, mp::int_list<I ...>>
{
    std::tuple<P0, P ...> t;
    template <class S> void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
    decltype(auto) operator*() { return pick_star<0>(*std::get<0>(t), t); }
};

template <class P0, class ... P> inline constexpr
auto pick_flat(P0 && p0, P && ... p)
{
    return PickFlat<std::tuple<P0, P ...>> { std::tuple<P0, P ...> { std::forward<P0>(p0), std::forward<P>(p) ... } };
}

template <class P0, class ... P> inline constexpr auto
pick_in(P0 && p0, P && ... p)
{
    return Pick<std::tuple<P0, P ...>> { std::forward<P0>(p0), std::forward<P>(p) ... };
}

template <class P0, class ... P> inline constexpr auto
pick(P0 && p0, P && ... p)
{
    return pick_in(start(std::forward<P0>(p0)), start(std::forward<P>(p)) ...);
}

// forward decl in atom.H
template <class P0, class ... P, int ... I>
struct Pick<std::tuple<P0, P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P0, P ...>;
    T t;

    template <class T> struct box { using type = T; };

// rank of largest subexpr. This is true for either prefix or suffix match.
    constexpr static rank_t rank_s()
    {
        return mp::fold_tuple(RANK_BAD, mp::map<box, T> {},
                              [](rank_t r, auto a)
                              {
                                  constexpr rank_t ar = std::decay_t<typename decltype(a)::type>::rank_s();
                                  return gt_rank(r, ar) ?  r : ar;
                              });
    }
    constexpr rank_t rank() const
    {
        if constexpr (constexpr rank_t rs=rank_s(); rs==RANK_ANY) {
            return mp::fold_tuple(RANK_BAD, t,
                                  [](rank_t r, auto && a)
                                  {
                                      rank_t ar = a.rank();
                                      assert(ar!=RANK_ANY); // cannot happen at runtime
                                      return gt_rank(r, ar) ?  r : ar;
                                  });
        } else {
            return rs;
        }
    }

// any size which is not DIM_BAD.
    constexpr static dim_t size_s(int k)
    {
        dim_t s = mp::fold_tuple(DIM_BAD, mp::map<box, T> {},
                                 [&k](dim_t s, auto a)
                                 {
                                     using A = std::decay_t<typename decltype(a)::type>;
                                     constexpr rank_t ar = A::rank_s();
                                     if (s!=DIM_BAD) {
                                         return s;
                                     } else if (ar>=0 && k>=ar) {
                                         return s;
                                     } else {
                                         dim_t zz = A::size_s(k);
                                         return zz;
                                     }
                                 });
        return s;
    }

// do early exit with fold_tuple (and with size_s(k)).
    constexpr dim_t size(int k) const
    {
        if (dim_t ss=size_s(k); ss==DIM_ANY) {
            auto f = [this, &k](auto && f, auto ii)
                     {
                         constexpr int i = decltype(ii)::value;
                         if constexpr (i<std::tuple_size_v<T>) {
                             auto const & a = std::get<i>(this->t);
                             if (k<a.rank()) {
                                 dim_t as = a.size(k);
                                 if (as!=DIM_BAD) {
                                     assert(as!=DIM_ANY); // cannot happen at runtime
                                     return as;
                                 } else {
                                     return f(f, mp::int_t<i+1> {});
                                 }
                             } else {
                                 return f(f, mp::int_t<i+1> {});
                             }
                         } else {
                             assert(0);
                             return DIM_BAD;
                         }
                     };
            return f(f, mp::int_t<0> {});
        } else {
            return ss;
        }
    }

    constexpr decltype(auto) shape() const
    {
        if constexpr (rank_s()==RANK_ANY) {
            std::vector<dim_t> s(rank());
            for_each([this, &s](int k) { s[k] = this->size(k); }, ra::iota(rank()));
            return s;
            // return map([this](int k) { return this->size(k); }, ra::iota(rank()));
        } else {
            std::array<dim_t, rank_s()> s {};
            for_each([this, &s](int k) { s[k] = this->size(k); }, ra::iota(rank_s()));
            return s;
        }
    }

    constexpr dim_t size()
    {
        dim_t c = 1;
        for_each([&c](auto && a) { c *= a; }, this->shape());
        return c;
    }

    constexpr static dim_t size_s()
    {
        if constexpr (rank_s()==RANK_ANY) {
            return DIM_ANY;
        } else {
            ra::dim_t s = 1;
            for (int i=0; i!=rank_s(); ++i) {
                if (dim_t ss=size_s(i); ss>=0) {
                    s *= ss;
                } else {
                    return ss; // either DIM_ANY or DIM_BAD
                }
            }
            return s;
        }
    }

// see test/ra-9.C [ra01] for forward() here.
    constexpr Pick(P0 p0_, P ... p_): t(std::forward<P0>(p0_), std::forward<P>(p_) ...)
    {
        CHECK_BOUNDS(check_expr(*this)); // TODO Maybe do this on ply?
    }

    template <class J>
    constexpr decltype(auto) at(J const & j)
    {
        return pick_at<0>(std::get<0>(t).at(j), t, j);
    }

    constexpr void adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }

    constexpr bool keep_stride(dim_t step, int z, int j) const
    {
        return (std::get<I>(t).keep_stride(step, z, j) && ...);
    }

    constexpr auto stride(int i) const
    {
        return std::make_tuple(std::get<I>(t).stride(i) ...);
    }

    constexpr auto flat()
    {
        return pick_flat(std::get<I>(t).flat() ...);
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(pick_flat(std::get<I>(t).flat() ...)))()
    {
        static_assert(rank_s()==0 || rank_s()==RANK_ANY || (rank_s()==1 && size_s()==1), // for coord types
                      "bad rank in conversion to scalar");
        assert(rank()==0 || (rank_s()==1 && size_s()==1)); // for coord types; so fixed only
        return *flat();
    }

// forward to make sure value y is not misused as ref [ra05]
#define DEF_ASSIGNOPS(OP) template <class X> void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_PICK
