
// (c) Daniel Llorens - 2013-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_BOOTSTRAP_H
#define RA_BOOTSTRAP_H

/// @file ra-bootstrap.H
/// @brief Multidimensional arrays. Before all other includes.

#include "ra/tuple-list.H"
#include "ra/tuple-dynamic.H"

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_BOOTSTRAP RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_BOOTSTRAP
        #define RA_CHECK_BOUNDS_RA_BOOTSTRAP 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_BOOTSTRAP==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

static_assert(sizeof(int)>=4, "bad assumption on int");
using rank_t = int;
using dim_t = ssize_t;
dim_t const DIM_ANY = -1073741823;
dim_t const DIM_BAD = -1073741824;
rank_t const RANK_ANY = DIM_ANY;
rank_t const RANK_BAD = DIM_BAD;

// utilities.

constexpr dim_t dim_prod(dim_t const a, dim_t const b)
{
    return (a==DIM_ANY) ? DIM_ANY : ((b==DIM_ANY) ? DIM_ANY : a*b);
}
constexpr rank_t rank_sum(rank_t const a, rank_t const b)
{
    return (a==RANK_ANY) ? RANK_ANY : ((b==RANK_ANY) ? RANK_ANY : a+b);
}

// used in array constructors.

enum class init_not_t { value };
constexpr init_not_t init_not = init_not_t::value;
enum class default_init_t { value };
constexpr default_init_t default_init = default_init_t::value;

// This is returned by ra_iterator::flat() when cellr > 0, or SmallBase::iterator::flat (@TODO When?)
template <class C>
struct cellp
{
    C c;
    void operator+=(dim_t const s) { c.p += s; }
    C & operator*() { return c; }

    constexpr auto operator+(dim_t const s) const { return c.p+s; }
    template <class P> bool & operator!=(P const & p_) const { return c.p!=p_; }
    template <class P> bool & operator==(P const & p_) const { return c.p==p_; }
};

// After next_in_cube() in next-in.H.
template <class S, class I, class P>
inline void next_in_cube(rank_t const framer, S const & dim, I & i, P & p)
{
    for (int j=framer-1; j>=0; --j) {
        ++i[j];
        if (i[j]<dim[j].size) {
            p += dim[j].stride;
            return;
        } else {
            i[j] = 0;
            p -= dim[j].stride*(dim[j].size-1);
        }
    }
    p = nullptr;
}

// Order of decl issues.
template <class T, rank_t RANK=RANK_ANY> struct Raw;

// Order of decl issues. Decl here because of ra_traits::shaped/make in ra-traits.H.
template <class T, class sizes, class strides> struct SmallArray;

template <class S> struct default_strides {};

template <class t0, class t1, class ... ti>
struct default_strides<std::tuple<t0, t1, ti ...>>
{
    using rest = typename default_strides<std::tuple<t1, ti ...>>::type;
    static int const stride0 = t1::value * mp::First_<rest>::value;
    using type = mp::Cons_<mp::int_t<stride0>, rest>;
};

template <class tend>
struct default_strides<std::tuple<tend>> { using type = mp::int_list<1>; };

template <>
struct default_strides<std::tuple<>> { using type = mp::int_list<>; };

template <class S> using default_strides_ = typename default_strides<S>::type;

template <class T, dim_t ... sizes>
using Small = SmallArray<T, mp::int_list<sizes ...>, default_strides_<mp::int_list<sizes ...> >>;

// Slice basic data type.
struct Dim { dim_t size, stride; };

// Order of decl issues.
template <class V> struct ra_traits_def;

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_BOOTSTRAP

#endif // RA_BOOTSTRAP_H
