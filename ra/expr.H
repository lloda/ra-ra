// -*- mode: c++; coding: utf-8 -*-
/// @file expr.H
/// @brief Operator nodes for expression templates.

// (c) Daniel Llorens - 2011-2014, 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/ply.H"
#include "ra/opcheck.H"

#ifndef RA_CHECK_BOUNDS_EXPR
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_EXPR 1
  #else
    #define RA_CHECK_BOUNDS_EXPR RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_EXPR==0
  #define CHECK_BOUNDS( cond )
#else
  #define CHECK_BOUNDS( cond ) RA_ASSERT( cond, 0 )
#endif

namespace ra {

// Manipulate ET through flat (raw pointer-like) iterators P ...
template <class Op, class T, class I=mp::iota<mp::len<T>>> struct Flat;

template <class Op, class T, int ... I>
struct Flat<Op, T, mp::int_list<I ...>>
{
    Op & op;
    T t;
    template <class S> constexpr void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
    constexpr decltype(auto) operator*() { return op(*std::get<I>(t) ...); }
};

template <class Op, class ... P> inline constexpr
auto flat(Op & op, P && ... p)
{
    return Flat<Op, std::tuple<P ...>> { op, std::tuple<P ...> { std::forward<P>(p) ... } };
}

template <class Op, class ... P> inline constexpr auto
expr(Op && op, P && ... p)
{
    return Expr<Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(p) ... };
}

// forward decl in atom.H
template <class Op, class ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P ...>;
    Op op;
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

// FIXME seems shape() and size() should be taken out.

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
    constexpr Expr(Op op_, P ... p_): op(std::forward<Op>(op_)), t(std::forward<P>(p_) ...)
    {
        CHECK_BOUNDS(check_expr(*this)); // TODO Maybe do this on ply? also do static part inconditionally.
    }

    template <class J>
    constexpr decltype(auto) at(J const & i)
    {
        return op(std::get<I>(t).at(i) ...);
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

    constexpr decltype(auto) flat()
    {
        return ra::flat(op, std::get<I>(t).flat() ...);
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(ra::flat(op, std::get<I>(t).flat() ...)))()
    {
        static_assert(rank_s()==0 || rank_s()==RANK_ANY || (rank_s()==1 && size_s()==1), // for coord types
                      "bad rank in conversion to scalar");
        assert(rank()==0 || (rank_s()==1 && size_s()==1)); // for coord types; so fixed only
        return *flat();
    }

// forward to make sure value y is not misused as ref [ra05].
#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

// -------------------

template <class Op, class ... A> inline constexpr auto
map(Op && op, A && ... a)
{
    return expr(std::forward<Op>(op), start(std::forward<A>(a)) ...);
}

template <class Op, class ... A> inline constexpr void
for_each(Op && op, A && ... a)
{
    ply(map(std::forward<Op>(op), std::forward<A>(a) ...));
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_EXPR
