
// (c) Daniel Llorens - 2013-2014, 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file iterator.H
/// @brief Array iterator for View, plus row-major STL iterator.

#pragma once
#include "ra/traits.H"
#include "ra/opcheck.H"
#include <iostream>

#ifndef RA_CHECK_BOUNDS_ITERATOR
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_ITERATOR 1
  #else
    #define RA_CHECK_BOUNDS_ITERATOR RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_ITERATOR==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

template <class I> struct Print_iterator { I i; };
template <class I> auto print_iterator(I && i) { return Print_iterator<I> { std::forward<I>(i) }; }

template <class I>
std::ostream & operator<<(std::ostream & o, Print_iterator<I> && p)
{
    o << "<cellr " << p.i.cellr << "[";
    for (int k=0; k<p.i.rank(); ++k) {
        o << k << ": " << p.i.dim[k].size << ", " << p.i.dim[k].stride << (k+1!=p.i.rank() ? "; " : "");
    }
    o << "]>";
    return o;
}

// V is View (FIXME so why parameterize? apparently only for order-of-decl.)
template <class V, rank_t cellr_=0>
struct cell_iterator
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
    constexpr rank_t rank() const { return dependent_frame_rank(rank_t(dim.size()), cellr_spec); }

    using dims_p = decltype(std::declval<V const>().dim); // FIXME can this be & ?
    dims_p dim; // FIXME take V & and ask through that. Should be const.

    using shape_type = mp::If_<rank_s()==DIM_ANY, std::vector<dim_t>, Small<dim_t, rank_s()>>;
    using atom_type = typename ra_traits<V>::value_type;
    using cell_type = View<atom_type, cellr>;
    using value_type = mp::If_<0==cellr, atom_type, cell_type>;

    cell_type c;

    constexpr cell_iterator() { c.p = nullptr; } // end marker only.
    constexpr cell_iterator(cell_iterator const & ci): dim(ci.dim), c { ci.c.dim, ci.c.p } {}
// needed because View = View copies contents, not the View itself [ra11]
    constexpr cell_iterator & assign(cell_iterator const & ci)
    {
        dim = ci.dim; c.dim = ci.c.dim; c.p = ci.c.p; return *this;
    }

// s_ is array's full shape; split it into dim/i (frame) and c (cell).
    template <class SS> constexpr
    cell_iterator(SS const & s_, atom_type * p_): dim(s_)
    {
        rank_t const rank = this->rank();
// see stl_iterator for the case of s_[0]=0, etc. [ra12].
        c.p = p_;
        rank_t const cellr = dependent_cell_rank(rank_t(dim.size()), cellr_spec);
        resize(c.dim, cellr);
        for (int k=0; k<cellr; ++k) {
            c.dim[k] = s_[k+rank];
        }
    }
    constexpr static dim_t size_s()
    {
        constexpr dim_t fullsize_s = ra_traits<V>::size_s();
        return fullsize_s==DIM_ANY ? DIM_ANY : fullsize_s / ra_traits<cell_type>::size_s();
    }
    constexpr shape_type shape() const
    {
        rank_t const iend = rank();
        shape_type s(ra_traits<shape_type>::make(iend));
        for (rank_t i=0; i!=iend; ++i) {
            s[i] = dim[i].size;
        }
        return s;
    }
    constexpr dim_t size(int j) const { CHECK_BOUNDS(inside(j, rank())); return dim[j].size; }
    constexpr dim_t stride(int j) const { return j<rank() ? dim[j].stride : 0; }
// FIXME k<rank() ?? check cell_iterator_small versions.
    constexpr bool keep_stride(dim_t step, int z, int j) const { return step*stride(z)==stride(j); }
    constexpr void adv(rank_t k, dim_t d) { c.p += stride(k)*d; }

    auto flat() const
    {
        if constexpr (0==cellr) {
            return c.p;
        } else {
            return CellFlat<cell_type> { c };
        }
    }
// Return type to allow either View & or View const & verb. Can't set self b/c original p isn't kept. TODO Think this over.
    template <class I>
    decltype(auto) at(I const & i_)
    {
        CHECK_BOUNDS(rank()<=dim_t(i_.size()) && "too few indices");
        if constexpr (0==cellr) {
            return c.p[Indexer1::index_short(rank(), dim, i_)];
        } else {
            return cell_type { c.dim, c.p+Indexer1::index_short(rank(), dim, i_) };
        }
    }
    decltype(auto) star() const { if constexpr (0==cellr) return *c.p; else return c; }
    decltype(auto) star() { if constexpr (0==cellr) return *c.p; else return c; }

#define DEF_ASSIGNOPS(OP)                                               \
    template <class X> void operator OP(X && x)                         \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); } \
    void operator OP(cell_iterator const & x)                           \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class Iterator>
struct stl_iterator
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
    stl_iterator(): ii() {};
// avoid ii = it.ii, see [ra11] above.
    stl_iterator(stl_iterator const & it) = default;
    stl_iterator & operator=(stl_iterator const & it) { i=it.i; ii.assign(it.ii); return *this; }
    template <class SS> stl_iterator(SS const & s_, typename Iterator::atom_type * p_)
        : ii(s_, p_),
          i(ra::ra_traits<shape_type>::make(dependent_frame_rank(s_.size(), ii.cellr_spec), 0))
    {
// [ra12] Null p_ so begin()==end() for iterator on empty range. Plying uses sizes so this doesn't matter.
        for (auto && dim: s_) {
            if (dim.size==0) {
                ii.c.p = nullptr;
                break;
            }
        }
    };

    template <class PP> bool operator==(PP const & j) const { return ii.c.p==j.ii.c.p; }
    template <class PP> bool operator!=(PP const & j) const { return ii.c.p!=j.ii.c.p; }
// taken from Iterator to handle the special case for cellr=0.
    decltype(auto) operator*() const { return ii.star(); }
    decltype(auto) operator*() { return ii.star(); }
    stl_iterator & operator++() { next_in_cube(ii.rank(), ii.dim, i, ii.c.p); return *this; }
};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_ITERATOR
