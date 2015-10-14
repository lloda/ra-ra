
// (c) Daniel Llorens - 2013-2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_LARGE_H
#define RA_LARGE_H

/// @file ra-large.H
/// @brief Variable size arrays.

#include "ra/ra-small.H"
#include "ra/ra-iterator.H"
#include "ra/ra-wrank.H"
#include <memory>

#ifdef EK_CHECK_BOUNDS
    #define EK_CHECK_BOUNDS_RA_LARGE EK_CHECK_BOUNDS
#else
    #ifndef EK_CHECK_BOUNDS_RA_LARGE
        #define EK_CHECK_BOUNDS_RA_LARGE 1
    #endif
#endif
#if EK_CHECK_BOUNDS_RA_LARGE==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

// Always C order. If you need another, transpose this.
template <class S, class D>
dim_t filldim(S sbegin, S send, D dbegin, D dend)
{
    dim_t next = 1;
    while (send>sbegin) {
        --send;
        --dend;
        assert(*send>=0);
        (*dend).size = *send;
        (*dend).stride = next;
        next *= *send;
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
// @TODO size is immutable so that it can be kept with rank.

// Would do this with a base class, but I like Raw being a POD.
#define DEFINE_ITERATORS(RANK)                                          \
    template <rank_t c=0> using iterator = ra::ra_iterator<Raw<T, RANK>, c>; \
    template <rank_t c=0> using const_iterator = ra::ra_iterator<Raw<T const, RANK>, c>; \
    template <rank_t c=0> auto iter() { return iterator<c>(dim, p); }   \
    template <rank_t c=0> auto iter() const { return const_iterator<c>(dim, p); } \
    auto begin() { return stl_iterator<iterator<0>>(dim, p); }          \
    auto begin() const { return stl_iterator<const_iterator<0>>(dim, p); } \
    auto end() { return stl_iterator<iterator<0>> {}; }                 \
    auto end() const { return stl_iterator<const_iterator<0>> {}; }

// See same thing for SmallBase.
#define DEFINE_ASSIGNMENT_OP(OPNAME, OP)                                \
    template <class X> Raw & OPNAME(X && x)                             \
    {                                                                   \
        ply_either(expr([](T & dest, T const & src) { dest OP src; }, this->iter(), start(x))); \
        return *this;                                                   \
    }
#define DEFINE_ASSIGNMENT_OPS                                           \
    DEFINE_ASSIGNMENT_OP(operator=, =)                                  \
    DEFINE_ASSIGNMENT_OP(operator+=, +=)                                \
    DEFINE_ASSIGNMENT_OP(operator-=, -=)                                \
    DEFINE_ASSIGNMENT_OP(operator/=, /=)                                \
    DEFINE_ASSIGNMENT_OP(operator*=, *=)                                \
    /* These three are unsafe */                                        \
    Raw() {}                                                            \
    Raw(Dimv const & dim_, T * p_): dim(dim_), p(p_) {}                 \
    Raw(std::initializer_list<dim_t> const s, T * p_): p(p_)            \
    {                                                                   \
        ra::resize(Raw::dim, s.size());                                 \
        filldim(s.begin(), s.end(), Raw::dim.begin(), Raw::dim.end());  \
    }                                                                   \
    template <class SS> Raw(SS const & s, T * p_): p(p_)                \
    {                                                                   \
        ra::resize(Raw::dim, s.size());                                 \
        filldim(s.begin(), s.end(), Raw::dim.begin(), Raw::dim.end());  \
    }                                                                   \
    Raw(Raw && x): dim(std::move(x.dim)), p(x.p) {}                     \
    Raw(Raw const & x): dim(x.dim), p(x.p) {}                           \
    Raw & operator=(Raw && x)                                           \
    {                                                                   \
        ply_either(expr([](T & dest, T const & src) { dest = src; }, this->iter(), start(x))); \
        return *this;                                                   \
    }                                                                   \
    Raw & operator=(Raw const & x)                                      \
    {                                                                   \
        ply_either(expr([](T & dest, T const & src) { dest = src; }, this->iter(), start(x))); \
        return *this;                                                   \
    }                                                                   \
    /* Ignore rank, row-major fill. @TODO Use xpr traversal */          \
    Raw & operator=(std::initializer_list<T> const x)                   \
    {                                                                   \
        assert(p && this->size()==ra::dim_t(x.size()) && "bad assignment"); \
        std::copy(x.begin(), x.end(), this->begin());                   \
        return *this;                                                   \
    }                                                                   \
    bool const empty() { return size()==0; } /* @TODO Optimize */

inline bool inside(dim_t i, dim_t b) { return i>=0 && i<b; }

inline dim_t select(Dim * dim, Dim const * dim_src, dim_t i)
{
    CHECK_BOUNDS(inside(i, dim_src->size));
    return dim_src->stride*i;
}
inline dim_t select(Dim * dim, Dim const * dim_src, all_t i)
{
    *dim = *dim_src;
    return 0;
}
template <class II>
inline dim_t select(Dim * dim, Dim const * dim_src, ra::Iota<II> i)
{
    CHECK_BOUNDS((inside(i.org_, dim_src->size) && inside(i.org_+(i.size_-1)*i.stride_, dim_src->size))
                 || (i.size_==0 && i.org_<=dim_src->size));
    dim->size = i.size_;
    dim->stride = dim_src->stride * i.stride_;
    return dim_src->stride*i.org_;
}
template <class I0, class ... I>
inline dim_t select_loop(Dim * dim, Dim const * dim_src, I0 i0, I ... i)
{
    return select(dim, dim_src, i0)+select_loop(dim+bool(!std::is_integral<I0>::value), dim_src+1, i ...);
}
inline dim_t select_loop(Dim * dim, Dim const * dim_src)
{
    return 0;
}

// @TODO Parameterize on Child having .data() so that there's only one pointer.
template <class T, rank_t RANK>
struct Raw
{
    using Dimv = Small<Dim, RANK>;

    Dimv dim;
    T * p;
    constexpr static rank_t rank() { return RANK; }
    constexpr static rank_t rank_s() { return RANK; };
    dim_t size() const { return proddim(dim.begin(), dim.end()); }
    constexpr dim_t size(rank_t const j) const { return dim[j].size; }
    constexpr dim_t stride(rank_t const j) const { return dim[j].stride; }
    auto data() { return p; }
    auto data() const { return p; }

    DEFINE_ITERATORS(RANK)
    DEFINE_ASSIGNMENT_OPS

// Specialize for rank() integer-args -> scalar, same in ra::SmallBase in ra-small.H.
#define SUBSCRIPTS(CONST)                                               \
    template <class ... I,                                              \
              enableifc_<(mp::Count<std::is_integral<I> ...>::value<RANK \
                          && mp::Count<is_beatable<I> ...>::value==sizeof...(I)), int> = 0> \
    auto operator()(I ... i) CONST                                      \
    {                                                                   \
        constexpr int reduced = mp::Count<std::is_integral<I> ...>::value; \
        Raw<T CONST, RANK-reduced> sub;                                 \
        sub.p = data() + select_loop(sub.dim.data(), this->dim.data(), i ...); \
        for (int i=sizeof...(I)-reduced; i<sub.rank(); ++i) {           \
            sub.dim[i] = this->dim[i+reduced];                          \
        }                                                               \
        return sub;                                                     \
    } /* @BUG doesn't handle generic zero-rank indices */               \
    template <class ... I, enableifc_<mp::Count<std::is_integral<I> ...>::value==RANK, int> = 0> \
    decltype(auto) operator()(I ... i) CONST                            \
    {                                                                   \
        return data()[select_loop(nullptr, this->dim.data(), i ...)];   \
    } /* @TODO More than one selector... */                             \
    template <class ... I, enableif_<mp::Not<mp::And<is_beatable<I> ...> >, int> = 0> \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
    template <class I>                                                  \
    auto at(I const & i) CONST                                          \
    {                                                                   \
        using Sub = Raw<T CONST, RANK-I::size()>; /* gcc accepts i.size() */ \
        return Sub { typename Sub::Dimv(dim.begin()+i.size(), dim.end()), \
                data() + Indexer1::index_p(dim, i) };                   \
    }                                                                   \
    T CONST & operator[](dim_t const i) CONST                           \
    {                                                                   \
        static_assert(rank()==1, "bad rank");                           \
        CHECK_BOUNDS(i<dim[0].size && "out of bounds");                 \
        return data()[i*dim[0].stride];                                 \
    }
    SUBSCRIPTS(const)
    SUBSCRIPTS(/*const*/)
#undef SUBSCRIPTS

    operator T & () { static_assert(RANK==0, "bad rank"); return data()[0]; }
    operator T const & () const { static_assert(RANK==0, "bad rank"); return data()[0]; }
};

template <class T, rank_t RANK>
struct ra_traits_def<Raw<T, RANK>>
{
    using V = Raw<T, RANK>;
    using value_type = T;

    using shape_type = Small<dim_t, RANK>;
    static shape_type shape(V const & v)
    {
        rank_t const iend = v.rank();
        shape_type s(ra_traits<shape_type>::make(iend));
        for (rank_t i=0; i!=iend; ++i) {
            s[i] = v.dim[i].size;
        }
        return s;
    }

    static dim_t size(V const & v) { return v.size(); }
    constexpr static rank_t rank(V const & v) { return v.rank(); }
    constexpr static rank_t rank_s() { return RANK; };
    constexpr static dim_t size_s() { return RANK==0 ? 1 : DIM_ANY; }
};

// Storage types.

template <class V> struct storage_traits
{
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    static V create(dim_t n) { return V(new T[n]); }
    static T const * data(V const & v) { return v.get(); }
    static T * data(V & v) { return v.get(); }
};
template <class T> struct storage_traits<std::vector<T>>
{
// @BUG This uses T(), so would need a create(, T) ...
    static std::vector<T> create(dim_t n) { return std::vector<T>(n); }
// @BUG No such for std::vector<bool>
    static T const * data(std::vector<T> const & v) { return v.data(); }
    static T * data(std::vector<T> & v) { return v.data(); }
};

template <class T, int RANK>
bool is_c_order(Raw<T, RANK> const & a)
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

// @TODO be convertible to Raw only, so that Raw::p is not duplicated
template <class Ptr, rank_t RANK>
struct WithStorage: public Raw<std::decay_t<decltype(*storage_traits<Ptr>::data(std::declval<Ptr>()))>, RANK>
{
    Ptr store;
    using T = std::decay_t<decltype(*storage_traits<Ptr>::data(store))>;
    using Raw = ra::Raw<T, RANK>;

// @TODO Explicit definitions are needed only to have Raw::p set. Remove the duplication as in SmallBase/SmallArray, then remove these, both the constructors and the operator=s.
    WithStorage(WithStorage && w): store(std::move(w.store))
    {
        Raw::dim = std::move(w.dim);
        Raw::p = storage_traits<Ptr>::data(store);
    }
    WithStorage(WithStorage const & w): store(w.store)
    {
        Raw::dim = w.dim;
        Raw::p = storage_traits<Ptr>::data(store);
    }
    WithStorage(WithStorage & w): store(w.store)
    {
        Raw::dim = w.dim;
        Raw::p = storage_traits<Ptr>::data(store);
    }
// Override Raw::operator= to allow initialization-of-reference. Unfortunately operator>>(std::istream &, WithStorage &) requires it. The presence of these operator= means that A(shape 2 3) = type-of-A [1 2 3] initializes so it doesn't behave as A(shape 2 3) = not-type-of-A [1 2 3] which will use Raw::operator= and frame match. @TODO Document this in the tests. Likely not what I want.
    WithStorage & operator=(WithStorage && w)
    {
        store = std::move(w.store);
        Raw::dim = std::move(w.dim);
        Raw::p = storage_traits<Ptr>::data(store);
        return *this;
    }
    WithStorage & operator=(WithStorage const & w)
    {
        store = w.store;
        Raw::dim = w.dim;
        Raw::p = storage_traits<Ptr>::data(store);
        return *this;
    }
    WithStorage & operator=(WithStorage & w)
    {
        store = w.store;
        Raw::dim = w.dim;
        Raw::p = storage_traits<Ptr>::data(store);
        return *this;
    }

// Provided b/c want {} to call initializer_list constructor below.
// @BUG Don't see why I need to init p; bogus warning on test-ra-io.C.
    explicit WithStorage(init_not_t) { Raw::p = nullptr; }
    WithStorage()
    {
        for (Dim & dimi: Raw::dim) { dimi = {0, 1}; }
        Raw::p = nullptr;
    }

    template <class SS> void init(SS const & s)
    {
        constexpr dim_t sr = ra_traits<decltype(s)>::size_s();
        static_assert(sr==DIM_ANY || RANK==RANK_ANY || sr==RANK, "rank mismatch for init shape");
        ra::resize(Raw::dim, s.size());
        dim_t t = filldim(s.begin(), s.end(), Raw::dim.begin(), Raw::dim.end());
        store = storage_traits<Ptr>::create(t);
        Raw::p = storage_traits<Ptr>::data(store);
    }

    template <class SS> WithStorage(SS const & s, default_init_t) { init(s); }
    WithStorage(std::initializer_list<dim_t> const s, default_init_t) { init(s); }
    template <class SS, class XX> WithStorage(SS const & s, XX && x): WithStorage(s, default_init)
    {
        ply_either(expr([](T & dest, T const & src) { dest = src; }, Raw::iter(), start(x)));
    }
    template <class XX> WithStorage(std::initializer_list<dim_t> const s, XX && x): WithStorage(s, default_init)
    {
        ply_either(expr([](T & dest, T const & src) { dest = src; }, Raw::iter(), start(x)));
    }
    WithStorage(std::initializer_list<dim_t> const s, std::initializer_list<T> const x): WithStorage(s, default_init)
    {
        std::copy(x.begin(), x.end(), this->begin()); // @TODO Use xpr traversal.
    }
    template <class SS> WithStorage(SS const & s, std::initializer_list<T> const x): WithStorage(s, default_init)
    {
        std::copy(x.begin(), x.end(), this->begin()); // @TODO Use xpr traversal.
    }
    template <class XX> WithStorage(XX && x): WithStorage(start(x).shape(), default_init) // @TODO ahem...
    {
// @TODO resize in init() should be doing compile time checks as possible. We don't accept <=RANK b/c we need to define the size.
        static_assert(decltype(start(x))::rank_s()==RANK || decltype(start(x))::rank_s()==RANK_ANY || RANK==RANK_ANY,
                      "bad data");
        ply_either(expr([](T & dest, T const & src) { dest = src; }, Raw::iter(), start(x)));
    }
    WithStorage(std::initializer_list<T> const x): WithStorage({dim_t(x.size())}, default_init)
    {
        static_assert(RANK==RANK_ANY || RANK==1, "rank mismatch with init list (of rank 1)");
        std::copy(x.begin(), x.end(), this->begin());
    }
    template <class P>
    WithStorage(std::initializer_list<dim_t> const s, P pbegin, P pend): WithStorage(s, default_init)
    {
        std::copy(pbegin, pend, this->begin()); // @TODO Use xpr traversal.
    }
    using Raw::operator=;

// only for some kinds of store.
    void resize(dim_t const s)
    {
        assert(this->rank()>0);
        store.resize(proddim(Raw::dim.begin()+1, Raw::dim.end())*s /* std::default_init_t someday... */);
        Raw::dim[0].size = s;
        Raw::p = store.data();
    }
    void resize(dim_t const s, T const & t)
    {
        assert(this->rank()>0);
        store.resize(proddim(Raw::dim.begin()+1, Raw::dim.end())*s, t);
        Raw::dim[0].size = s;
        Raw::p = store.data();
    }
    void push_back(T const & t)
    {
        assert(this->rank()==1);
        store.push_back(t);
        ++Raw::dim[0].size;
        Raw::p = store.data();
    }
// lets us move. A template + std::forward would not work for push_back(brace-enclosed-list).
    void push_back(T && t)
    {
        assert(this->rank()==1);
        store.push_back(std::move(t));
        ++Raw::dim[0].size;
        Raw::p = store.data();
    }
    void pop_back()
    {
        assert(this->rank()==1 && Raw::dim[0].size>0);
        store.pop_back();
        --Raw::dim[0].size;
    }
    template <class ... A>
    void emplace_back(A && ... a)
    {
        assert(this->rank()==1);
        store.emplace_back(std::forward<A>(a) ...);
        ++Raw::dim[0].size;
        Raw::p = store.data();
    }
    bool empty() const
    {
        return this->size()==0;
    }
    T const & back() const { assert(this->rank()==1 && "bad rank for back"); return store[this->size()-1]; }
    T & back() { assert(this->rank()==1 && "bad rank for back"); return store[this->size()-1]; }

// WithStorage is always compact/row-major. Then the 0-rank STL-like iterators can be raw pointers. @TODO But .iter() should also be able to benefit from this constraint, and the check should be faster for some cases (like RANK==1) or ellidable.

    auto begin() { assert(is_c_order(*this)); return this->data(); }
    auto begin() const { assert(is_c_order(*this)); return this->data(); }
    auto end() { return this->data()+this->size(); }
    auto end() const { return this->data()+this->size(); }
};

// Beyond this, we probably should have fixed-size (~std::dynarray), resizeable (~std::vector).
template <class T, rank_t RANK=RANK_ANY> using Unique = WithStorage<std::unique_ptr<T []>, RANK>;
template <class T, rank_t RANK=RANK_ANY> using Owned = WithStorage<std::vector<T>, RANK>;
template <class T, rank_t RANK=RANK_ANY> using Shared = WithStorage<std::shared_ptr<T>, RANK>;

// -------------
// Used in the Guile wrappers to allow an array parameter to either borrow from Guile
// storage or convert into a new array (e.g. passing 'f32 into 'f64).
// @TODO Can use unique_ptr's deleter for this?
// @TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args
// -------------

struct NullDeleter { template <class T> void operator()(T * p) {} };
struct Deleter { template <class T> void operator()(T * p) { delete[] p; } };

template <rank_t RANK, class T>
Shared<T, RANK> shared_borrowing(Raw<T, RANK> & raw)
{
    Shared<T, RANK> a(ra::init_not);
    a.dim = raw.dim;
    a.p = raw.p;
    a.store = std::shared_ptr<T>(raw.data(), NullDeleter());
    return a;
}

// @TODO Used to forbid; replace by a positive generic guarantee that V(a) works as values.
// @TODO Possibly in ra-type.H.
template <class V>
struct is_shared
{
    static bool const value = false;
};

template <class T, rank_t RANK>
struct is_shared<Shared<T, RANK> >
{
    static bool const value = true;
};

template <class T>
struct Raw<T, RANK_ANY>
{
    using Dimv = std::vector<Dim>; // maybe use Unique<Dim, 1> here.

    Dimv dim;
    T * p;
    rank_t rank() const { return rank_t(dim.size()); } // @CONVERSION
    dim_t size() const { return proddim(dim.begin(), dim.end()); }
    constexpr dim_t size(rank_t const j) const { return dim[j].size; }
    constexpr dim_t stride(rank_t const j) const { return dim[j].stride; }
    auto data() { return p; }
    auto data() const { return p; }

    DEFINE_ITERATORS(RANK_ANY)
    DEFINE_ASSIGNMENT_OPS

    template <rank_t SRANK> Raw(Raw<T, SRANK> const & x): dim(x.dim.begin(), x.dim.end()), p(x.p) {}

#define SUBSCRIPTS(CONST)                                               \
    template <class ... I>                                              \
    auto operator()(I ... i) CONST                                      \
    {                                                                   \
        constexpr int reduced = mp::Count<std::is_integral<I> ...>::value; \
        assert(this->rank()>=reduced);                                  \
        Raw<T CONST, RANK_ANY> sub;                                     \
        sub.dim.resize(this->rank()-reduced);                           \
        sub.p = data() + select_loop(sub.dim.data(), this->dim.data(), i ...); \
        for (int i=sizeof...(I)-reduced; i<sub.rank(); ++i) {           \
            sub.dim[i] = this->dim[i+reduced];                          \
        }                                                               \
        return sub;                                                     \
    }                                                                   \
    template <class I>                                                  \
    auto at(I const & i) CONST                                          \
    {                                                                   \
        return Raw<T CONST, RANK_ANY> { Dimv(dim.begin()+i.size(), dim.end()), \
                data() + Indexer1::index_p(dim, i) };                   \
    }                                                                   \
    T CONST & operator[](dim_t const i) CONST                           \
    {                                                                   \
        CHECK_BOUNDS(rank()==1 && "bad rank");                          \
        CHECK_BOUNDS(i<dim[0].size && "out of bounds");                 \
        return data()[i*dim[0].stride];                                 \
    }
    SUBSCRIPTS(const)
    SUBSCRIPTS(/*const*/)
#undef SUBSCRIPTS

    operator T & () { assert(rank()==0); return data()[0]; };
    operator T const & () const { assert(rank()==0); return data()[0]; };
};

#undef DEFINE_ITERATORS
#undef DEFINE_ASSIGNMENT_OPS

template <class T>
struct ra_traits_def<Raw<T, RANK_ANY>>
{
    using V = Raw<T, RANK_ANY>;
    using value_type = T;

    using shape_type = std::vector<dim_t>; // maybe use Unique<Dim, 1> here.
    static shape_type shape(V const & v)
    {
        rank_t const iend = v.rank();
        shape_type s(ra_traits<shape_type>::make(iend));
        for (rank_t i=0; i!=iend; ++i) {
            s[i] = v.dim[i].size;
        }
        return s;
    }

    static dim_t size(V const & v) { return v.size(); }
    static rank_t rank(V const & v) { return v.rank(); }
    constexpr static rank_t rank_s() { return RANK_ANY; };
    constexpr static rank_t size_s() { return DIM_ANY; }
};

template <class Ptr, rank_t RANK>
struct ra_traits_def<WithStorage<Ptr, RANK>>
    : public ra_traits_def<Raw<typename WithStorage<Ptr, RANK>::T, RANK>>
{
};

template <class T, int RANK>
Raw<T, RANK> reverse(Raw<T, RANK> const & raw, int k)
{
    Raw<T, RANK> r = raw;
    auto & dim = r.dim[k];
    if (dim.size!=0) {
        r.p += dim.stride*(dim.size-1);
        dim.stride *= -1;
    }
    return r;
}

#define TRANSPOSE_BODY                                                  \
    {                                                                   \
        auto rp = std::max_element(s.begin(), s.end());                 \
        assert(rp!=s.end() && "bad axes spec");                         \
        rank_t rr = *rp;                                                \
        assert((RANK==RANK_ANY || RANK==rr+1) && "bad output rank");    \
        Raw<T, RANK> r { ra_traits<decltype(r.dim)>::make(rr+1, Dim { DIM_BAD, 0 }), raw.data() }; \
        int k = 0;                                                      \
        for (int sk: s) {                                               \
            Dim & dest = r.dim[sk];                                     \
            dest.stride += raw.dim[k].stride;                           \
            dest.size = dest.size>=0 ? std::min(dest.size, raw.dim[k].size) : raw.dim[k].size; \
            ++k;                                                        \
        }                                                               \
        for (Dim const & dest: r.dim) {                                 \
            assert(dest.size!=DIM_BAD && "axes left unset");            \
        }                                                               \
        return r;                                                       \
    }

template <class T, int RANK, class S>
Raw<T, RANK> transpose(Raw<T, RANK> const & raw, S const & s) TRANSPOSE_BODY

template <class T, int RANK, class I>
Raw<T, RANK> transpose(Raw<T, RANK> const & raw, std::initializer_list<I> s) TRANSPOSE_BODY

template <class T, int RANK>
Raw<T, RANK> transpose(Raw<T, RANK> const & raw)
{
    static_assert(RANK==2 || RANK==RANK_ANY, "bad rank");
    assert(raw.rank()==2 && "bad rank");  // @TODO reduce to 'static where possible'.
    return transpose(raw, {1, 0});
}

// @TODO Merge with transpose(); transpose(A, {0, 0}) should work. Also from(A, ra::_0, ra::_0) should work.
template <class A, enableifc_<ra_traits<A>::size_s()==DIM_ANY, int> =0>
inline auto diag(A && a)
{
    assert(a.rank()==2 && a.size(0)==a.size(1) && "bad rank for diag");
    return Raw<typename std::remove_reference<decltype(*a.data())>::type, 1>
        ({{a.size(0), a.stride(0)+a.stride(1)}}, a.data());
}

// Make last sizes of Raw<> be compile-time constants.
template <class super_t, rank_t SUPERR, class T, int RANK>
auto explode_(Raw<T, RANK> const & a)
{
// @TODO Reduce to a single check, either the first or the second.
    static_assert(RANK>=SUPERR || RANK==RANK_ANY, "rank of a is too low");
    assert(a.rank()>=SUPERR && "rank of a is too low");
    Raw<super_t, rank_sum(RANK, -SUPERR)> b;
    ra::resize(b.dim, a.rank()-SUPERR);
    dim_t r = 1;
    for (int i=0; i<SUPERR; ++i) {
        r *= a.size(i+b.rank());
    }
    assert(r*sizeof(T)==sizeof(super_t) && "size of SUPERR axes doesn't match super type");
    for (int i=0; i<b.rank(); ++i) {
        assert(a.stride(i) % r==0 && "stride of SUPERR axes doesn't match super type");
        b.dim[i].stride = a.stride(i) / r;
        b.dim[i].size = a.size(i);
    }
    assert(b.rank()==0 || a.stride(b.rank()-1)==r && "super type is not compact in array");
    b.p = reinterpret_cast<super_t *>(a.data());
    return b;
}

template <class super_t, class T, int RANK>
auto explode(Raw<T, RANK> const & a)
{
    return explode_<super_t, ra_traits<super_t>::rank_s()>(a);
}

// Suporting rank>1 would require poking into the ranks of sub_t.
template <class sub_t, rank_t SUBR, class T, int RANK>
auto compress_(Raw<T, RANK> const & a)
{
    static_assert(RANK!=RANK_ANY, "var rank tbd");
    Raw<sub_t, RANK+SUBR> b;
    constexpr dim_t r = sizeof(T)/sizeof(sub_t);
    static_assert(sizeof(T)==r*sizeof(sub_t), "cannot make axis of T from sub type");
    for (int i=0; i<a.rank(); ++i) {
        b.dim[i].stride = a.stride(i) * r;
        b.dim[i].size = a.size(i);
    }
    static_assert(SUBR==1, "SUBR!=1 tbd");
    b.dim[a.rank()].stride = 1;
    b.dim[a.rank()].size = r;
    b.p = reinterpret_cast<sub_t *>(a.data());
    return b;
}

template <class sub_t, class T, int RANK>
auto compress(Raw<T, RANK> const & a)
{
    return compress_<sub_t, 1>(a);
}

// Checks for functions that require compact arrays (@todo they really shouldn't).
template <class A>
inline bool const crm(A const & a)
{
    return a.size()==0 || is_c_order(a);
}
template <class A, class Int>
inline bool const crm(A const & a, Int const n)
{
    switch (a.rank()) {
    case 1: return a.size(0)==n && a.stride(0)==1;
    case 2: return a.size(1)==n && a.stride(1)==1 && a.stride(0)==n;
    default: return false;
    }
}
template <class A, class Int, class Jnt>
inline bool const crm(A const & a, Int const nrow, Jnt const ncol)
{
    return a.size(0)==nrow && a.size(1)==ncol && a.stride(1)==1 && a.stride(0)==ncol;
}

} // namespace ra::

#undef CHECK_BOUNDS
#undef EK_CHECK_BOUNDS_RA_LARGE

#endif // RA_LARGE_H
