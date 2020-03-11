// -*- mode: c++; coding: utf-8 -*-
/// @file small.H
/// @brief Arrays with static dimensions. Compare with View, Container.

// (c) Daniel Llorens - 2013-2016, 2018-2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/expr.H"
#include "ra/mpdebug.H"

namespace ra {


// --------------------
// Develop indices for Small
// --------------------

template <class sizes, class strides>
struct Indexer0
{
    static_assert(mp::len<sizes> == mp::len<strides>, "mismatched sizes & strides");

    template <rank_t end, rank_t k, class P>
    constexpr static std::enable_if_t<k==end, dim_t> index_p_(dim_t const c, P const & p)
    {
        return c;
    }
    template <rank_t end, rank_t k, class P>
    constexpr static std::enable_if_t<(k<end), dim_t> index_p_(dim_t const c, P const & p)
    {
        RA_CHECK(inside(p[k], mp::first<sizes>::value));
        return Indexer0<mp::drop1<sizes>, mp::drop1<strides>>::template index_p_<end, k+1>(c + p[k] * mp::first<strides>::value, p);
    }
    template <class P>
    constexpr static dim_t index_p(P const & p) // for Container::at().
    {
// gcc accepts p.size(), but I also need P = std::array to work. See also below.
        static_assert(mp::len<sizes> >= size_s<P>(), "too many indices");
        return index_p_<size_s<P>(), 0>(0, p);
    }
    template <class P, std::enable_if_t<size_s<P>()!=RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        static_assert(mp::len<sizes> <= size_s<P>(), "too few indices");
        return index_p_<mp::len<sizes>, 0>(0, p);
    }
    template <class P, std::enable_if_t<size_s<P>()==RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        RA_CHECK(mp::len<sizes> <= p.size());
        return index_p_<mp::len<sizes>, 0>(0, p);
    }
};


// --------------------
// Small iterator
// --------------------
// TODO Refactor with cell_iterator / STLIterator for View?

// V is always SmallBase<SmallView, ...>
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

    using cell_sizes = mp::drop<typename V::sizes, framer>;
    using cell_strides = mp::drop<typename V::strides, framer>;
    using sizes = mp::take<typename V::sizes, framer>; // these are strides on atom_type * p !!
    using strides = mp::take<typename V::strides, framer>;

    using shape_type = std::array<dim_t, framer>;
    using atom_type = typename V::value_type; // FIXME fix ra_traits<SmallBase>
    using cell_type = SmallView<atom_type, cell_sizes, cell_strides>;
    using value_type = std::conditional_t<0==cellr, atom_type, cell_type>;
    using frame_type = SmallView<int, sizes, strides>; // only to compute ssizes

    cell_type c;

    constexpr cell_iterator_small(cell_iterator_small const & ci): c { ci.c.p } {}
// see STLIterator for the case of s_[0]=0, etc. [ra12].
    constexpr cell_iterator_small(atom_type * p_): c { p_ } {}

    constexpr static dim_t size(int k) { RA_CHECK(inside(k, rank())); return V::size(k); }
    constexpr static dim_t size_s(int k) { /* RA_CHECK(inside(k, rank_s())); */ return V::size(k); }
    constexpr static dim_t stride(int k) { return k<rank() ? V::stride(k) : 0; }
    constexpr static bool keep_stride(dim_t st, int z, int j)
    {
        return st*(z<rank() ? stride(z) : 0)==(j<rank() ? stride(j) : 0);
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
        RA_CHECK(rank()<=dim_t(i_.size()) && "too few indices");
        if constexpr (0==cellr) {
            return c.p[Indexer0<sizes, strides>::index_short(i_)];
        } else {
            return cell_type(c.p + Indexer0<sizes, strides>::index_short(i_));
        }
    }

    FOR_EACH(RA_DEF_ASSIGNOPS, =, *=, +=, -=, /=)
};


// --------------------
// STLIterator for both cell_iterator_small & cell_iterator
// FIXME make it work for any array iterator, as in ply_ravel, ply_index.
// --------------------

template <class S, class I, class P>
inline void next_in_cube(rank_t const framer, S const & dim, I & i, P & p)
{
    for (int k=framer-1; k>=0; --k) {
        ++i[k];
        if (i[k]<dim[k].size) {
            p += dim[k].stride;
            return;
        } else {
            i[k] = 0;
            p -= dim[k].stride*(dim[k].size-1);
        }
    }
    p = nullptr;
}

template <int k, class sizes, class strides, class I, class P> void
next_in_cube(I & i, P & p)
{
    if constexpr (k>=0) {
        ++i[k];
        if (i[k]<mp::ref<sizes, k>::value) {
            p += mp::ref<strides, k>::value;
        } else {
            i[k] = 0;
            p -= mp::ref<strides, k>::value*(mp::ref<sizes, k>::value-1);
            next_in_cube<k-1, sizes, strides>(i, p);
        }
    } else {
        p = nullptr;
    }
}

template <class Iterator>
struct STLIterator
{
    using value_type = typename Iterator::value_type;
    using difference_type = dim_t;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;
    using shape_type = typename Iterator::shape_type;

    Iterator ii;
    shape_type i;
    STLIterator(STLIterator const & it) = default;
    constexpr STLIterator & operator=(STLIterator const & it)
    {
        i = it.i;
        ii.Iterator::~Iterator(); // no-op except for View<RANK_ANY>. Still...
        new (&ii) Iterator(it.ii); // avoid ii = it.ii [ra11]
        return *this;
    }
// FIXME shape_type may be std::array or std::vector, so can't get rid of ra_traits::make yet
    STLIterator(Iterator const & ii_): ii(ii_), i(ra::ra_traits<shape_type>::make(ii.rank(), 0))
    {
// [ra12] Null p_ so begin()==end() for empty range. ply() uses sizes so this doesn't matter.
        if (ii.c.p!=nullptr && 0==ra::size(ii)) {
            ii.c.p = nullptr;
        }
    };

    template <class PP> bool operator==(PP const & j) const { return ii.c.p==j.ii.c.p; }
    template <class PP> bool operator!=(PP const & j) const { return ii.c.p!=j.ii.c.p; }

    decltype(auto) operator*() const { if constexpr (0==Iterator::cellr) return *ii.c.p; else return ii.c; }
    decltype(auto) operator*() { if constexpr (0==Iterator::cellr) return *ii.c.p; else return ii.c; }
    STLIterator & operator++()
    {
        if constexpr (0==Iterator::rank_s()) { // when rank==0, DIM_ANY check isn't enough :-/
            ii.c.p = nullptr;
        } else if constexpr (DIM_ANY != ra::size_s<Iterator>()) {
            next_in_cube<Iterator::rank()-1, typename Iterator::sizes, typename Iterator::strides>(i, ii.c.p);
        } else {
            next_in_cube(ii.rank(), ii.dim, i, ii.c.p);
        }
        return *this;
    }
};

template <class T> STLIterator<T> stl_iterator(T && t) { return STLIterator<T>(std::forward<T>(t)); }


// --------------------
// Base for both small view & container
// --------------------

template <class sizes_, class strides_, class ... I>
struct FilterDims
{
    using sizes = sizes_;
    using strides = strides_;
};
template <class sizes_, class strides_, class I0, class ... I>
struct FilterDims<sizes_, strides_, I0, I ...>
{
    constexpr static int s = is_beatable<I0>::skip;
    constexpr static int s_src = is_beatable<I0>::skip_src;
    using next = FilterDims<mp::drop<sizes_, s_src>, mp::drop<strides_, s_src>, I ...>;
    using sizes = mp::append<mp::take<sizes_, s>, typename next::sizes>;
    using strides = mp::append<mp::take<strides_, s>, typename next::strides>;
};

template <dim_t size0, dim_t stride0> inline
constexpr dim_t select(dim_t i0)
{
    RA_CHECK(inside(i0, size0));
    return i0*stride0;
};
template <dim_t size0, dim_t stride0, int n> inline
constexpr dim_t select(dots_t<n> i0)
{
    return 0;
}

template <class sizes, class strides> inline
constexpr dim_t select_loop()
{
    return 0;
}
template <class sizes, class strides, class I0, class ... I> inline
constexpr dim_t select_loop(I0 i0, I ... i)
{
    constexpr int s_src = is_beatable<I0>::skip_src;
    return select<mp::first<sizes>::value, mp::first<strides>::value>(i0)
        + select_loop<mp::drop<sizes, s_src>, mp::drop<strides, s_src>>(i ...);
}

template <class T> struct mat_uv   { T uu, uv, vu, vv; };
template <class T> struct mat_xy   { T xx, xy, yx, yy; };
template <class T> struct mat_uvz  { T uu, uv, uz, vu, vv, vz, zu, zv, zz; };
template <class T> struct mat_xyz  { T xx, xy, xz, yx, yy, yz, zx, zy, zz; };
template <class T> struct mat_xyzw { T xx, xy, xz, xw, yx, yy, yz, yw, zx, zy, zz, zw, wx, wy, wz, ww; };

template <class T> struct vec_uv   { T u, v; };
template <class T> struct vec_xy   { T x, y; };
template <class T> struct vec_tp   { T t, p; };
template <class T> struct vec_uvz  { T u, v, z; };
template <class T> struct vec_xyz  { T x, y, z; };
template <class T> struct vec_rtp  { T r, t, p; };
template <class T> struct vec_xyzw { T u, v, z, w; };

template <template <class, class, class> class Child_, class T, class sizes_, class strides_>
struct SmallBase
{
    using sizes = sizes_;
    using strides = strides_;
    using value_type = T;

    template <class TT> using BadDimension = mp::int_t<(TT::value<0 || TT::value==DIM_ANY || TT::value==DIM_BAD)>;
    static_assert(!mp::apply<mp::orb, mp::map<BadDimension, sizes>>::value, "negative dimensions");
    static_assert(mp::len<sizes> == mp::len<strides>, "bad strides"); // TODO full static check on strides.

    using Child = Child_<T, sizes, strides>;

    constexpr static rank_t rank()  { return mp::len<sizes>; }
    constexpr static rank_t rank_s()  { return mp::len<sizes>; }
    constexpr static dim_t size() { return mp::apply<mp::prod, sizes>::value; }
    constexpr static dim_t size_s() { return size(); }
    constexpr static auto ssizes = mp::tuple_values<std::array<dim_t, rank()>, sizes>();
    constexpr static auto sstrides = mp::tuple_values<std::array<dim_t, rank()>, strides>();
    constexpr static dim_t size(int k) { return ssizes[k]; }
    constexpr static dim_t size_s(int k) { return ssizes[k]; }
    constexpr static dim_t stride(int k) { return sstrides[k]; }

    constexpr T * data() { return static_cast<Child &>(*this).p; }
    constexpr T const * data() const { return static_cast<Child const &>(*this).p; }

// Specialize for rank() integer-args -> scalar, same in ra::View in big.H.
#define SUBSCRIPTS(CONST)                                               \
    template <class ... I, std::enable_if_t<((0 + ... + std::is_integral_v<I>)<rank() \
                                             && (is_beatable<I>::static_p && ...)), int> = 0> \
    constexpr auto operator()(I ... i) CONST                            \
    {                                                                   \
        using FD = FilterDims<sizes, strides, I ...>;                   \
        return SmallView<T CONST, typename FD::sizes, typename FD::strides> \
            (data()+select_loop<sizes, strides>(i ...));                \
    }                                                                   \
    template <class ... I, std::enable_if_t<(0 + ... + std::is_integral_v<I>)==rank(), int> = 0> \
    constexpr decltype(auto) operator()(I ... i) CONST                            \
    {                                                                   \
        return data()[select_loop<sizes, strides>(i ...)];              \
    } /* TODO More than one selector... */                              \
    template <class ... I, std::enable_if_t<(!is_beatable<I>::static_p || ...), int> = 0> \
    constexpr auto operator()(I && ... i) CONST                         \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
    /* BUG I must be fixed size, otherwise we can't make out the output type. */ \
    template <class I>                                                  \
    constexpr auto at(I const & i) CONST                                \
    {                                                                   \
        return SmallView<T CONST, mp::drop<sizes, ra::size_s<I>()>, mp::drop<strides, ra::size_s<I>()>> \
            (data()+Indexer0<sizes, strides>::index_p(i));              \
    }                                                                   \
    constexpr decltype(auto) operator[](dim_t const i) CONST            \
    {                                                                   \
        return (*this)(i);                                              \
    }
    FOR_EACH(SUBSCRIPTS, /*const*/, const)
#undef SUBSCRIPTS

// see same thing for View.
#define DEF_ASSIGNOPS(OP)                                               \
    template <class X, std::enable_if_t<!mp::is_tuple_v<std::decay_t<X>>, int> =0> \
    constexpr Child & operator OP(X && x)                               \
    {                                                                   \
        ra::start(static_cast<Child &>(*this)) OP x;                    \
        return static_cast<Child &>(*this);                             \
    }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS

    // braces don't match X &&
    constexpr Child & operator=(nested_arg<T, sizes> const & x)
    {
        ra::iter<-1>(static_cast<Child &>(*this)) = mp::from_tuple<std::array<typename nested_tuple<T, sizes>::sub, size(0)>>(x);
        return static_cast<Child &>(*this);
    }
    // braces row-major ravel for rank!=1
    constexpr Child & operator=(ravel_arg<T, sizes> const & x_)
    {
        auto x = mp::from_tuple<std::array<T, size()>>(x_);
        auto b = this->begin();
        for (auto const & xx: x) { *b = xx; ++b; } // std::copy will be constexpr in c++20
        return static_cast<Child &>(*this);
    }

// TODO would replace by s(ra::iota) if that could be made constexpr
#define DEF_AS(CONST)                                                   \
    template <int ss, int oo=0>                                         \
    constexpr auto as() CONST                                           \
    {                                                                   \
        static_assert(rank()>=1, "bad rank for as<>");                  \
        static_assert(ss>=0 && oo>=0 && ss+oo<=size(), "bad size for as<>"); \
        return SmallView<T CONST, mp::cons<mp::int_t<ss>, mp::drop1<sizes>>, strides>(this->data()+oo*this->stride(0)); \
    }
    FOR_EACH(DEF_AS, /* const */, const)
#undef DEF_AS

    template <rank_t c=0> using iterator = ra::cell_iterator_small<SmallBase<SmallView, T, sizes, strides>, c>;
    template <rank_t c=0> using const_iterator = ra::cell_iterator_small<SmallBase<SmallView, T const, sizes, strides>, c>;
    template <rank_t c=0> constexpr iterator<c> iter() { return data(); }
    template <rank_t c=0> constexpr const_iterator<c> iter() const { return data(); }

// FIXME see if we need to extend this for cellr!=0.
// template <class P> using STLIterator = std::conditional_t<have_default_strides, P, STLIterator<Iterator<P>>>;
    constexpr static bool have_default_strides = std::is_same_v<strides, default_strides<sizes>>;
    template <class I, class P> using pick_STLIterator = std::conditional_t<have_default_strides, P, ra::STLIterator<I>>;
    using STLIterator = pick_STLIterator<iterator<0>, T *>;
    using STLConstIterator = pick_STLIterator<const_iterator<0>, T const *>;

// TODO In C++17 begin() end() may be different types, at least for ranged for
// (https://en.cppreference.com/w/cpp/language/range-for).
// See if we can use this to simplify end() and !=end() test.
// TODO With default strides I can just return p. Make sure to test before changing this.
    constexpr STLIterator begin() { if constexpr (have_default_strides) return data(); else return iter(); }
    constexpr STLConstIterator begin() const { if constexpr (have_default_strides) return data(); else return iter(); }
    constexpr STLIterator end() { if constexpr (have_default_strides) return data()+size(); else return iterator<0>(nullptr); }
    constexpr STLConstIterator end() const { if constexpr (have_default_strides) return data()+size(); else return const_iterator<0>(nullptr); }

// allowing rank 1 for coord types
    constexpr static bool convertible_to_scalar = size()==1; // rank()==0 || (rank()==1 && size()==1);

// Renames.
#define COMP_RENAME_C(name__, req_dim0__, req_dim1__, CONST)            \
    operator name__<T> CONST &() CONST                                  \
    {                                                                   \
        static_assert(std::is_same_v<strides, default_strides<mp::int_list<req_dim0__, req_dim1__>>>, \
                      "renames only on default strides");               \
        static_assert(size(0)==req_dim0__ && size(1)==req_dim1__, "dimension error"); \
        return reinterpret_cast<name__<T> CONST &>(*this);              \
    }
#define COMP_RENAME(name__, dim0__, dim1__)             \
    COMP_RENAME_C(name__, dim0__, dim1__, /* const */)  \
    COMP_RENAME_C(name__, dim0__, dim1__, const)
    COMP_RENAME(mat_xy,   2, 2)
    COMP_RENAME(mat_uv,   2, 2)
    COMP_RENAME(mat_xyz,  3, 3)
    COMP_RENAME(mat_uvz,  3, 3)
    COMP_RENAME(mat_xyzw, 4, 4)
#undef COMP_RENAME
#undef COMP_RENAME_C
#define COMP_RENAME_C(name__, dim0__, CONST)                            \
    operator name__<T> CONST &() CONST                                  \
    {                                                                   \
        static_assert(std::is_same_v<strides, default_strides<mp::int_list<dim0__>>>, \
                      "renames only on default strides");               \
        static_assert(size(0)==dim0__, "dimension error");              \
        return reinterpret_cast<name__<T> CONST &>(*this);              \
    }
#define COMP_RENAME(name__, dim0__)             \
    COMP_RENAME_C(name__, dim0__, /* const */)  \
    COMP_RENAME_C(name__, dim0__, const)
    COMP_RENAME(vec_xy,   2)
    COMP_RENAME(vec_uv,   2)
    COMP_RENAME(vec_tp,   2)
    COMP_RENAME(vec_xyz,  3)
    COMP_RENAME(vec_uvz,  3)
    COMP_RENAME(vec_rtp,  3)
    COMP_RENAME(vec_xyzw, 4)
#undef COMP_RENAME
#undef COMP_RENAME_C
};


// ---------------------
// Small view & container
// ---------------------
// Strides are compile time, so we can put most members in the view type.

template <class T, class sizes, class strides>
struct SmallView: public SmallBase<SmallView, T, sizes, strides>
{
    using Base = SmallBase<SmallView, T, sizes, strides>;
    using Base::rank, Base::size, Base::operator=;

    T * p;
    constexpr SmallView(T * p_): p(p_) {}
    constexpr SmallView(SmallView const & s): p(s.p) {}

    constexpr operator T & () { static_assert(Base::convertible_to_scalar); return p[0]; }
    constexpr operator T const & () const { static_assert(Base::convertible_to_scalar); return p[0]; };
};

template <class T, class sizes, class strides, class ... nested_args, class ... ravel_args>
struct SmallArray<T, sizes, strides, std::tuple<nested_args ...>, std::tuple<ravel_args ...>>
    : public SmallBase<SmallArray, T, sizes, strides>
{
    using Base = SmallBase<SmallArray, T, sizes, strides>;
    using Base::rank, Base::size;

    T p[Base::size()];

    constexpr SmallArray(): p() {}
    // braces don't match (X &&)
    constexpr SmallArray(nested_args const & ... x): p()
    {
        static_cast<Base &>(*this) = nested_arg<T, sizes> { x ... };
    }
    // braces row-major ravel for rank!=1
    constexpr SmallArray(ravel_args const & ... x): p()
    {
        static_cast<Base &>(*this) = ravel_arg<T, sizes> { x ... };
    }
    constexpr SmallArray(T const & t): p() // needed if T isn't registered as scalar [ra44]
    {
        for (auto & x: p) { x = t; } // std::fill will be constexpr in c++20
    }
    // X && x makes this a better match than nested_args ... for 1 argument.
    template <class X, std::enable_if_t<!std::is_same_v<T, std::decay_t<X>>
                                        && !mp::is_tuple_v<std::decay_t<X>>, int> =0>
    constexpr SmallArray(X && x): p()
    {
        static_cast<Base &>(*this) = x;
    }

    constexpr operator SmallView<T, sizes, strides> () { return SmallView<T, sizes, strides>(p); }
    constexpr operator SmallView<T const, sizes, strides> const () { return SmallView<T const, sizes, strides>(p); }

// BUG these make SmallArray<T, N> std::is_convertible to T even though conversion isn't possible b/c of the assert.
#define DEF_SCALARS(CONST)                                              \
    constexpr operator T CONST & () CONST { static_assert(Base::convertible_to_scalar); return p[0]; } \
    T CONST & back() CONST                                              \
    {                                                                   \
        static_assert(Base::rank()==1, "bad rank for back");            \
        static_assert(Base::size()>0, "bad size for back");             \
        return (*this)[Base::size()-1];                                 \
    }
    FOR_EACH(DEF_SCALARS, /* const */, const)
#undef DEF_SCALARS
};

// FIXME unfortunately necessary. Try to remove the need, also of (S, begin, end) in Container, once the nested_tuple constructors work.
template <class A, class I, class J>
A ravel_from_iterators(I && begin, J && end)
{
    A a;
    std::copy(std::forward<I>(begin), std::forward<J>(end), a.begin());
    return a;
}

// FIXME Type_ seems superfluous
template <template <class, class, class> class Type_,
          class T, class sizes, class strides>
struct ra_traits_small
{
    using V = Type_<T, sizes, strides>;
    using value_type = T;
    // constexpr static auto const & shape(V const & v) { return V::ssizes; }
    constexpr static auto shape(V const & v) { return SmallView<ra::dim_t const, mp::int_list<V::rank_s()>, mp::int_list<1>>(V::ssizes.data()); }
#define MAKE_COND (std::is_same_v<strides, mp::int_list<1>> && std::is_same_v<sizes, mp::take<sizes, 1>>)
    constexpr static V make(dim_t const n)
    {
        static_assert(MAKE_COND, "bad type for ra_traits::make");
        RA_CHECK(n==V::size(0));
        return V {};
    }
    template <class TT> static V make(dim_t n, TT const & t)
    {
        static_assert(MAKE_COND, "bad type for ra_traits::make");
        RA_CHECK(n==V::size(0));
        return V(t);
    }
#undef MAKE_COND
    constexpr static dim_t size(V const & v) { return v.size(); }
    constexpr static rank_t rank(V const & v) { return V::rank(); }
    constexpr static dim_t size_s() { return V::size(); }
    constexpr static rank_t rank_s() { return V::rank(); };
};

template <class T, class sizes, class strides>
struct ra_traits_def<SmallArray<T, sizes, strides>>: public ra_traits_small<SmallArray, T, sizes, strides> {};

template <class T, class sizes, class strides>
struct ra_traits_def<SmallView<T, sizes, strides>>: public ra_traits_small<SmallView, T, sizes, strides> {};


// ---------------------
// Support for builtin arrays
// ---------------------

template <class T, class I=mp::iota<std::rank_v<T>>>
struct builtin_array_sizes;

template <class T, int ... I>
struct builtin_array_sizes<T, mp::int_list<I ...>>
{
    using type = mp::int_list<std::extent_v<T, I> ...>;
};

template <class T> using builtin_array_sizes_t = typename builtin_array_sizes<T>::type;

template <class T>
struct builtin_array_types
{
// do preserve const.
    using A = std::remove_volatile_t<std::remove_reference_t<T>>;
    using E = std::remove_all_extents_t<A>;
    using sizes = builtin_array_sizes_t<A>;
    using view = SmallView<E, sizes>;
};

// forward declared in type.H.
template <class T, std::enable_if_t<is_builtin_array<T>, int>>
inline constexpr auto start(T && t)
{
    using Z = builtin_array_types<T>;
    return typename Z::view((typename Z::E *)(t)).iter();
}

template <class T>
struct ra_traits_def<T, std::enable_if_t<is_builtin_array<T>>>
{
    using S = typename builtin_array_types<T>::view;
// FIXME messy start()
    constexpr static decltype(auto) shape(T const & t) { return ra::shape(start(t)); }
    constexpr static dim_t size(T const & t) { return ra::size_s<S>(); }
    constexpr static dim_t size_s() { return ra::size_s<S>(); }
    constexpr static rank_t rank(T const & t) { return S::rank(); }
    constexpr static rank_t rank_s() { return S::rank_s(); }
};


// --------------------
// Small ops; cf view-ops.H.
// FIXME maybe there, or separate file.
// TODO See if this can be merged with Reframe (e.g. beat(reframe(a)) -> transpose(a) ?)
// --------------------

template <class A, class i>
struct axis_indices
{
    template <class T> using match_index = mp::int_t<(T::value==i::value)>;
    using I = mp::iota<mp::len<A>>;
    using type = mp::Filter_<mp::map<match_index, A>, I>;
// don't enforce, so allow dead axes (e.g. in transpose<1>(rank 1 array)).
    // static_assert((mp::len<type>)>0, "dst axis doesn't appear in transposed axes list");
};

template <class axes_list, class src_sizes, class src_strides>
struct axes_list_indices
{
    static_assert(mp::len<axes_list> == mp::len<src_sizes>, "bad size for transposed axes list");
    constexpr static int talmax = mp::fold<mp::max, void, axes_list>::value;
    constexpr static int talmin = mp::fold<mp::min, void, axes_list>::value;
    static_assert(talmin >= 0, "bad index in transposed axes list");
// don't enforce, so allow dead axes (e.g. in transpose<1>(rank 1 array)).
    // static_assert(talmax < mp::len<src_sizes>, "bad index in transposed axes list");

    template <class dst_i> struct dst_indices_
    {
        using type = typename axis_indices<axes_list, dst_i>::type;
        template <class i> using sizesi = mp::ref<src_sizes, i::value>;
        template <class i> using stridesi = mp::ref<src_strides, i::value>;
        using stride = mp::fold<mp::sum, void, mp::map<stridesi, type>>;
        using size = mp::fold<mp::min, void, mp::map<sizesi, type>>;
    };

    template <class dst_i> using dst_indices = typename dst_indices_<dst_i>::type;
    template <class dst_i> using dst_size = typename dst_indices_<dst_i>::size;
    template <class dst_i> using dst_stride = typename dst_indices_<dst_i>::stride;

    using dst = mp::iota<(talmax>=0 ? (1+talmax) : 0)>;
    using type = mp::map<dst_indices, dst>;
    using sizes =  mp::map<dst_size, dst>;
    using strides =  mp::map<dst_stride, dst>;
};

#define DEF_TRANSPOSE(CONST)                                            \
    template <int ... Iarg, template <class, class, class> class Child, class T, class sizes, class strides> \
    inline auto transpose(SmallBase<Child, T, sizes, strides> CONST & a) \
    {                                                                   \
        using ti = axes_list_indices<mp::int_list<Iarg ...>, sizes, strides>; \
        return SmallView<T CONST, typename ti::sizes, typename ti::strides>(a.data()); \
    };                                                                  \
                                                                        \
    template <template <class, class, class> class Child, class T, class sizes, class strides> \
    inline auto diag(SmallBase<Child, T, sizes, strides> CONST & a)     \
    {                                                                   \
        return transpose<0, 0>(a);                                      \
    }
FOR_EACH(DEF_TRANSPOSE, /* const */, const)
#undef DEF_TRANSPOSE

// TODO Used by ProductRule; waiting for proper generalization.
template <template <class, class, class> class Child1, class T1, class sizes1, class strides1,
          template <class, class, class> class Child2, class T2, class sizes2, class strides2>
auto cat(SmallBase<Child1, T1, sizes1, strides1> const & a1, SmallBase<Child2, T2, sizes2, strides2> const & a2)
{
    using A1 = SmallBase<Child1, T1, sizes1, strides1>;
    using A2 = SmallBase<Child2, T2, sizes2, strides2>;
    static_assert(A1::rank()==1 && A2::rank()==1, "bad ranks for cat"); // gcc accepts a1.rank(), etc.
    using T = std::decay_t<decltype(a1[0])>;
    Small<T, A1::size()+A2::size()> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    std::copy(a2.begin(), a2.end(), val.begin()+a1.size());
    return val;
}

template <template <class, class, class> class Child1, class T1, class sizes1, class strides1,
          class A2,
          std::enable_if_t<is_scalar<A2>, int> =0>
auto cat(SmallBase<Child1, T1, sizes1, strides1> const & a1, A2 const & a2)
{
    using A1 = SmallBase<Child1, T1, sizes1, strides1>;
    static_assert(A1::rank()==1, "bad ranks for cat");
    using T = std::decay_t<decltype(a1[0])>;
    Small<T, A1::size()+1> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    val[a1.size()] = a2;
    return val;
}

template <class A1,
          template <class, class, class> class Child2, class T2, class sizes2, class strides2,
          std::enable_if_t<is_scalar<A1>, int> =0>
auto cat(A1 const & a1, SmallBase<Child2, T2, sizes2, strides2> const & a2)
{
    using A2 = SmallBase<Child2, T2, sizes2, strides2>;
    static_assert(A2::rank()==1, "bad ranks for cat");
    using T = std::decay_t<decltype(a2[0])>;
    Small<T, 1+A2::size()> val;
    val[0] = a1;
    std::copy(a2.begin(), a2.end(), val.begin()+1);
    return val;
}

// FIXME should be local (constexpr lambda + mp::apply?)
template <int s> struct explode_divop
{
    template <class T> struct op_
    {
        static_assert((T::value/s)*s==T::value);
        using type = mp::int_t<T::value / s>;
    };
    template <class T> using op = typename op_<T>::type;
};

// See view-ops.H:explode, collapse. FIXME support real->complex, etc.
template <class super_t,
          template <class, class, class> class Child, class T, class sizes, class strides>
auto explode(SmallBase<Child, T, sizes, strides> & a)
{
    using ta = SmallBase<Child, T, sizes, strides>;
// the returned type has strides in super_t, but to support general strides we'd need strides in T. Maybe FIXME?
    static_assert(super_t::have_default_strides);
    constexpr rank_t ra = ta::rank_s();
    constexpr rank_t rb = super_t::rank_s();
    static_assert(std::is_same_v<mp::drop<sizes, ra-rb>, typename super_t::sizes>);
    static_assert(std::is_same_v<mp::drop<strides, ra-rb>, typename super_t::strides>);
    using cstrides = mp::map<explode_divop<ra::size_s<super_t>()>::template op, mp::take<strides, ra-rb>>;
    return SmallView<super_t, mp::take<sizes, ra-rb>, cstrides>((super_t *) a.data());
}

} // namespace ra
