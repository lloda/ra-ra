
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/ply.H"
#include <iterator>

/// @file small.H
/// @brief Arrays with fixed size.

#ifndef RA_CHECK_BOUNDS_SMALL
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_SMALL 1
  #else
    #define RA_CHECK_BOUNDS_SMALL RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_SMALL==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

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
        CHECK_BOUNDS(inside(p[k], mp::First_<sizes>::value));
        return Indexer0<mp::Drop1_<sizes>, mp::Drop1_<strides>>::template index_p_<end, k+1>(c + p[k] * mp::First_<strides>::value, p);
    }
    template <class P>
    constexpr static dim_t index_p(P const & p) // for Container::at().
    {
// gcc accepts p.size(), but I also need P = std::array to work. See also below.
        static_assert(mp::len<sizes> >= ra_traits<P>::size_s(), "too many indices");
        return index_p_<ra_traits<P>::size_s(), 0>(0, p);
    }
    template <class P, std::enable_if_t<ra_traits<P>::size_s()!=RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        static_assert(mp::len<sizes> <= ra_traits<P>::size_s(), "too few indices");
        return index_p_<mp::len<sizes>, 0>(0, p);
    }
    template <class P, std::enable_if_t<ra_traits<P>::size_s()==RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        CHECK_BOUNDS(mp::len<sizes> <= p.size());
        return index_p_<mp::len<sizes>, 0>(0, p);
    }
};

struct Indexer1
{
    template <class Dim, class P>
    constexpr static dim_t index_p(Dim const & dim, P const & p)
    {
        CHECK_BOUNDS(dim_t(dim.size())>=start(p).size(0) && "too many indices");
// use dim.data() to skip the size check.
        dim_t c = 0;
        for_each([&c](auto && d, auto && p) { CHECK_BOUNDS(inside(p, d.size)); c += d.stride*p; },
                 ptr(dim.data()), p);
        return c;
    }

// used by ra_iterator::at() for rank matching on rank<driving rank, no slicing. todo Static check.
    template <class Dim, class P>
    constexpr static dim_t index_short(rank_t framer, Dim const & dim, P const & p)
    {
        dim_t c = 0;
        for (rank_t k=0; k<framer; ++k) {
            CHECK_BOUNDS(inside(p[k], dim[k].size));
            c += dim[k].stride * p[k];
        }
        return c;
    }
};

// Static dimensions, stack storage. See View, Unique, Shared.
template <class T, class sizes, class strides> struct SmallArray; // Used as type of Small::iterator::shape().
template <class T, class sizes, class strides> struct SmallView; // Used as types of Small::iterator::at().

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
    using next = FilterDims<mp::Drop_<sizes_, s_src>, mp::Drop_<strides_, s_src>, I ...>;
    using sizes = mp::Append_<mp::Take_<sizes_, s>, typename next::sizes>;
    using strides = mp::Append_<mp::Take_<strides_, s>, typename next::strides>;
};

template <dim_t size0, dim_t stride0> inline
constexpr dim_t select(dim_t i0)
{
    CHECK_BOUNDS(inside(i0, size0));
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
    return select<mp::First_<sizes>::value, mp::First_<strides>::value>(i0)
        + select_loop<mp::Drop_<sizes, s_src>, mp::Drop_<strides, s_src>>(i ...);
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

// TODO Should this be itself an xpr type, or do I need a ra_iterator-like obj?
template <template <class, class, class> class Child_, class T, class sizes_, class strides_>
struct SmallBase
{
    using sizes = sizes_;
    using strides = strides_;

    constexpr static std::array<dim_t, mp::len<sizes_>> ssizes = mp::tuple_copy<sizes, std::array<dim_t, mp::len<sizes>> >::f();
    constexpr static std::array<dim_t, mp::len<strides_>> sstrides = mp::tuple_copy<strides, std::array<dim_t, mp::len<strides>> >::f();

// TODO complete static check on strides.
    template <class TTT> struct BadDimension
    {
        using type = mp::int_t<(TTT::value<0 || TTT::value==DIM_ANY || TTT::value==DIM_BAD)>;
    };
    static_assert(mp::len<sizes> == mp::len<strides>, "bad strides");
    static_assert(!mp::Apply_<mp::Or, mp::Map_<BadDimension, sizes>>::value, "negative dimensions");

// TODO look at Dim below; useable in the same way?
    using Child = Child_<T, sizes, strides>;

    constexpr static rank_t rank()  { return mp::len<sizes>; }
    constexpr static rank_t rank_s()  { return mp::len<sizes>; }
    constexpr static dim_t size() { return mp::Apply_<mp::Prod, sizes>::value; }
    constexpr static dim_t size_s() { return size(); }
    constexpr static dim_t size(int j) { return ssizes[j]; }
    constexpr static dim_t stride(int j) { return sstrides[j]; }

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
    } /* TODO More than one selector... */                             \
    template <class ... I, std::enable_if_t<(!is_beatable<I>::static_p || ...), int> = 0> \
    constexpr auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
 /* BUG I must be fixed size, otherwise we can't make out the output type. */ \
    template <class I>                                                  \
    constexpr auto at(I const & i) CONST                                \
    {                                                                   \
        return SmallView<T CONST, mp::Drop_<sizes, ra_traits<I>::size_s()>, mp::Drop_<strides, ra_traits<I>::size_s()>> \
            (data()+Indexer0<sizes, strides>::index_p(i));              \
    }                                                                   \
    constexpr decltype(auto) operator[](dim_t const i) CONST            \
    {                                                                   \
        return (*this)(i);                                              \
    }
    SUBSCRIPTS(const)
    SUBSCRIPTS(/*const*/)
#undef SUBSCRIPTS

// See same thing for View.
#define DEF_ASSIGNOPS(OP)                                               \
        template <class X> constexpr Child & operator OP(X && x)        \
    {                                                                   \
        /* TODO generally forbid writing to smaller rank from larger rank (small.H [ref01]) */ \
        constexpr rank_t xrank = std::decay_t<decltype(start(x))>::rank_s(); \
        static_assert(xrank<=rank() || xrank==RANK_ANY, "bad assigment"); \
        for_each([](T & y, T const & x) { y OP x; }, static_cast<Child &>(*this), x); \
        return static_cast<Child &>(*this);                             \
    }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
    /* Array type is not deduced by (X &&). FIXME ra:: loop, need iter<1>(Small). */
    constexpr Child & operator=(typename nested_braces<T, rank()>::list const x)
    {
        assert(size(0)==dim_t(x.size()));
        int i = 0;
        for (auto const & xx: x) { static_cast<Child &>(*this)(i) = xx; ++i; } /* std::copy not constexpr */
        return static_cast<Child &>(*this);
    }
    /* Fill row-major ravel for any rank. FIXME Use ra:: traversal need start(iterator). */
    constexpr Child & operator=(std::initializer_list<T> const x)
    {
        assert(size()==dim_t(x.size()));
        auto b = this->begin();
        for (auto const & xx: x) { *b = xx; ++b; } /* std::copy not constexpr */
        return static_cast<Child &>(*this);
    }


// TODO Would replace by s(ra::iota) if that could be made constexpr
#define DEF_AS(CONST)                                                   \
    template <int ss, int oo=0>                                         \
    constexpr auto as() CONST                                           \
    {                                                                   \
        static_assert(rank()>=1, "bad rank for as<>");                  \
        static_assert(ss>=0 && oo>=0 && ss+oo<=size(), "bad size for as<>"); \
        return SmallView<T CONST, mp::Cons_<mp::int_t<ss>, mp::Drop1_<sizes>>, strides>(this->data()+oo*this->stride(0)); \
    }
    DEF_AS(const)
    DEF_AS(/* const */)
#undef DEF_AS

// TODO This is rank 0 only. Maybe reuse cell_iterator?
    template <class P>
    struct Iterator
    {
        P p;

        using value_type = typename std::iterator_traits<P>::value_type;

        constexpr Iterator(P p_): p(p_) {}
        constexpr Iterator(Iterator const & q): p(q.p) {}
        constexpr Iterator() = delete;

// TODO See ra_traits<Small>. Why do these twice?
        constexpr static auto const & shape() { return ssizes; }
        constexpr static dim_t size(int j) { return SmallBase::size(j); }
        constexpr static dim_t size_s(int j) { return SmallBase::size(j); }
// For xpr use; cf this stride() against SmallBase::stride().
        constexpr static dim_t stride(int j) { return j<rank() ? SmallBase::stride(j) : 0; }
        constexpr bool keep_stride(dim_t step, int z, int j) const
        {
            return step*(z<rank() ? stride(z) : 0)==(j<rank() ? stride(j) : 0);
        }
        constexpr static auto size() { return SmallBase::size(); }
        constexpr static auto rank() { return SmallBase::rank(); }
        constexpr static auto size_s() { return size(); }
        constexpr static auto rank_s() { return rank(); }

        template <class I> constexpr decltype(auto) at(I const & i_)
        {
            return p[Indexer0<sizes, strides>::index_short(i_)];
        }
        constexpr void adv(rank_t const k, dim_t const d)
        {
            p += (k<rank()) * stride(k)*d;
        }
        constexpr auto flat() { return p; }
        constexpr auto flat() const { return p; }

#define DEF_ASSIGNOPS(OP)                                               \
        template <class X> void operator OP(X && x)                     \
        { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); } \
        template <class X> void operator OP(Iterator const & x)         \
        { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
        FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
    };
#undef DEF_ASSIGNOPS

    template <rank_t c=0> constexpr auto iter() { static_assert(c==0, "not yet"); return Iterator<T *>(data()); }
    template <rank_t c=0> constexpr auto iter() const { static_assert(c==0, "not yet"); return Iterator<T const *>(data()); }

    template <class Iterator>
    struct general_stl_iterator
    {
        using value_type = typename Iterator::value_type;
        using difference_type = dim_t;
        using pointer = value_type *;
        using reference = value_type &;
        using const_reference = value_type const &;
        using iterator_category = std::forward_iterator_tag;

        Iterator ii;
        std::decay_t<decltype(ssizes)> i;
        general_stl_iterator(pointer const & p): ii(std::tuple_size_v<decltype(i)> ==0 ? nullptr : p) { std::fill(i.begin(), i.end(), 0.); }
        general_stl_iterator(): ii(pointer(nullptr)) {}

        template <class PP> bool operator==(PP const & q) const { return ii.p==q.ii.p; }
        template <class PP> bool operator!=(PP const & q) const { return ii.p!=q.ii.p; }
        decltype(auto) operator*() const { return *(ii.p); }
        decltype(auto) operator*() { return *(ii.p); }
        template <int k=rank()-1>
        std::enable_if_t<(k>=0)> next_in_cube()
        {
            ++i[k];
            if (i[k]<mp::Ref_<sizes, k>::value) {
                ii.p += mp::Ref_<strides, k>::value;
            } else {
                i[k] = 0;
                ii.p -= mp::Ref_<strides, k>::value*(mp::Ref_<sizes, k>::value-1);
                next_in_cube<k-1>();
                return;
            }
        }
        template <int k> std::enable_if_t<(k<0)> next_in_cube() { ii.p = nullptr; }
        general_stl_iterator & operator++() { next_in_cube(); return *this; }
    };

    constexpr static bool have_default_strides = std::is_same_v<strides, default_strides_<sizes>>;
    template <class P> using stl_iterator = mp::If_<have_default_strides, P, general_stl_iterator<Iterator<P>>>;
// TODO In C++17 begin() end() may be different types. See if we can use this to simplify end() and !=end() test.
    constexpr auto begin() { return stl_iterator<T *>(data()); }
    constexpr auto begin() const { return stl_iterator<T const *>(data()); }
    constexpr auto end() { return have_default_strides ? stl_iterator<T *>(data()+size()) : stl_iterator<T *>(); }
    constexpr auto end() const { return have_default_strides ? stl_iterator<T const *>(data()+size()) : stl_iterator<T const *>(); }

// Renames.
#define COMP_RENAME_C(name__, req_dim0__, req_dim1__, CONST)            \
    operator name__<T> CONST &() CONST                                  \
    {                                                                   \
        static_assert(std::is_same_v<strides, default_strides_<mp::int_list<req_dim0__, req_dim1__>>>, "renames only on default strides"); \
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
        static_assert(std::is_same_v<strides, default_strides_<mp::int_list<dim0__>>>, "renames only on default strides"); \
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
// Strides are compile time known, so we can put most members in the view type.
// ---------------------

template <class T, class sizes, class strides>
struct SmallView: public SmallBase<SmallView, T, sizes, strides>
{
    using Base = SmallBase<ra::SmallView, T, sizes, strides>; // TODO qualifier needed for clang 3.8 bug

    T * p;
    constexpr SmallView(T * p_): p(p_) {}
    constexpr SmallView(SmallView const & s): p(s.p) {}

    constexpr operator T & ()
    {
        static_assert(Base::rank()==0 || (Base::rank()==1 && Base::size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    }
    constexpr operator T const & () const
    {
        static_assert(Base::rank()==0 || (Base::rank()==1 && Base::size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    };

    using Base::operator=;
// templated operator= cannot take these over. As in ra::View.
    SmallView & operator=(SmallView && x) { return Base::operator=(x); }
    SmallView & operator=(SmallView const & x) { return Base::operator=(x); }
};

#define DEF_SMALLARRAY                                                  \
    /*  TODO qualifier needed for clang 3.8 bug */                      \
    using Base = SmallBase<ra::SmallArray, T, sizes, strides>;          \
    T p[Base::size()];                                                  \
                                                                        \
    constexpr SmallArray(): p() {}                                      \
    template <class X>                                                  \
    constexpr SmallArray(X && x): p()  /* init for constexpr */         \
    {                                                                   \
        static_cast<Base &>(*this) = x;                                 \
    }                                                                   \
    /* List type is not deduced by (X &&) */                            \
    constexpr SmallArray(typename nested_braces<T, Base::rank()>::list x): p() \
    {                                                                   \
        static_cast<Base &>(*this) = x;                                 \
    }                                                                   \
    /* List type is not deduced by (X &&) */                            \
    constexpr SmallArray(std::initializer_list<T> x): p()               \
    {                                                                   \
        static_cast<Base &>(*this) = x;                                 \
    }                                                                   \
    /* test-small-0 and test-small-1 pass without these two; make sure they're needed. */ \
    /* 1. Remove for start(begin) or start(begin, end); only here because std::vector has it */ \
    template <class P>                                                  \
    constexpr SmallArray(P begin, P end)                                \
    {                                                                   \
        std::copy(begin, end, this->begin());                           \
    }                                                                   \
    /* 2. Special case is needed if ra::start() doesn't acknowledge T as scalar */ \
    constexpr SmallArray(T const & t)                                   \
    {                                                                   \
        std::fill(this->begin(), this->end(), t);                       \
    }

template <class T, class sizes, class strides>
struct SmallArray: public SmallBase<SmallArray, T, sizes, strides>
{
    DEF_SMALLARRAY

    using Base::rank;
    using Base::size;

    constexpr operator SmallView<T, sizes, strides> () { return SmallView<T, sizes, strides>(p); }
    constexpr operator SmallView<T const, sizes, strides> const () { return SmallView<T const, sizes, strides>(p); }

// BUG these make SmallArray<T, N> std::is_convertible to T even though conversion isn't possible b/c of the assert.
    constexpr operator T & ()
    {
        static_assert(rank()==0 || (rank()==1 && size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    };
    constexpr operator T const & () const
    {
        static_assert(rank()==0 || (rank()==1 && size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    };
    using Base::operator=;

    T const & back() const { static_assert(Base::rank()==1, "bad rank for back"); return (*this)[Base::size()-1]; }
    T & back() { static_assert(Base::rank()==1, "bad rank for back"); return (*this)[Base::size()-1]; }
};

// This specialization exists only to avoid -Warray-bounds on operator[] when sizes is [0].
template <class T, class strides>
struct SmallArray<T, mp::int_list<0>, strides>: public SmallBase<SmallArray, T, mp::int_list<0>, strides>
{
    using sizes = mp::int_list<0>;

    DEF_SMALLARRAY

    T & operator[](dim_t const i) { assert(0 && "internal error"); }
    T const & operator[](dim_t const i) const { assert(0 && "internal error"); }
    operator SmallView<T, sizes, strides> () { return SmallView<T, sizes, strides>(p); }
    operator SmallView<T const, sizes, strides> const () { return SmallView<T const, sizes, strides>(p); }
};
#undef DEF_SMALLARRAY

template <template <class, class, class> class Type_,
          class T, class sizes, class strides>
struct ra_traits_small
{
    using V = Type_<T, sizes, strides>;
    using value_type = T;
    using shape_type = std::decay_t<decltype(V::ssizes)>;
    // constexpr static auto const & shape(V const & v) { return V::ssizes; }
    constexpr static auto shape(V const & v) { return SmallView<ra::dim_t const, mp::int_list<V::rank_s()>, mp::int_list<1>>(V::ssizes.data()); }
#define MAKE_COND (std::is_same_v<strides, mp::int_list<1>> && std::is_same_v<sizes, mp::Take_<sizes, 1>>)
    constexpr static V make(dim_t const n)
    {
        static_assert(MAKE_COND, "bad type for ra_traits::make");
        assert(n==V::size(0));
        return V {};
    }
    template <class TT> static V make(dim_t n, TT const & t)
    {
        static_assert(MAKE_COND, "bad type for ra_traits::make");
        assert(n==V::size(0));
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

template <class A, class i>
struct axis_indices
{
    template <class T> struct MatchIndex { using type = mp::int_t<(T::value==i::value)>; };
    using I = mp::Iota_<mp::len<A>>;
    using W = mp::Map_<MatchIndex, A>;
    using type = mp::Filter_<W, I>;
    static_assert(mp::len<type>> 0, "dst axis doesn't appear in transposed axes list");
};

template <class axes_list, class src_sizes, class src_strides>
struct axes_list_indices
{
    static_assert(mp::len<axes_list> == mp::len<src_sizes>, "bad size for transposed axes list");
    constexpr static int talmax = mp::Fold_<mp::Max, void, axes_list>::value;
    constexpr static int talmin = mp::Fold_<mp::Min, void, axes_list>::value;
    static_assert(talmax < mp::len<src_sizes>, "bad index in transposed axes list");
    static_assert(talmin >= 0, "bad index in transposed axes list");

    template <class dst_i> struct dst_indices
    {
        using type = typename axis_indices<axes_list, dst_i>::type;
        template <class i> using sizesi = mp::Ref<src_sizes, i::value>;
        template <class i> using stridesi = mp::Ref<src_strides, i::value>;
        using stride = mp::Fold_<mp::Sum, void, mp::Map_<stridesi, type>>;
        using size = mp::Fold_<mp::Min, void, mp::Map_<sizesi, type>>;
    };

    template <class dst_i> struct dst_size { using type = typename dst_indices<dst_i>::size; };
    template <class dst_i> struct dst_stride { using type = typename dst_indices<dst_i>::stride; };

    using dst = mp::Iota_<(talmax>=0 ? (1+talmax) : 0)>;
    using type = mp::Map_<dst_indices, dst>;
    using sizes =  mp::Map_<dst_size, dst>;
    using strides =  mp::Map_<dst_stride, dst>;
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
DEF_TRANSPOSE(const)
DEF_TRANSPOSE(/* */)
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
    template <class T> struct op
    {
        static_assert((T::value/s)*s==T::value);
        using type = mp::int_t<T::value / s>;
    };
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
    static_assert(std::is_same<mp::Drop_<sizes, ra-rb>, typename super_t::sizes>::value);
    static_assert(std::is_same<mp::Drop_<strides, ra-rb>, typename super_t::strides>::value);
    using cstrides = mp::Map_<explode_divop<super_t::size_s()>::template op, mp::Take_<strides, ra-rb>>;
    return SmallView<super_t, mp::Take_<sizes, ra-rb>, cstrides>((super_t *) a.data());
}

template <class T, class I=std::make_integer_sequence<int, std::rank_v<T>>>
struct builtin_array_sizes;

template <class T, int ... I>
struct builtin_array_sizes<T, std::integer_sequence<int, I ...> >
{
    using type = mp::int_list<std::extent_v<T, I> ...>;
};

template <class T> using builtin_array_sizes_t = typename builtin_array_sizes<T>::type;

// forward declared in type.H.
template <class T, std::enable_if_t<is_builtin_array<T>, int>>
inline constexpr auto start(T && t)
{
// preserve const.
    using A = std::remove_volatile_t<std::remove_reference_t<T>>;
    using E = std::remove_all_extents_t<A>;
    using sizes = builtin_array_sizes_t<A>;
    return SmallView<E, sizes, default_strides_<sizes>>((E *)(t)).iter();
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_SMALL
