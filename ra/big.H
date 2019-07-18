// -*- mode: c++; coding: utf-8 -*-
/// @file big.H
/// @brief Arrays with dynamic size.

// (c) Daniel Llorens - 2013-2014, 2017-2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/small.H"
#include <memory>
#include <iostream>

#if defined(RA_CHECK_BOUNDS) && RA_CHECK_BOUNDS==0
  #define CHECK_BOUNDS( cond )
#else
  #define CHECK_BOUNDS( cond ) RA_ASSERT( cond, 0 )
#endif

namespace ra {

// Dope vector element
struct Dim { dim_t size, stride; };

// For debugging
inline std::ostream & operator<<(std::ostream & o, Dim const & dim)
{ o << "[Dim " << dim.size << " " << dim.stride << "]"; return o; }


// --------------------
// nested braces for Container initializers
// --------------------

// Avoid clash of T with scalar constructors (for rank 0).
template <class T, rank_t rank, class Enable=void>
struct nested_braces { using list = no_arg; };

template <class T, rank_t rank>
struct nested_braces<T, rank, std::enable_if_t<(rank==1)>>
{
    using list = std::initializer_list<T>;
    template <size_t N> constexpr static
    void shape(list l, std::array<dim_t, N> & s)
    {
        static_assert(N>0);
        s[N-1] = l.size();
    }
};

template <class T, rank_t rank>
struct nested_braces<T, rank, std::enable_if_t<(rank>1)>>
{
    using sub = nested_braces<T, rank-1>;
    using list = std::initializer_list<typename sub::list>;
    template <size_t N> constexpr static
    void shape(list l, std::array<dim_t, N> & s)
    {
        s[N-rank] = l.size();
        if (l.size()>0) {
            sub::template shape(*(l.begin()), s);
        } else {
            std::fill(s.begin()+N-rank+1, s.end(), 0);
        }
    }
};


// --------------------
// Develop indices for Big
// --------------------

struct Indexer1
{
    template <class Dim, class P>
    constexpr static dim_t index_p(Dim const & dim, P && p)
    {
        CHECK_BOUNDS(dim_t(dim.size())>=start(p).size(0) && "too many indices");
// use dim.data() to skip the size check.
        dim_t c = 0;
        for_each([&c](auto && d, auto && p) { CHECK_BOUNDS(inside(p, d.size)); c += d.stride*p; },
                 ptr(dim.data()), p);
        return c;
    }

// used by cell_iterator::at() for rank matching on rank<driving rank, no slicing. TODO Static check.
    template <class Dim, class P>
    constexpr static dim_t index_short(rank_t framer, Dim const & dim, P const & p)
    {
        dim_t c = 0;
        for (rank_t k=0; k<framer; ++k) {
            CHECK_BOUNDS(inside(p[k], dim[k].size) || (dim[k].size==DIM_BAD && dim[k].stride==0));
            c += dim[k].stride * p[k];
        }
        return c;
    }
};


// --------------------
// Big iterator
// --------------------
// TODO Refactor with cell_iterator_small for SmallView?

// V is View. FIXME Parameterize? apparently only for order-of-decl.
template <class V, rank_t cellr_=0>
struct cell_iterator
{
    constexpr static rank_t cellr_spec = cellr_;
    static_assert(cellr_spec!=RANK_ANY && cellr_spec!=RANK_BAD, "bad cell rank");
    constexpr static rank_t fullr = std::decay_t<V>::rank_s();
    constexpr static rank_t cellr = dependent_cell_rank(fullr, cellr_spec);
    constexpr static rank_t framer = dependent_frame_rank(fullr, cellr_spec);
    static_assert(cellr>=0 || cellr==RANK_ANY, "bad cell rank");
    static_assert(framer>=0 || framer==RANK_ANY, "bad frame rank");
    static_assert(fullr==cellr || gt_rank(fullr, cellr), "bad cell rank");

    using Dimv_ = typename std::decay_t<V>::Dimv;
    using Dimv = std::conditional_t<std::is_lvalue_reference_v<V>, Dimv_ const &, Dimv_>;
    Dimv dim;

    constexpr static rank_t rank_s() { return framer; }
    constexpr rank_t rank() const { return dependent_frame_rank(rank_t(dim.size()), cellr_spec); }

    using shape_type = std::conditional_t<rank_s()==DIM_ANY, std::vector<dim_t>,
                                          Small<dim_t, rank_s()==DIM_ANY ? 0 : rank_s()>>; // still needs protection :-/
    using atom_type = typename ra_traits<V>::value_type;
    using cell_type = View<atom_type, cellr>;
    using value_type = std::conditional_t<0==cellr, atom_type, cell_type>;

    cell_type c;

    constexpr cell_iterator(cell_iterator const & ci): dim(ci.dim), c { ci.c.dim, ci.c.p } {}
// s_ is array's full shape; split it into dim/i (frame) and c (cell).
    constexpr cell_iterator(Dimv const & dim_, atom_type * p_): dim(dim_)
    {
        rank_t rank = this->rank();
// see stl_iterator for the case of dim_[0]=0, etc. [ra12].
        c.p = p_;
        rank_t cellr = dependent_cell_rank(rank_t(dim.size()), cellr_spec);
        resize(c.dim, cellr);
        for (int k=0; k<cellr; ++k) {
            c.dim[k] = dim_[k+rank];
        }
    }

    constexpr static dim_t size_s(int i) { /* CHECK_BOUNDS(inside(k, rank())); */return DIM_ANY; }
    constexpr dim_t size(int k) const { CHECK_BOUNDS(inside(k, rank())); return dim[k].size; }
    constexpr dim_t stride(int k) const { return k<rank() ? dim[k].stride : 0; }
// FIXME handle z or j over rank()? check cell_iterator_small versions.
    constexpr bool keep_stride(dim_t step, int z, int j) const { return step*stride(z)==stride(j); }
    constexpr void adv(rank_t k, dim_t d) { c.p += stride(k)*d; }

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
    decltype(auto) at(I const & i_)
    {
        CHECK_BOUNDS(rank()<=dim_t(i_.size()) && "too few indices");
        if constexpr (0==cellr) {
            return c.p[Indexer1::index_short(rank(), dim, i_)];
        } else {
            return cell_type { c.dim, c.p+Indexer1::index_short(rank(), dim, i_) };
        }
    }

#define DEF_ASSIGNOPS(OP)                                               \
    template <class X> decltype(auto) operator OP(X && x)               \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); return *this; }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};


// --------------------
// Indexing views
// --------------------

// Always C order. If you need another, transpose this.
// Works on dim vector with sizes already assigned, so that I can work from an expr. Not pretty though.
template <class D>
dim_t filldim(int n, D dend)
{
    dim_t next = 1;
    for (; n>0; --n) {
        --dend;
        CHECK_BOUNDS((*dend).size>=0 && "bad dim");
        (*dend).stride = next;
        next *= (*dend).size;
    }
    return next;
}

template <class D>
dim_t proddim(D d, D dend)
{
    dim_t t = 1;
    for (; d!=dend; ++d) {
        t *= (*d).size;
    }
    return t;
}

// raw <- shared; raw <- unique; shared <-- unique.
// layout is
// [data] (fixed shape)
// [size] p -> [data...] (fixed rank)
// [rank] [size...] p -> [data...] (var rank)
// TODO size is immutable so that it can be kept together with rank.

#define DEF_ITERATORS(RANK)                                             \
    template <rank_t c=0> constexpr auto iter() && { return ra::cell_iterator<View<T, RANK>, c>(std::move(dim), p); } \
    template <rank_t c=0> constexpr auto iter() & { return ra::cell_iterator<View<T, RANK> &, c>(dim, p); } \
    template <rank_t c=0> constexpr auto iter() const & { return ra::cell_iterator<View<T const, RANK> &, c>(dim, p); } \
    constexpr auto begin() const { return stl_iterator(iter()); }                 \
    constexpr auto begin() { return stl_iterator(iter()); }                       \
    /* here dim doesn't matter, but we have to give it if it's a ref. */ \
    constexpr auto end() const { return stl_iterator(decltype(iter())(dim, nullptr)); } \
    constexpr auto end() { return stl_iterator(decltype(iter())(dim, nullptr)); }

// See same thing for SmallBase.
#define DEF_ASSIGNOPS(OP)                                               \
    template <class X> View & operator OP (X && x) { ra::start(*this) OP x; return *this; }
#define DEF_VIEW_COMMON(RANK)                                           \
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)                          \
    /* Constructors using pointers need extra care */                   \
    /* constexpr */ /* triggers box/bug83.C in gcc 8.3 */ View(): p(nullptr) {} \
    View(Dimv const & dim_, T * p_): dim(dim_), p(p_) {} /* [ra36] */   \
    template <class SS>                                                 \
    View(SS && s, T * p_): p(p_)                                        \
    {                                                                   \
        if constexpr (std::is_convertible_v<value_t<SS>, Dim>) {        \
            ra::resize(View::dim, start(s).size(0)); /* [ra37] */       \
            start(View::dim) = s;                                       \
        } else {                                                        \
            auto w = map([](auto && s) { return Dim { ra::dim_t(s), 0 }; }, s); \
            ra::resize(View::dim, w.size(0)); /* [ra37] */              \
            start(View::dim) = w;                                       \
            filldim(View::dim.size(), View::dim.end());                 \
        }                                                               \
    }                                                                   \
    View(std::initializer_list<dim_t> s, T * p_): View(start(s), p_) {} \
    /* lack of these causes runtime bug [ra38] FIXME why? */            \
    View(View && x) = default;                                          \
    View(View const & x) = default;                                     \
    /* declaring View(View &&) deletes this, so we need to repeat it [ra34] */ \
    View & operator=(View const & x)                                    \
    {                                                                   \
        ra::start(*this) = x;                                           \
        return *this;                                                   \
    }                                                                   \
    /* array type is not deduced by (X &&) */                           \
    View & operator=(typename nested_braces<T, RANK>::list x)           \
    {                                                                   \
        ra::iter<-1>(*this) = x;                                        \
        return *this;                                                   \
    }                                                                   \
    /* braces row-major ravel for rank!=1 */                            \
    using ravel_arg = std::conditional_t<RANK==1, no_arg, std::initializer_list<T>>; \
    View & operator=(ravel_arg const x)                                 \
    {                                                                   \
        CHECK_BOUNDS(p && this->size()==ra::dim_t(x.size()) && "bad assignment"); \
        std::copy(x.begin(), x.end(), this->begin());                   \
        return *this;                                                   \
    }                                                                   \
    bool const empty() const { return 0==size(); } /* TODO Optimize */

inline dim_t select(Dim * dim, Dim const * dim_src, dim_t i)
{
    CHECK_BOUNDS(inside(i, dim_src->size));
    return dim_src->stride*i;
}
template <class II>
inline dim_t select(Dim * dim, Dim const * dim_src, ra::Iota<II> i)
{
    CHECK_BOUNDS((inside(i.i_, dim_src->size) && inside(i.i_+(i.size_-1)*i.stride_, dim_src->size))
                 || (i.size_==0 && i.i_<=dim_src->size));
    dim->size = i.size_;
    dim->stride = dim_src->stride * i.stride_;
    return dim_src->stride*i.i_;
}
template <class I0, class ... I>
inline dim_t select_loop(Dim * dim, Dim const * dim_src, I0 && i0, I && ... i)
{
    return select(dim, dim_src, std::forward<I0>(i0))
        + select_loop(dim+is_beatable<I0>::skip, dim_src+is_beatable<I0>::skip_src, std::forward<I>(i) ...);
}
template <int n, class ... I>
inline dim_t select_loop(Dim * dim, Dim const * dim_src, dots_t<n> dots, I && ... i)
{
    for (Dim * end = dim+n; dim!=end; ++dim, ++dim_src) {
        *dim = *dim_src;
    }
    return select_loop(dim, dim_src, std::forward<I>(i) ...);
}
template <int n, class ... I>
inline dim_t select_loop(Dim * dim, Dim const * dim_src, insert_t<n> insert, I && ... i)
{
    for (Dim * end = dim+n; dim!=end; ++dim) {
        dim->size = DIM_BAD;
        dim->stride = 0;
    }
    return select_loop(dim, dim_src, std::forward<I>(i) ...);
}
inline dim_t select_loop(Dim * dim, Dim const * dim_src)
{
    return 0;
}

// allow conversion only from T to T const with null type.
template <class T> using const_atom = std::conditional_t<std::is_const_v<T>, no_arg, T const>;


// --------------------
// View for fixed rank
// --------------------

// TODO Parameterize on Child having .data() so that there's only one pointer.
// TODO A constructor, if only for CHECK_BOUNDS (positive sizes, strides inside, etc.)
template <class T, rank_t RANK>
struct View
{
    using Dimv = Small<Dim, RANK>;

    Dimv dim;
    T * p;

    constexpr static rank_t rank_s() { return RANK; };
    constexpr static rank_t rank() { return RANK; }
    constexpr static dim_t size_s(int j) { return DIM_ANY; }
    constexpr dim_t size() const { return proddim(dim.begin(), dim.end()); }
    constexpr dim_t size(int j) const { return dim[j].size; }
    constexpr dim_t stride(int j) const { return dim[j].stride; }
    constexpr auto data() { return p; }
    constexpr auto data() const { return p; }

    // Specialize for rank() integer-args -> scalar, same in ra::SmallBase in small.H.
#define SUBSCRIPTS(CONST)                                               \
    template <class ... I,                                              \
              std::enable_if_t<((0 + ... + std::is_integral_v<std::decay_t<I>>)<RANK \
                                && (0 + ... + is_beatable<I>::value)==sizeof...(I)), int> = 0> \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        constexpr rank_t extended = (0 + ... + (is_beatable<I>::skip-is_beatable<I>::skip_src)); \
        constexpr rank_t subrank = rank_sum(RANK, extended);            \
        static_assert(subrank>=0, "bad subrank");                       \
        View<T CONST, subrank> sub;                                     \
        sub.p = data() + select_loop(sub.dim.data(), this->dim.data(), i ...); \
        /* fill the rest of dim, skipping over beatable subscripts */   \
        for (int i=(0 + ... + is_beatable<I>::skip); i<subrank; ++i) {  \
            sub.dim[i] = this->dim[i-extended];                         \
        }                                                               \
        return sub;                                                     \
    }                                                                   \
    /* BUG doesn't handle generic zero-rank indices */                  \
    template <class ... I, std::enable_if_t<(0 + ... + std::is_integral_v<I>)==RANK, int> = 0> \
    decltype(auto) operator()(I const & ... i) CONST                    \
    {                                                                   \
        return data()[select_loop(nullptr, this->dim.data(), i ...)];   \
    }                                                                   \
    /* TODO > 1 selector... This still covers (unbeatable, integer) for example, which could be reduced. */ \
    template <class ... I, std::enable_if_t<!(is_beatable<I>::value && ...), int> = 0> \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
    /* FIXME should this also take foreign vector? See big-0.C */       \
    template <class  I>                                                 \
    auto at(I && i) CONST                                               \
    {                                                                   \
        constexpr rank_t subrank = rank_diff(RANK, ra::size_s<I>());  /* gcc accepts i.size() */ \
        using Sub = View<T CONST, subrank>;                             \
        if constexpr (subrank==RANK_ANY) {                              \
            return Sub { typename Sub::Dimv(dim.begin()+i.size(0), dim.end()),  /* Dimv is std::vector */ \
                    data() + Indexer1::index_p(dim, i) };               \
        } else {                                                        \
            return Sub { typename Sub::Dimv(ptr(dim.begin()+ra::size(i))),  /* Dimv is ra::Small */ \
                    data() + Indexer1::index_p(dim, i) };               \
        }                                                               \
    }                                                                   \
    decltype(auto) operator[](dim_t const i) CONST                      \
    {                                                                   \
        return (*this)(i);                                              \
    }
    FOR_EACH(SUBSCRIPTS, /*const*/, const)
#undef SUBSCRIPTS

    DEF_ITERATORS(RANK)
    DEF_VIEW_COMMON(RANK)

// implicit conversions from var rank. The guards are needed against ambiguity in ra-viewconst branch.
    template <rank_t R, std::enable_if_t<R==RANK_ANY && R!=RANK, int> =0>
    View(View<T const, R> const & x): dim(x.dim), p(x.p) {}
    template <rank_t R, std::enable_if_t<R==RANK_ANY && R!=RANK, int> =0>
    View(View<std::remove_const_t<T>, R> const & x): dim(x.dim), p(x.p) {}

// conversion to const from non const
    operator View<const_atom<T>, RANK> const & () const
    {
        return *reinterpret_cast<View<const_atom<T>, RANK> const *>(this);
    }

// conversions to scalar. The static asserts fail due to a bug in gcc 8.2 [ra43]. Recheck on gcc 9.
    operator T & () { /* static_assert(RANK==0, "bad rank"); */ assert(RANK==0); return data()[0]; }
    operator T const & () const { /* static_assert(RANK==0, "bad rank"); */ assert(RANK==0); return data()[0]; }
};

template <class T, rank_t RANK>
struct ra_traits_def<View<T, RANK>>
{
    using V = View<T, RANK>;
    using value_type = T;

    static decltype(auto) shape(V const & v) { return ra::shape(v.iter()); }
    static dim_t size(V const & v) { return ra::size(v); }
    constexpr static rank_t rank(V const & v) { return v.rank(); }
    constexpr static rank_t rank_s() { return RANK; };
    constexpr static dim_t size_s() { return RANK==0 ? 1 : DIM_ANY; }
};


// --------------------
// View for var rank
// --------------------

template <class T>
struct View<T, RANK_ANY>
{
    using Dimv = std::vector<Dim>; // maybe use Unique<Dim, 1> here.

    Dimv dim;
    T * p;

    constexpr static rank_t rank_s() { return RANK_ANY; }
    constexpr rank_t rank() const { return rank_t(dim.size()); }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr dim_t size() const { return proddim(dim.begin(), dim.end()); }
    constexpr dim_t size(int const j) const { return dim[j].size; }
    constexpr dim_t stride(int const j) const { return dim[j].stride; }
    constexpr auto data() { return p; }
    constexpr auto data() const { return p; }

// Contrary to RANK!=RANK_ANY, the scalar case cannot be separated at compile time. So operator() will return a rank 0 view in that case (and rely on conversion if, say, this ends up assigned to a scalar).
#define SUBSCRIPTS(CONST)                                               \
    template <class ... I, std::enable_if_t<(is_beatable<I>::value && ...), int> = 0> \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        constexpr rank_t extended = (0 + ... + (is_beatable<I>::skip-is_beatable<I>::skip_src)); \
        assert(this->rank()+extended>=0);                               \
        View<T CONST, RANK_ANY> sub;                                    \
        sub.dim.resize(this->rank()+extended);                          \
        sub.p = data() + select_loop(sub.dim.data(), this->dim.data(), i ...); \
        for (int i=(0 + ... + is_beatable<I>::skip); i<sub.rank(); ++i) { \
            sub.dim[i] = this->dim[i-extended];                         \
        }                                                               \
        return sub;                                                     \
    }                                                                   \
    /* TODO More than one selector... */                                \
    template <class ... I, std::enable_if_t<!(is_beatable<I>::value && ...), int> = 0> \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
    template <class I>                                                  \
    auto at(I && i) CONST                                               \
    {                                                                   \
        return View<T CONST, RANK_ANY> { Dimv(dim.begin()+i.size(), dim.end()), /* Dimv is std::vector */ \
                data() + Indexer1::index_p(dim, i) };                   \
    }                                                                   \
    constexpr T CONST & operator[](dim_t const i) CONST                 \
    {                                                                   \
        CHECK_BOUNDS(rank()==1 && "bad rank");                          \
        CHECK_BOUNDS(inside(i, dim[0].size));                           \
        return data()[i*dim[0].stride];                                 \
    }
    FOR_EACH(SUBSCRIPTS, /*const*/, const)
#undef SUBSCRIPTS

    DEF_ITERATORS(RANK_ANY)
    DEF_VIEW_COMMON(RANK_ANY)

    // conversions from fixed rank
    template <rank_t R, std::enable_if_t<R!=RANK_ANY, int> =0>
    View(View<T const, R> const & x): dim(x.dim.begin(), x.dim.end()), p(x.p) {}
    template <rank_t R, std::enable_if_t<R!=RANK_ANY, int> =0>
    View(View<std::remove_const_t<T>, R> const & x): dim(x.dim.begin(), x.dim.end()), p(x.p) {}

// conversion to const from non const
    operator View<const_atom<T>> const & ()
    {
        return *reinterpret_cast<View<const_atom<T>> const *>(this);
    }

// conversions to scalar
    operator T & () { assert(rank()==0); return data()[0]; };
    operator T const & () const { assert(rank()==0); return data()[0]; };
};

#undef DEF_ITERATORS
#undef DEF_VIEW_COMMON
#undef DEF_ASSIGNOPS

template <class T>
struct ra_traits_def<View<T, RANK_ANY>>
{
    using V = View<T, RANK_ANY>;
    using value_type = T;

    static decltype(auto) shape(V const & v) { return ra::shape(v.iter()); }
    static dim_t size(V const & v) { return ra::size(v); }
    static rank_t rank(V const & v) { return v.rank(); }
    constexpr static rank_t rank_s() { return RANK_ANY; };
    constexpr static dim_t size_s() { return DIM_ANY; }
};


// --------------------
// Container types
// --------------------

template <class V> struct storage_traits
{
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    static V create(dim_t n) { CHECK_BOUNDS(n>=0); return V(new T[n]); }
    static T const * data(V const & v) { return v.get(); }
    static T * data(V & v) { return v.get(); }
};
template <class T_, class A> struct storage_traits<std::vector<T_, A>>
{
    using T = T_;
    static std::vector<T, A> create(dim_t n) { return std::vector<T, A>(n); }
    static T const * data(std::vector<T, A> const & v) { return v.data(); } // BUG not if T = bool
    static T * data(std::vector<T, A> & v) { return v.data(); }
};

template <class T, rank_t RANK> inline
bool is_c_order(View<T, RANK> const & a)
{
    dim_t s = 1;
    for (int i=a.rank()-1; i>=0; --i) {
        if (s!=a.stride(i)) {
            return false;
        }
        s *= a.size(i);
        if (s==0) {
            return true;
        }
    }
    return true;
}

// TODO be convertible to View only, so that View::p is not duplicated
template <class Store, rank_t RANK>
struct Container: public View<typename storage_traits<Store>::T, RANK>
{
    Store store;
    using T = typename storage_traits<Store>::T;
    using View = ra::View<T, RANK>;
    using View::rank_s;

    using shape_arg = typename decltype(std::declval<View>().iter())::shape_type;

    View & view() { return *this; }
    View const & view() const { return *this; }

    template <class ... A> decltype(auto) operator()(A && ... a) { return View::operator()(std::forward<A>(a) ...); }
    template <class ... A> decltype(auto) operator()(A && ... a) const { return View::operator()(std::forward<A>(a) ...); }

// TODO Explicit definitions are needed only to have View::p set. Remove the duplication as in SmallBase/SmallArray, then remove these, both the constructors and the operator=s.
    Container(Container && w): store(std::move(w.store))
    {
        View::dim = std::move(w.dim);
        View::p = storage_traits<Store>::data(store);
    }
    Container(Container const & w): store(w.store)
    {
        View::dim = w.dim;
        View::p = storage_traits<Store>::data(store);
    }
    Container(Container & w): store(w.store)
    {
        View::dim = w.dim;
        View::p = storage_traits<Store>::data(store);
    }

// Override View::operator= to allow initialization-of-reference. Unfortunately operator>>(std::istream &, Container &) requires it. The presence of these operator= means that A(shape 2 3) = type-of-A [1 2 3] initializes so it doesn't behave as A(shape 2 3) = not-type-of-A [1 2 3] which will use View::operator= and frame match. See test/ownership.C [ra20].
// TODO do this through .set() op.
// TODO don't require copiable T from constructors, see fill1 below. That requires initialization and not update semantics for operator=.
    Container & operator=(Container && w)
    {
        store = std::move(w.store);
        View::dim = std::move(w.dim);
        View::p = storage_traits<Store>::data(store);
        return *this;
    }
    Container & operator=(Container const & w)
    {
        store = w.store;
        View::dim = w.dim;
        View::p = storage_traits<Store>::data(store);
        return *this;
    }
    Container & operator=(Container & w)
    {
        store = w.store;
        View::dim = w.dim;
        View::p = storage_traits<Store>::data(store);
        return *this;
    }

// Provided so that {} calls shape_arg constructor below.
    Container()
    {
        for (Dim & dimi: View::dim) { dimi = {0, 1}; } // 1 so we can push_back()
    }

    template <class S> void init(S && s)
    {
        auto w = map([](auto && s) { return Dim { ra::dim_t(s), 0 }; }, s);
        static_assert(1==decltype(w)::rank_s(), "rank mismatch for init shape"); // I decided not to accept rank extension here.
// [ra37] Need two parts because Dimv might be STL type. Otherwise I'd just View::dim.set(map(...)). Not being able to traverse backwards hurts here. See other uses of ra::vector (or start) here.
        if constexpr (RANK==RANK_ANY) {
            ra::resize(View::dim, w.size(0));
        }
        start(View::dim) = w;
        dim_t t = filldim(View::dim.size(), View::dim.end());
        store = storage_traits<Store>::create(t);
        View::p = storage_traits<Store>::data(store);
    }

// FIXME use of fill1 requires T to be copiable, this is unfortunate as it conflicts with the semantics of view_.operator=.
// store(x) avoids it for Big, but doesn't work for Unique. Should construct in place as std::vector does.
    template <class Pbegin> void fill1(dim_t xsize, Pbegin xbegin)
    {
        CHECK_BOUNDS(this->size()==xsize && "mismatched sizes");
        std::copy_n(xbegin, xsize, this->begin()); // TODO Use xpr traversal.
    }

// shape_arg overloads handle {...} arguments. Size check is courtesy of conversion (if shape_arg is Small) or init().

// explicit shape.
    Container(shape_arg const & s, none_t) { init(s); }
    template <class XX> Container(shape_arg const & s, XX && x): Container(s, none) { view() = x; }

// shape from data.
    template <class XX> Container(XX && x): Container(ra::shape(x), none) { view() = x; }
    Container(typename nested_braces<T, RANK>::list x)
    {
        static_assert(RANK!=RANK_ANY);
        std::array<dim_t, RANK> s;
        nested_braces<T, RANK>::template shape(x, s);
        init(s);
        view() = x;
    }

// braces row-major ravel for rank!=1
    Container(typename View::ravel_arg x)
        : Container({dim_t(x.size())}, none) { fill1(x.size(), x.begin()); }

// shape + row-major ravel. // TODO Maybe remove these? See also small.H.
    Container(shape_arg const & s, std::initializer_list<T> x)
        : Container(s, none) { fill1(x.size(), x.begin()); }
    template <class TT>
    Container(shape_arg const & s, TT * p)
        : Container(s, none) { fill1(this->size(), p); }
    template <class P>
    Container(shape_arg const & s, P pbegin, P pend)
        : Container(s, none) { fill1(this->size(), pbegin); }

// these are needed when shape_arg is std::vector, since that doesn't handle conversions like Small does.
    template <class SS> Container(SS && s, none_t) { init(s); }
    template <class SS, class XX> Container(SS && s, XX && x): Container(s, none) { view() = x; }
    template <class SS> Container(SS const & s, std::initializer_list<T> x)
        : Container(s, none) { fill1(x.size(), x.begin()); }

    using View::operator=;

// only for some kinds of store.
    void resize(dim_t const s)
    {
        static_assert(RANK==RANK_ANY || RANK>0); CHECK_BOUNDS(this->rank()>0);
        store.resize(proddim(View::dim.begin()+1, View::dim.end())*s);
        View::dim[0].size = s;
        View::p = store.data();
    }
    void resize(dim_t const s, T const & t)
    {
        static_assert(RANK==RANK_ANY || RANK>0); CHECK_BOUNDS(this->rank()>0);
        store.resize(proddim(View::dim.begin()+1, View::dim.end())*s, t);
        View::dim[0].size = s;
        View::p = store.data();
    }
// lets us move. A template + std::forward wouldn't work for push_back(brace-enclosed-list).
    void push_back(T && t)
    {
        static_assert(RANK==1 || RANK==RANK_ANY); CHECK_BOUNDS(this->rank()==1);
        store.push_back(std::move(t));
        ++View::dim[0].size;
        View::p = store.data();
    }
    void push_back(T const & t)
    {
        static_assert(RANK==1 || RANK==RANK_ANY); CHECK_BOUNDS(this->rank()==1);
        store.push_back(t);
        ++View::dim[0].size;
        View::p = store.data();
    }
    template <class ... A>
    void emplace_back(A && ... a)
    {
        static_assert(RANK==1 || RANK==RANK_ANY); CHECK_BOUNDS(this->rank()==1);
        store.emplace_back(std::forward<A>(a) ...);
        ++View::dim[0].size;
        View::p = store.data();
    }
    void pop_back()
    {
        static_assert(RANK==1 || RANK==RANK_ANY); CHECK_BOUNDS(this->rank()==1);
        CHECK_BOUNDS(View::dim[0].size>0);
        store.pop_back();
        --View::dim[0].size;
    }
    bool empty() const
    {
        return this->size()==0;
    }
    T const & back() const { CHECK_BOUNDS(this->rank()==1 && this->size()>0); return store[this->size()-1]; }
    T & back() { CHECK_BOUNDS(this->rank()==1 && this->size()>0); return store[this->size()-1]; }

// Container is always compact/row-major. Then the 0-rank STL-like iterators can be raw pointers. TODO But .iter() should also be able to benefit from this constraint, and the check should be faster for some cases (like RANK==1) or ellidable.

    auto begin() { assert(is_c_order(*this)); return this->data(); }
    auto begin() const { assert(is_c_order(*this)); return this->data(); }
    auto end() { return this->data()+this->size(); }
    auto end() const { return this->data()+this->size(); }
};

template <class Store, rank_t RANK>
struct ra_traits_def<Container<Store, RANK>>
    : public ra_traits_def<View<typename Container<Store, RANK>::T, RANK>> {};

template <class Store, rank_t RANK>
void swap(Container<Store, RANK> & a, Container<Store, RANK> & b)
{
    std::swap(a.dim, b.dim);
    std::swap(a.store, b.store);
    std::swap(a.p, b.p);
}

// Default storage for Big - see https://stackoverflow.com/a/21028912
// Allocator adaptor that interposes construct() calls to convert value initialization into default initialization.
template <typename T, typename A=std::allocator<T>>
struct default_init_allocator: public A
{
    using a_t = std::allocator_traits<A>;
    using A::A;

    template <typename U>
    struct rebind
    {
        using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
    };

    template <typename U>
    void construct(U * ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
    {
        ::new(static_cast<void *>(ptr)) U;
    }
    template <typename U, typename... Args>
    void construct(U * ptr, Args &&... args)
    {
        a_t::construct(static_cast<A &>(*this), ptr, std::forward<Args>(args)...);
    }
};

// Beyond this, we probably should have fixed-size (~std::dynarray), resizeable (~std::vector).
template <class T, rank_t RANK=RANK_ANY> using Big = Container<std::vector<T, default_init_allocator<T>>, RANK>;
template <class T, rank_t RANK=RANK_ANY> using Unique = Container<std::unique_ptr<T []>, RANK>;
template <class T, rank_t RANK=RANK_ANY> using Shared = Container<std::shared_ptr<T>, RANK>;

// -------------
// Used in the Guile wrappers to allow an array parameter to either borrow from Guile
// storage or convert into a new array (e.g. passing 'f32 into 'f64).
// TODO Can use unique_ptr's deleter for this?
// TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args
// -------------

struct NullDeleter { template <class T> void operator()(T * p) {} };
struct Deleter { template <class T> void operator()(T * p) { delete[] p; } };

template <rank_t RANK, class T>
Shared<T, RANK> shared_borrowing(View<T, RANK> & raw)
{
    Shared<T, RANK> a;
    a.dim = raw.dim;
    a.p = raw.p;
    a.store = std::shared_ptr<T>(raw.data(), NullDeleter());
    return a;
}

} // namespace ra

#undef CHECK_BOUNDS
