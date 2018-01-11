
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file iterator.H
/// @brief Array iterator for SmallView, plus row-major STL iterator.
// TODO Refactor with cell_iterator for View?

#pragma once
#include "ra/traits.H"
#include "ra/opcheck.H"
#include <iostream>

#ifndef RA_CHECK_BOUNDS_ITERATOR_SMALL
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_ITERATOR_SMALL 1
  #else
    #define RA_CHECK_BOUNDS_ITERATOR_SMALL RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_ITERATOR_SMALL==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

template <class I> struct Print_iterator_small { I i; };
template <class I> auto print_iterator_small(I && i) { return Print_iterator_small<I> { std::forward<I>(i) }; }

template <class I>
std::ostream & operator<<(std::ostream & o, Print_iterator_small<I> && p)
{
    return (o << "we'll see");
}

// V is SmallBase (FIXME so why parameterize? apparently only for order-of-decl.)
template <class V, rank_t cellr_=0>
struct cell_iterator_small
{
    constexpr static rank_t cellr_spec = cellr_;
    static_assert(cellr_spec!=RANK_ANY && cellr_spec!=RANK_BAD, "bad cell rank");
    constexpr static rank_t fullr = V::rank_s();
    constexpr static rank_t cellr = dependent_cell_rank(fullr, cellr_spec);
    constexpr static rank_t framer = dependent_frame_rank(fullr, cellr_spec);
    static_assert(cellr>=0 || cellr==RANK_ANY, "bad cell rank");
    static_assert(framer>=0 || framer==RANK_ANY, "bad frame rank");
    static_assert(fullr==cellr || gt_rank(fullr, cellr), "bad cell rank");

    constexpr static rank_t rank_s() { return framer; }
    constexpr static rank_t rank() { return framer; }

    using cell_sizes = mp::Drop_<typename V::sizes, framer>;
    using cell_strides = mp::Drop_<typename V::strides, framer>;
    using sizes = mp::Take_<typename V::sizes, framer>; /* this are strides on atom_type * p !! */
    using strides = mp::Take_<typename V::strides, framer>;

    using shape_type = std::array<dim_t, framer>;
    using atom_type = typename V::value_type; // FIXME fix ra_traits<SmallBase>
    using cell_type = SmallView<atom_type, cell_sizes, cell_strides>;
    using value_type = mp::If_<0==cellr, atom_type, cell_type>;

    cell_type c;

    constexpr cell_iterator_small(): c { nullptr } {} // end marker only.
    constexpr cell_iterator_small(cell_iterator_small const & ci): c { ci.c.p } {}
// needed because View = View copies contents, not the View itself [ra11]
    constexpr cell_iterator_small & assign(cell_iterator_small const & ci)
    {
        c.p = ci.c.p; return *this;
    }

// see stl_iterator for the case of s_[0]=0, etc. [ra12].
    constexpr cell_iterator_small(atom_type * p_): c { p_ } {}

    constexpr static dim_t size_s()
    {
        using frame_type = SmallView<int, sizes, strides>; // only to compute ssizes
        return ra_traits<frame_type>::size_s();
    }
    constexpr static auto const & shape()
    {
        using frame_type = SmallView<int, sizes, strides>; // only to compute ssizes
        return frame_type::Base::ssizes;
    }
    constexpr static dim_t size(int j) { CHECK_BOUNDS(inside(j, rank())); return V::size(j); }
    constexpr static dim_t size_s(int j) { CHECK_BOUNDS(inside(j, rank())); return V::size(j); }
    constexpr static dim_t stride(int j) { return j<rank() ? V::stride(j) : 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j)
    {
        return step*(z<rank() ? stride(z) : 0)==(j<rank() ? stride(j) : 0);
    }
    constexpr void adv(rank_t k, dim_t d) { c.p += (k<rank()) * stride(k)*d; }

    constexpr auto flat() const
    {
        if constexpr (0==cellr) {
            return c.p;
        } else {
            return CellFlat<cell_type> { c };
        }
    }
// Return type to allow either View & or View const & verb. Can't set self b/c original p isn't kept. TODO Think this over.
    template <class I>
    constexpr decltype(auto) at(I const & i_)
    {
        CHECK_BOUNDS(rank()<=dim_t(i_.size()) && "too few indices");
        if constexpr (0==cellr) {
            return c.p[Indexer0<sizes, strides>::index_short(i_)];
        } else {
            return cell_type(c.p + Indexer0<sizes, strides>::index_short(i_));
        }
    }
    decltype(auto) star() const { if constexpr (0==cellr) return *c.p; else return c; }
    decltype(auto) star() { if constexpr (0==cellr) return *c.p; else return c; }

#define DEF_ASSIGNOPS(OP)                                               \
    template <class X> void operator OP(X && x)                         \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); } \
    void operator OP(cell_iterator_small const & x)                       \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class Iterator>
struct stl_iterator_small
{
    using value_type = typename Iterator::value_type;
    using difference_type = dim_t;
    using pointer = value_type *;
    using reference = value_type &;
    using const_reference = value_type const &;
    using iterator_category = std::forward_iterator_tag;
    using shape_type = typename Iterator::shape_type;

    Iterator ii;
    shape_type i;
    stl_iterator_small(): ii() {};
// avoid ii = it.ii, see [ra11] above.
    stl_iterator_small(stl_iterator_small const & it) = default;
    stl_iterator_small & operator=(stl_iterator_small const & it) { i=it.i; ii.assign(it.ii); return *this; }
// [ra12] Null p_ so begin()==end() for iterator on empty range. Plying uses sizes so this doesn't matter.
    stl_iterator_small(typename Iterator::atom_type * p_)
        : ii(i.size()==0 ? nullptr : p_),
          i(ra::ra_traits<shape_type>::make(Iterator::framer, 0)) {}

    template <class PP> bool operator==(PP const & j) const { return ii.c.p==j.ii.c.p; }
    template <class PP> bool operator!=(PP const & j) const { return ii.c.p!=j.ii.c.p; }
// taken from Iterator to handle the special case for cellr=0.
    decltype(auto) operator*() const { return ii.star(); }
    decltype(auto) operator*() { return ii.star(); }

    template <int k=Iterator::rank()-1>
    std::enable_if_t<(k>=0)> next_in_cube()
    {
        ++i[k];
        if (i[k]<mp::Ref_<typename Iterator::sizes, k>::value) {
            ii.c.p += mp::Ref_<typename Iterator::strides, k>::value;
        } else {
            i[k] = 0;
            ii.c.p -= mp::Ref_<typename Iterator::strides, k>::value*(mp::Ref_<typename Iterator::sizes, k>::value-1);
            next_in_cube<k-1>();
            return;
        }
    }
    template <int k> std::enable_if_t<(k<0)> next_in_cube() { ii.c.p = nullptr; }
    stl_iterator_small & operator++() { next_in_cube(); return *this; }
};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_ITERATOR_SMALL
