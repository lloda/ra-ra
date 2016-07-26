
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_SMALL_H
#define RA_SMALL_H

#include "ra/ra-ply.H"
#include <iterator>

/// @file ra-small.H
/// @brief Fixed-size multidimensional arrays.

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_SMALL RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_SMALL
        #define RA_CHECK_BOUNDS_RA_SMALL 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_SMALL==0
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
        static_assert(mp::len<sizes> >= P::size(), "too many indices"); // gcc accepts p.size()
        return Indexer0<sizes, strides>::template index_p_<P::size(), 0>(0, p);
    }
    template <class P, std::enable_if_t<ra_traits<P>::size_s()!=RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
// gcc accepts p.size(), but I also need P = std::array to work.
        static_assert(mp::len<sizes> <= ra_traits<P>::size_s(), "too few indices");
        return Indexer0<sizes, strides>::template index_p_<mp::len<sizes>, 0>(0, p);
    }
    template <class P, std::enable_if_t<ra_traits<P>::size_s()==RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        CHECK_BOUNDS(mp::len<sizes> <= p.size());
        return Indexer0<sizes, strides>::template index_p_<mp::len<sizes>, 0>(0, p);
    }
};

struct Indexer1
{
    template <rank_t k, class Dim>
    constexpr static dim_t index_(Dim const & dim)
    {
        return 0;
    }
    template <rank_t k, class Dim, class Iarg0, class ... Iarg>
    constexpr static dim_t index_(Dim const & dim, Iarg0 const i0_, Iarg const ... i_)
    {
        CHECK_BOUNDS(inside(i0_, dim[k].size));
        return i0_ * dim[k].stride + index_<k+1>(dim, i_ ...);
    }
    template <class Dim, class ... Iarg>
    static dim_t index(Dim const & dim, Iarg const ... i_)
    {
        CHECK_BOUNDS(dim.size()>=sizeof...(Iarg) && "too many indices");
        return index_<0>(dim, i_ ...);
    }

    template <class Dim, class P>
    constexpr static dim_t index_p_(Dim const & dim, P const & p)
    {
        dim_t c = 0;
        for (rank_t k=0; k<p.size(); ++k) {
            CHECK_BOUNDS(inside(p[k], dim[k].size));
            c += dim[k].stride * p[k];
        }
        return c;
    }
    template <class Dim, class P>
    static dim_t index_p(Dim const & dim, P const & p)
    {
        CHECK_BOUNDS(dim.size()>=p.size() && "too many indices");
        return index_p_(dim, p);
    }

// used by ra_iterator::at() for rank matching on rank<driving rank, no slicing. @todo Static check.
    template <class Dim, class P>
    static dim_t index_short(rank_t framer, Dim const & dim, P const & p)
    {
        dim_t c = 0;
        for (rank_t k=0; k<framer; ++k) {
            CHECK_BOUNDS(inside(p[k], dim[k].size));
            c += dim[k].stride * p[k];
        }
        return c;
    }
};

// Static dimensions, stack storage. See Raw, Unique, Shared.
template <class T, class sizes, class strides> struct SmallArray; // Used as type of Small::iterator::shape().
template <class T, class sizes, class strides> struct SmallSlice; // Used as types of Small::iterator::at().

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

template <dim_t size0, dim_t stride0> constexpr dim_t select(dim_t i0)
{
    return ([i0](){ CHECK_BOUNDS(inside(i0, size0)); })(), i0*stride0;
};
template <dim_t size0, dim_t stride0, int n> constexpr dim_t select(dots_t<n> i0)
{
    return 0;
}

template <class sizes, class strides>
constexpr dim_t select_loop()
{
    return 0;
}
template <class sizes, class strides, class I0, class ... I>
constexpr dim_t select_loop(I0 i0, I ... i)
{
    constexpr int s_src = is_beatable<I0>::skip_src;
    return select<mp::First_<sizes>::value, mp::First_<strides>::value>(i0)
        + select_loop<mp::Drop_<sizes, s_src>, mp::Drop_<strides, s_src>>(i ...);
}

// Component renames. // @TODO Tests.
template <class T> struct mat_uv   { T uu, uv, vu, vv; };
template <class T> struct mat_xy   { T xx, xy, yx, yy; };
template <class T> struct mat_uvz  { T uu, uv, uz, vu, vv, vz, zu, zv, zz; };
template <class T> struct mat_xyz  { T xx, xy, xz, yx, yy, yz, zx, zy, zz; };
template <class T> struct mat_xyzw { T xx, xy, xz, xw, yx, yy, yz, yw,
                                       zx, zy, zz, zw, wx, wy, wz, ww; };

template <class T> struct vec_uv   { T u, v; };
template <class T> struct vec_xy   { T x, y; };
template <class T> struct vec_tp   { T t, p; };
template <class T> struct vec_uvz  { T u, v, z; };
template <class T> struct vec_xyz  { T x, y, z; };
template <class T> struct vec_rtp  { T r, t, p; };
template <class T> struct vec_xyzw { T u, v, z, w; };

template <class T, class sizes, class strides>
struct SmallRenames {};

template <class T, class strides, int M, int N>
struct SmallRenames<T, mp::int_list<M, N>, strides>
{
    constexpr static bool have_default_strides = std::is_same<strides, default_strides_<mp::int_list<M, N>>>::value;

#define COMPONENT_RENAME(name__, req_dim0__, req_dim1__)                \
    operator name__<T> &()                                              \
    {                                                                   \
        static_assert(have_default_strides, "renames only on default strides"); \
        static_assert(M==req_dim0__ && N==req_dim1__, "dimension error"); \
        return reinterpret_cast<name__<T> &>(*this);                    \
    }                                                                   \
    operator name__<T> const &() const                                  \
    {                                                                   \
        static_assert(have_default_strides, "renames only on default strides"); \
        static_assert(M==req_dim0__ && N==req_dim1__, "dimension_error"); \
        return reinterpret_cast<name__<T> const &>(*this);              \
    }
    COMPONENT_RENAME(mat_xy,   2, 2)
    COMPONENT_RENAME(mat_uv,   2, 2)
    COMPONENT_RENAME(mat_xyz,  3, 3)
    COMPONENT_RENAME(mat_uvz,  3, 3)
    COMPONENT_RENAME(mat_xyzw, 4, 4)
#undef COMPONENT_RENAME
};

template <class T, class strides, int M>
struct SmallRenames<T, mp::int_list<M>, strides>
{
    constexpr static bool have_default_strides = std::is_same<strides, default_strides_<mp::int_list<M>>>::value;

#define COMPONENT_RENAME(name__, req_dim0__)                            \
    operator name__<T> &()                                              \
    {                                                                   \
        static_assert(have_default_strides, "renames only on default strides"); \
        static_assert(M==req_dim0__, "dimension error");                \
        return reinterpret_cast<name__<T> &>(*this);                    \
    }                                                                   \
    operator name__<T> const &() const                                  \
    {                                                                   \
        static_assert(have_default_strides, "renames only on default strides"); \
        static_assert(M==req_dim0__, "dimension_error");                \
        return reinterpret_cast<name__<T> const &>(*this);              \
    }
    COMPONENT_RENAME(vec_xy,   2)
    COMPONENT_RENAME(vec_uv,   2)
    COMPONENT_RENAME(vec_tp,   2)
    COMPONENT_RENAME(vec_xyz,  3)
    COMPONENT_RENAME(vec_uvz,  3)
    COMPONENT_RENAME(vec_rtp,  3)
    COMPONENT_RENAME(vec_xyzw, 4)
#undef COMPONENT_RENAME
};

// @TODO Should this be itself an xpr type, or do I need a ra_iterator-like obj?
template <template <class, class, class> class Child_, class T, class sizes_, class strides_>
struct SmallBase: public SmallRenames<T, sizes_, strides_>
{
    using sizes = sizes_;
    using strides = strides_;

// @TODO complete static check on strides.
    template <class TTT> struct BadDimension
    {
        using type = mp::int_t<(TTT::value<0 || TTT::value==DIM_ANY || TTT::value==DIM_BAD)>;
    };
    static_assert(mp::len<sizes> == mp::len<strides>, "bad strides");
    static_assert(!mp::Apply_<mp::Or, mp::Map_<BadDimension, sizes>>::value, "negative dimensions");

// @TODO look at Dim below; useable in the same way?
    using Child = Child_<T, sizes, strides>;

    constexpr static dim_t size() { return mp::Apply_<mp::Prod, sizes>::value; }
    constexpr static rank_t rank()  { return mp::len<sizes>; }
// @TODO Remove; used by ra::cross() to treat is_slice & is_array_iterator uniformly, which can't be done in general.
    constexpr static dim_t size_s() { return size(); }
    constexpr static dim_t size(int const j) { return mp::on_tuple<sizes>::ref(j); }
    constexpr static dim_t stride(int const j) { return mp::on_tuple<strides>::ref(j); }

    constexpr T * data() { return static_cast<Child &>(*this).p; }
    constexpr T const * data() const { return static_cast<Child const &>(*this).p; }

// Specialize for rank() integer-args -> scalar, same in ra::Raw in ra-large.H.
// @TODO waiting for fold expressions (N4191) on some of these enable_if_ts.
// @TODO Cf https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations#recs.cpp17
#define SUBSCRIPTS(CONST)                                               \
    template <class ... I, std::enable_if_t<(mp::Count<std::is_integral<I> ...>::value<rank() \
                                       && mp::And<mp::int_t<is_beatable<I>::static_p> ...>::value), int> = 0> \
    constexpr auto operator()(I ... i) CONST                            \
    {                                                                   \
        using FD = FilterDims<sizes, strides, I ...>;                   \
        return SmallSlice<T CONST, typename FD::sizes, typename FD::strides> \
            (data()+select_loop<sizes, strides>(i ...));                \
    }                                                                   \
    template <class ... I, std::enable_if_t<mp::Count<std::is_integral<I> ...>::value==rank(), int> = 0> \
    decltype(auto) operator()(I ... i) CONST                            \
    {                                                                   \
        return data()[select_loop<sizes, strides>(i ...)];              \
    } /* @TODO More than one selector... */                             \
    template <class ... I, std::enable_if_t<mp::Or<mp::int_t< !is_beatable<I>::static_p> ...>::value, int> = 0> \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
 /* @BUG I must be fixed size, otherwise we can't make out the output type. */ \
    template <class I>                                                  \
    constexpr auto at(I const & i) CONST                          \
    {                                                                   \
        return SmallSlice<T CONST, mp::Drop_<sizes, I::size()>, mp::Drop_<strides, I::size()>> \
            (data()+Indexer0<sizes, strides>::index_p(i));              \
    }                                                                   \
    decltype(auto) operator[](dim_t const i) CONST                      \
    {                                                                   \
        static_assert(rank()==1, "bad rank");                           \
        return data()[i*mp::Ref_<strides, 0>::value];                   \
    }
    SUBSCRIPTS(const)
    SUBSCRIPTS(/*const*/)
#undef SUBSCRIPTS

// See same thing for Raw.
#define DEFINE_ASSIGNMENT_OP(OPNAME, OP)                                \
    template <class X> Child & OPNAME(X && x)                           \
    {                                                                   \
        /* @TODO generally forbid writing to smaller rank from larger rank (ra-small.H [ref01]) */ \
        static_assert(std::decay_t<decltype(start(x))>::rank_s()<=rank() \
                      || std::decay_t<decltype(start(x))>::rank_s()==RANK_ANY, "bad assigment"); \
        ply_either(expr([](T & dest, T const & src) { dest OP src; }, this->iter(), start(x))); \
        return static_cast<Child &>(*this);                             \
    }
    DEFINE_ASSIGNMENT_OP(operator=, =)
    DEFINE_ASSIGNMENT_OP(operator+=, +=)
    DEFINE_ASSIGNMENT_OP(operator-=, -=)
    DEFINE_ASSIGNMENT_OP(operator/=, /=)
    DEFINE_ASSIGNMENT_OP(operator*=, *=)
#undef DEFINE_ASSIGNMENT_OP
    Child & operator=(std::initializer_list<T> const x)
    {
        assert(x.size()==size() && "bad initializer list"); /* static_assert */
        std::copy(x.begin(), x.end(), this->begin());
        return static_cast<Child &>(*this);
    }

// @TODO Would replace by s(ra::iota) if that could be made constexpr
#define DEFINE_AS(CONST)                                                \
    template <int ss, int oo=0>                                         \
    auto as() CONST                                                     \
    {                                                                   \
        static_assert(rank()>=1, "bad rank for as<>");                  \
        static_assert(ss>=0 && oo>=0 && ss+oo<=size(), "bad size for as<>"); \
        return SmallSlice<T CONST, mp::Cons_<mp::int_t<ss>, mp::Drop1_<sizes> >, strides>(this->data()+oo*this->stride(0)); \
    }
    DEFINE_AS(const)
    DEFINE_AS(/* const */)
#undef DEFINE_AS

// @TODO This is scalar only. Eventually reuse cell_iterator.
    template <class P>
    struct Iterator
    {
// @TODO See ra_traits<Small>. Why do these twice?
        using shape_type = Small<dim_t, mp::len<sizes> >;
        static auto shape() { return mp::tuple_copy<sizes, shape_type>::f(); }

        P p;

// @TODO use cellp for cellr>0, when that's supported (or replace all this by ra::ra_iterator). But for cellr==0 we must use p, because in auto contexts (e.g. ra-operators.H) *(flat()) won't get converted to *p, and that can result in infinite looping in ply_ravel().
        // auto flat() { return cellp<SmallSlice<T, mp::nil, mp::nil>> { p }; }
        // auto flat() const { return cellp<SmallSlice<T, mp::nil, mp::nil>> { p }; }
        auto flat() { return p; }
        auto flat() const { return p; }

        using value_type = typename std::iterator_traits<P>::value_type;

        bool done() const { return p==nullptr; }

        Iterator(P p_): p(size()==0 ? nullptr: p_) {}
        template <class PP> Iterator(PP const & q): p(size()==0 ? nullptr : q.p) {}
        Iterator(): p(nullptr) {}

// For xpr use; cf this stride() against SmallBase::stride().
// @TODO Should have support in xpr and traversal for static bounds; so on_tuple is not needed.
        constexpr static dim_t size(int const j) { return mp::on_tuple<sizes>::ref(j); }
        constexpr static dim_t stride(int const j) { return j<rank() ? mp::on_tuple<strides>::ref(j) : 0; }
        constexpr static dim_t preferred_stride(int const j) { return stride(j); }

// @TODO We really shouldn't need this redundant stuff :-(
        constexpr static dim_t size() { return SmallBase::size(); }
        constexpr static rank_t rank() { return SmallBase::rank(); }
        constexpr static dim_t size_s() { return size(); }
        constexpr static rank_t rank_s() { return rank(); }

        template <class I> decltype(auto) at(I const & i_)
        {
            return p[Indexer0<sizes, strides>::index_short(i_)];
        }
        void adv(rank_t const k, dim_t const d)
        {
            p += (k<rank()) * stride(k)*d;
        }
        bool constexpr compact_stride(dim_t const step, int const z, int const j) const
        {
            return step*(z<rank() ? stride(z) : 0)==(j<rank() ? stride(j) : 0);
        }
    };

    template <class Iterator>
    struct general_stl_iterator: private Iterator
    {
        using value_type = typename Iterator::value_type;
        using difference_type = dim_t;
        using pointer = value_type *;
        using reference = value_type &;
        using const_reference = value_type const &;
        using iterator_category = std::forward_iterator_tag;

        typename Iterator::shape_type i;
        template <class PP> general_stl_iterator(PP const & p): Iterator(p) { std::fill(i.begin(), i.end(), 0.); }
        general_stl_iterator(): Iterator() {}

        template <class PP> bool operator==(PP const & q) const { return Iterator::p==q.p; }
        template <class PP> bool operator!=(PP const & q) const { return Iterator::p!=q.p; }
        decltype(auto) operator*() const { return *Iterator::p; }
        decltype(auto) operator*() { return *Iterator::p; }
        template <int k=rank()-1>
        std::enable_if_t<(k>=0)> next_in_cube()
        {
            ++i[k];
            if (i[k]<mp::Ref_<sizes, k>::value) {
                Iterator::p += mp::Ref_<strides, k>::value;
            } else {
                i[k] = 0;
                Iterator::p -= mp::Ref_<strides, k>::value*(mp::Ref_<sizes, k>::value-1);
                next_in_cube<k-1>();
                return;
            }
        }
        template <int k> std::enable_if_t<(k<0)> next_in_cube() { Iterator::p = nullptr; }
        general_stl_iterator & operator++() { next_in_cube(); return *this; }
    };

    constexpr static bool have_default_strides = std::is_same<strides, default_strides_<sizes>>::value;
    template <class P> using stl_iterator = mp::If_<have_default_strides, P, general_stl_iterator<Iterator<P>>>;

// @TODO A cell-rank>0 iterator.
    constexpr auto iter() { return Iterator<T *>(data()); }
    constexpr auto iter() const { return Iterator<T const *>(data()); }
    constexpr auto begin() { return stl_iterator<T *>(data()); }
    constexpr auto begin() const { return stl_iterator<T const *>(data()); }
    constexpr auto end() { return have_default_strides ? stl_iterator<T *>(data()+size()) : stl_iterator<T *>(); }
    constexpr auto end() const { return have_default_strides ? stl_iterator<T const *>(data()+size()) : stl_iterator<T const *>(); }

};

template <class T, class sizes, class strides>
struct SmallSlice: public SmallBase<SmallSlice, T, sizes, strides>
{
    using Base = SmallBase<ra::SmallSlice, T, sizes, strides>; // @TODO qualifier needed for clang 3.8 bug

    T * p;
    constexpr SmallSlice(T * p_): p(p_) {}
    constexpr SmallSlice(SmallSlice && s): p(std::move(s.p)) {}
    constexpr SmallSlice(SmallSlice const & s): p(s.p) {}

    operator T & ()
    {
        static_assert(Base::rank()==0 || (Base::rank()==1 && Base::size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    }
    operator T const & () const
    {
        static_assert(Base::rank()==0 || (Base::rank()==1 && Base::size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    };

    using Base::operator=;
// templated operator= cannot take these over. As in ra::Raw.
    SmallSlice & operator=(SmallSlice && x)
    {
        ply_either(expr([](T & dest, T const & src) { dest = src; }, this->iter(), start(x)));
        return *this;
    }
    SmallSlice & operator=(SmallSlice const & x)
    {
        ply_either(expr([](T & dest, T const & src) { dest = src; }, this->iter(), start(x)));
        return *this;
    }
};

#define SMALLARRAY_DEFINITIONS                                          \
    /*  @TODO qualifier needed for clang 3.8 bug */                     \
    using Base = SmallBase<ra::SmallArray, T, sizes, strides>;          \
    T p[Base::size()];                                                  \
                                                                        \
    SmallArray() {}                                                     \
    explicit SmallArray(init_not_t) {}                                  \
    SmallArray(std::initializer_list<T> const x)                        \
    {                                                                   \
        assert(x.size()==Base::size() && "bad initializer list"); /* static_assert */ \
        std::copy(x.begin(), x.end(), this->begin());                   \
    }                                                                   \
    template <class P> /* @TODO remove; only here because std::vector has it */ \
    SmallArray(P begin, P end)                                          \
    {                                                                   \
        std::copy(begin, end, this->begin());                           \
    }                                                                   \
    /* Special case and qualifier are needed because ra::start() may    \
       not accept T as scalar (say if T is a struct) */                 \
    SmallArray(T const & t)                                             \
    {                                                                   \
        std::fill(this->begin(), this->end(), t);                       \
    }                                                                   \
    template <class R, std::enable_if_t<std::is_convertible<R, T>::value && is_scalar<R>, int> = 0> \
    SmallArray(R const & r)                                             \
    {                                                                   \
        std::fill(this->begin(), this->end(), r);                       \
    }                                                                   \
    template <class X, std::enable_if_t<is_ra<X>, int> = 0>                        \
    SmallArray(X && x)                                                  \
    {                                                                   \
        ply_either(expr([](T & dest, T const & src) { dest = src; }, Base::iter(), start(x))); \
    }

template <class T, class sizes, class strides>
struct SmallArray: public SmallBase<SmallArray, T, sizes, strides>
{
    SMALLARRAY_DEFINITIONS

    using Base::rank;
    using Base::size;

    operator SmallSlice<T, sizes, strides> () { return SmallSlice<T, sizes, strides>(p); }
    operator SmallSlice<T const, sizes, strides> const () { return SmallSlice<T const, sizes, strides>(p); }

// @BUG these make SmallArray<T, N> std::is_convertible to T even though conversion isn't possible b/c of the assert.
    operator T & ()
    {
        static_assert(rank()==0 || (rank()==1 && size()==1), "bad rank"); // rank 1 for coord types
        return p[0];
    };
    operator T const & () const
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

    SMALLARRAY_DEFINITIONS

    T & operator[](dim_t const i) { assert(0 && "internal error"); abort(); }
    T const & operator[](dim_t const i) const { assert(0 && "internal error"); abort(); }
    operator SmallSlice<T, sizes, strides> () { return SmallSlice<T, sizes, strides>(p); }
    operator SmallSlice<T const, sizes, strides> const () { return SmallSlice<T const, sizes, strides>(p); }
};

#undef SMALLARRAY_DEFINITIONS

template <template <class, class, class> class Type_,
          class T, class sizes, class strides>
struct ra_traits_small
{
    using V = Type_<T, sizes, strides>;
    using value_type = T;

    using shape_type = Small<dim_t, V::rank()>;
    static shape_type shape(V const & v) { return mp::tuple_copy<sizes, shape_type>::f(); }
#define MAKE_COND (mp::And<std::is_same<strides, mp::int_list<1>>, std::is_same<sizes, mp::Take_<sizes, 1>>>::value)
    static V make(dim_t const n)
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
struct ra_traits_def<SmallSlice<T, sizes, strides>>: public ra_traits_small<SmallSlice, T, sizes, strides> {};

template <class A, class i>
struct axis_indices
{
    template <class T> struct MatchIndex
    {
        using type = mp::int_t<(T::value==i::value)>;
    };
    using I = mp::Iota_<mp::len<A> >;
    using W = mp::Map_<MatchIndex, A>;
    using type = mp::Filter_<W, I>;
    static_assert(mp::len<type> > 0, "dst axis doesn't appear in transposed axes list");
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
        using stride = mp::Fold_<mp::Sum, void, mp::Map_<stridesi, type> >;
        using size = mp::Fold_<mp::Min, void, mp::Map_<sizesi, type> >;
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
        return SmallSlice<T CONST, typename ti::sizes, typename ti::strides>(a.data()); \
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

// @TODO Used by ProductRule; waiting for proper generalization.
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

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_SMALL

#endif // RA_SMALL_H
