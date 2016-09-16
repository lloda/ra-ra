
// (c) Daniel Llorens - 2013-2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_ITERATOR_H
#define RA_ITERATOR_H

/// @file iterator.H
/// @brief Row-major iterator, supports both STL and ply interfaces.

#include "ra/traits.H"
#include "ra/opcheck.H"
#include <iostream>

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_ITERATOR RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_ITERATOR
        #define RA_CHECK_BOUNDS_RA_ITERATOR 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_ITERATOR==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

template <class I> struct Print_iterator { I i; };

template <class I>
Print_iterator<I> print_iterator(I && i) { return Print_iterator<I> { std::forward<I>(i) }; }

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

// @TODO Allow infinite rank; need a special value of crank for that.
constexpr rank_t dependent_cell_rank(rank_t rank, rank_t crank)
{
    return crank>=0 ? crank // not dependent
         : rank==RANK_ANY ? RANK_ANY // defer
         : rank+crank;
}

constexpr rank_t dependent_frame_rank(rank_t rank, rank_t crank)
{
    return rank==RANK_ANY ? RANK_ANY // defer
        : crank>=0 ? rank-crank // not dependent
        : -crank;
}

template <class V, rank_t cellr_, class Child> // Raw or Small.
struct cell_iterator
{
    constexpr static rank_t cellr_spec = cellr_;
    static_assert(cellr_spec!=RANK_ANY && cellr_spec!=RANK_BAD, "bad cell rank");
    constexpr static rank_t fullr = ra_traits<V>::rank_s();
    constexpr static rank_t cellr = dependent_cell_rank(fullr, cellr_spec);
    constexpr static rank_t framer = dependent_frame_rank(fullr, cellr_spec);
    static_assert(cellr>=0 || cellr==RANK_ANY, "bad cell rank");
    static_assert(framer>=0 || framer==RANK_ANY, "bad frame rank");
    static_assert(fullr==cellr || gt_rank(fullr, cellr), "bad cell rank");

    constexpr static rank_t rank_s() { return framer; }

    using shape_type = mp::If_<rank_s()==DIM_ANY, typename ra_traits<V>::shape_type, Small<dim_t, rank_s()>>;
    using dims_p = decltype(std::declval<V const>().dim); // @TODO can this be & ?
    using atom_type = typename ra_traits<V>::value_type;
    using cell_type = Raw<atom_type, cellr>;
    using value_type = mp::If_<cellr==0, atom_type, cell_type>;

    dims_p dim; // @TODO Either have an equivalent object for Small, or take V & and ask through that.
    cell_type c;

    constexpr rank_t rank() const { return dependent_frame_rank(rank_t(dim.size()), cellr_spec); }

    cell_iterator() { c.p = nullptr; } // end marker only.
    cell_iterator(cell_iterator const & ci): dim(ci.dim), c { ci.c.dim, ci.c.p } {}
    cell_iterator(cell_iterator && ci): dim(ci.dim), c { ci.c.dim, ci.c.p } {}
// needed because Raw = Raw copies contents, not the Raw itself ([ra11] in test-ra-0.C).
    cell_iterator & operator=(cell_iterator const & ci) { dim = ci.dim; c.dim = ci.c.dim; c.p = ci.c.p; return *this; }
    cell_iterator & operator=(cell_iterator && ci) { dim = ci.dim; c.dim = ci.c.dim; c.p = ci.c.p; return *this; }

// s_ is array's full shape; split it into dim/i (frame) and c (cell).
    template <class SS> cell_iterator(SS const & s_, atom_type * p_): dim(s_)
    {
        rank_t const rank = this->rank();
        for (int k=0; ; ++k) {
            if (k==rank) {
                c.p = p_;
                break;
            }
// so begin()==end() for un-plied iterator on empty range. For STL-like use; ra::Iterator doesn't check loop end in this way.
            if (s_[k].size==0) {
                c.p = nullptr;
                break;
            }
        }
        rank_t const cellr = dependent_cell_rank(rank_t(dim.size()), cellr_spec);
        resize(c.dim, cellr);
        for (int k=0; k<cellr; ++k) {
            c.dim[k] = s_[k+rank];
        }
    }

    constexpr static dim_t fullsize_s() { return ra_traits<V>::size_s(); }
    constexpr static dim_t size_s()
    {
        return fullsize_s()==DIM_ANY ? DIM_ANY : fullsize_s() / ra_traits<cell_type>::size_s();
    }
    // @TODO Refactor w/ ra_traits<Raw>::shape.
    shape_type shape() const
    {
        rank_t const iend = rank();
        shape_type s(ra_traits<shape_type>::make(iend));
        for (rank_t i=0; i!=iend; ++i) {
            s[i] = dim[i].size;
        }
        return s;
    }

    constexpr dim_t size(int const j) const { CHECK_BOUNDS(inside(j, rank())); return dim[j].size; }
    constexpr dim_t stride(int const j) const { return j<rank() ? dim[j].stride : 0; }
    constexpr bool compact_stride(dim_t const step, int const z, int const j) const
    {
        dim_t sz = z<rank() ? dim[z].stride : 0;
        dim_t sj = j<rank() ? dim[j].stride : 0;
        return step*sz==sj;
    }
    void adv(rank_t const k, dim_t const d)
    {
        c.p += (k<rank()) * dim[k].stride*d;
    }
};

template <class V, rank_t cellr=0>
struct ra_iterator: public cell_iterator<V, cellr, ra_iterator<V, cellr>>
{
    using P = cell_iterator<V, cellr, ra_iterator<V, cellr>>;

    auto flat() const { return cellp<typename P::cell_type> { P::c }; }
// Return type to allow either Raw & or Raw const & verb. Can't set self b/c original p isn't kept. @TODO Think this over.
    template <class I>
    typename P::value_type at(I const & i_)
    {
        CHECK_BOUNDS(P::rank()<=dim_t(i_.size()) && "too few indices");
        return typename P::cell_type { P::c.dim, P::c.p+Indexer1::index_short(P::rank(), P::dim, i_) };
    }
    decltype(auto) operator_star() const { return P::c; }
    decltype(auto) operator_star() { return P::c; }

    ra_iterator(): P() {};
    template <class SS> ra_iterator(SS const & s_, typename P::atom_type * p_): P(s_, p_) {};

#define DEF_EQOPS(OP)                                                   \
    template <class X> void operator OP(X && x)                         \
    {                                                                   \
        ply_either(expr([](auto && y, auto && x) { y OP x; }, *this, start(x))); \
    }
    FOR_EACH(DEF_EQOPS, =, *=, +=, -=, /=)
#undef DEF_EQOPS
};

template <class V>
struct ra_iterator<V, 0>: public cell_iterator<V, 0, ra_iterator<V, 0>>
{
    using P = cell_iterator<V, 0, ra_iterator<V, 0>>;

    auto flat() const { return P::c.p; }
    template <class I>
    decltype(auto) at(I const & i_)
    {
        CHECK_BOUNDS(P::rank()<=rank_t(i_.size()) && "too few indices");
        return P::c.p[Indexer1::index_short(P::rank(), P::dim, i_)];
    }
    decltype(auto) operator_star() const { return *P::c.p; }
    decltype(auto) operator_star() { return *P::c.p; }

    ra_iterator(): P() {};
    template <class SS> ra_iterator(SS const & s_, typename P::atom_type * p_): P(s_, p_) {};
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
    using shape_type = typename Iterator::P::shape_type;

    Iterator ii;
    shape_type i;
    stl_iterator(): ii() {};
    template <class SS> stl_iterator(SS const & s_, typename Iterator::P::atom_type * p_)
        : ii(s_, p_), i(ra::ra_traits<shape_type>::make(dependent_frame_rank(s_.size(), ii.cellr_spec), 0)) {};

    template <class PP> bool operator==(PP const & j) const { return ii.c.p==j.ii.c.p; }
    template <class PP> bool operator!=(PP const & j) const { return ii.c.p!=j.ii.c.p; }
// taken from Iterator to handle the special case for cellr=0.
    decltype(auto) operator*() const { return ii.operator_star(); }
    decltype(auto) operator*() { return ii.operator_star(); }
    stl_iterator & operator++()
    {
        next_in_cube(ii.rank(), ii.dim, i, ii.c.p);
        return *this;
    }
};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_ITERATOR

#endif // RA_ITERATOR_H
