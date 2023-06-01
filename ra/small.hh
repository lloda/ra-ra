// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with static dimensions, cf big.hh.

// (c) Daniel Llorens - 2013-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"
#include "expr.hh"

namespace ra {

// Used by CellBig / CellSmall.
template <class C>
struct CellFlat
{
    C c;
    constexpr void operator+=(dim_t const s) { c.p += s; }
    constexpr C & operator*() { return c; }
};


// --------------------
// Helpers for slicing
// --------------------

template <class I>
struct is_beatable_def
{
    constexpr static bool value = std::is_integral_v<I>;
    constexpr static int skip_src = 1;
    constexpr static int skip = 0;
    constexpr static bool static_p = value; // can the beating be resolved statically?
};

template <class I> requires (is_iota<I>)
struct is_beatable_def<I>
{
    using T = decltype(I::i);
    constexpr static bool value = std::is_integral_v<T> && (DIM_BAD != I::len_s(0));
    constexpr static int skip_src = 1;
    constexpr static int skip = 1;
    constexpr static bool static_p = false; // FIXME see Iota with ct N, S
};

// FIXME have a 'filler' version (e.g. with default n = -1) or maybe a distinct type.
template <int n>
struct is_beatable_def<dots_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = n;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <int n>
struct is_beatable_def<insert_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = 0;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <class I> using is_beatable = is_beatable_def<std::decay_t<I>>;


// --------------------
// Develop indices for Small
// --------------------

namespace indexer0 {

    template <class lens, class steps, class P, rank_t end, rank_t k=0>
    constexpr dim_t index(P const & p)
    {
        if constexpr (k==end) {
            return 0;
        } else {
            RA_CHECK(inside(p[k], mp::ref<lens, k>::value));
            return (p[k] * mp::ref<steps, k>::value) + index<lens, steps, P, end, k+1>(p);
        }
    }

    template <class lens, class steps, class P>
    constexpr dim_t shorter(P const & p) // for Container::at().
    {
        static_assert(mp::len<lens> >= size_s<P>(), "Too many indices.");
        return index<lens, steps, P, size_s<P>()>(p);
    }

    template <class lens, class steps, class P>
    constexpr dim_t longer(P const & p) // for IteratorConcept::at().
    {
        if constexpr (RANK_ANY==size_s<P>()) {
            RA_CHECK(mp::len<lens> <= p.size(), "Too few indices.");
        } else {
            static_assert(mp::len<lens> <= size_s<P>(), "Too few indices.");
        }
        return index<lens, steps, P, mp::len<lens>>(p);
    }

} // namespace indexer0


// --------------------
// Small iterator
// --------------------
// TODO Refactor with CellBig / STLIterator

// V is always SmallBase<SmallView, ...>
template <class V, rank_t cellr_spec=0>
struct CellSmall
{
    static_assert(cellr_spec!=RANK_ANY && cellr_spec!=RANK_BAD, "bad cell rank");
    constexpr static rank_t fullr = ra::rank_s<V>();
    constexpr static rank_t cellr = dependent_cell_rank(fullr, cellr_spec);
    constexpr static rank_t framer = dependent_frame_rank(fullr, cellr_spec);
    static_assert(cellr>=0 || cellr==RANK_ANY, "bad cell rank");
    static_assert(framer>=0 || framer==RANK_ANY, "bad frame rank");
    static_assert(fullr==cellr || gt_rank(fullr, cellr), "bad cell rank");

    using cell_lens = mp::drop<typename V::lens, framer>;
    using cell_steps = mp::drop<typename V::steps, framer>;
    using lens = mp::take<typename V::lens, framer>; // these are steps on atom_type * p !!
    using steps = mp::take<typename V::steps, framer>;

    using shape_type = std::array<dim_t, framer>;
    using atom_type = typename V::value_type;
    using cell_type = SmallView<atom_type, cell_lens, cell_steps>;
    using value_type = std::conditional_t<0==cellr, atom_type, cell_type>;
    using frame_type = SmallView<int, lens, steps>; // only to compute slens

    cell_type c;

    constexpr CellSmall(CellSmall const & ci): c { ci.c.p } {}
// see STLIterator for the case of s_[0]=0, etc. [ra12].
    constexpr CellSmall(atom_type * p_): c { p_ } {}
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr static rank_t rank_s() { return framer; }
    constexpr static rank_t rank() { return framer; }
    constexpr static dim_t len_s(int k) { RA_CHECK(inside(k, rank_s())); return V::len(k); }
    constexpr static dim_t len(int k) { RA_CHECK(inside(k, rank())); return V::len(k); }
    constexpr static dim_t step(int k) { return k<rank() ? V::step(k) : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { c.p += (k<rank()) * step(k)*d; }

    constexpr auto
    flat() const
    {
        if constexpr (0==cellr) {
            return c.p;
        } else {
            return CellFlat<cell_type> { c };
        }
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        if constexpr (0==cellr) {
            return c.p[indexer0::longer<lens, steps>(i)];
        } else {
            return cell_type(c.p + indexer0::longer<lens, steps>(i));
        }
    }
};


// --------------------
// STLIterator for both CellSmall & CellBig
// FIXME make it work for any array iterator, as in ply_ravel, ply_index.
// --------------------

template <class S, class I, class P>
constexpr void
next_in_cube(rank_t const framer, S const & dimv, I & i, P & p)
{
    for (int k=framer-1; k>=0; --k) {
        ++i[k];
        if (i[k]<dimv[k].len) {
            p += dimv[k].step;
            return;
        } else {
            i[k] = 0;
            p -= dimv[k].step*(dimv[k].len-1);
        }
    }
    p = nullptr;
}

template <int k, class lens, class steps, class I, class P>
constexpr void
next_in_cube(I & i, P & p)
{
    if constexpr (k>=0) {
        ++i[k];
        if (i[k]<mp::ref<lens, k>::value) {
            p += mp::ref<steps, k>::value;
        } else {
            i[k] = 0;
            p -= mp::ref<steps, k>::value*(mp::ref<lens, k>::value-1);
            next_in_cube<k-1, lens, steps>(i, p);
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
    STLIterator(Iterator const & ii_)
        : ii(ii_),
// shape_type may be std::array or std::vector.
          i([&]()
            { if constexpr (DIM_ANY==size_s<shape_type>()) {
                    return shape_type(ii.rank(), 0);
                } else {
                    return shape_type {0};
                }
            }())
    {
// [ra12] Null p_ so begin()==end() for empty range. ply() uses lens so this doesn't matter.
        if (0==ra::size(ii)) {
            ii.c.p = nullptr;
        }
    };

    template <class PP> bool operator==(PP const & j) const { return ii.c.p==j.ii.c.p; }
    template <class PP> bool operator!=(PP const & j) const { return ii.c.p!=j.ii.c.p; }

    decltype(auto) operator*() const { if constexpr (0==Iterator::cellr) return *ii.c.p; else return ii.c; }
    decltype(auto) operator*() { if constexpr (0==Iterator::cellr) return *ii.c.p; else return ii.c; }
    STLIterator & operator++()
    {
        if constexpr (0==Iterator::rank_s()) { // when rank==0, DIM_ANY check isn't enough
            ii.c.p = nullptr;
        } else if constexpr (DIM_ANY != ra::size_s<Iterator>()) {
            next_in_cube<Iterator::rank()-1, typename Iterator::lens, typename Iterator::steps>(i, ii.c.p);
        } else {
            next_in_cube(ii.rank(), ii.dimv, i, ii.c.p);
        }
        return *this;
    }
};

template <class T> STLIterator<T> stl_iterator(T && t) { return STLIterator<T>(std::forward<T>(t)); }


// --------------------
// Base for both small view & container
// --------------------

template <class lens_, class steps_, class ... I>
struct FilterDims
{
    using lens = lens_;
    using steps = steps_;
};

template <class lens_, class steps_, class I0, class ... I>
struct FilterDims<lens_, steps_, I0, I ...>
{
    constexpr static int s = is_beatable<I0>::skip;
    constexpr static int s_src = is_beatable<I0>::skip_src;
    using next = FilterDims<mp::drop<lens_, s_src>, mp::drop<steps_, s_src>, I ...>;
    using lens = mp::append<mp::take<lens_, s>, typename next::lens>;
    using steps = mp::append<mp::take<steps_, s>, typename next::steps>;
};

template <dim_t len0, dim_t step0>
constexpr dim_t
select(dim_t i0)
{
    RA_CHECK(inside(i0, len0));
    return i0*step0;
};

template <dim_t len0, dim_t step0, int n>
constexpr dim_t
select(dots_t<n> i0)
{
    return 0;
}

template <class lens, class steps>
constexpr dim_t
select_loop()
{
    return 0;
}

template <class lens, class steps, class I0, class ... I>
constexpr dim_t
select_loop(I0 i0, I ... i)
{
    constexpr int s_src = is_beatable<I0>::skip_src;
    return select<mp::first<lens>::value, mp::first<steps>::value>(i0)
        + select_loop<mp::drop<lens, s_src>, mp::drop<steps, s_src>>(i ...);
}

template <template <class ...> class Child_, class T, class lens_, class steps_>
struct SmallBase
{
    using lens = lens_;
    using steps = steps_;
    using value_type = T;

    template <class TT> using BadDimension = mp::int_c<(TT::value<0 || TT::value==DIM_ANY || TT::value==DIM_BAD)>;
    static_assert(!mp::apply<mp::orb, mp::map<BadDimension, lens>>::value, "Negative dimensions.");
    static_assert(mp::len<lens> == mp::len<steps>, "Mismatched lengths & steps."); // TODO static check on steps.

    using Child = Child_<T, lens, steps>;

    constexpr static rank_t rank() { return mp::len<lens>; }
    constexpr static rank_t rank_s() { return mp::len<lens>; }
    constexpr static dim_t size() { return mp::apply<mp::prod, lens>::value; }
    constexpr static dim_t size_s() { return size(); }
    constexpr static auto slens = mp::tuple_values<std::array<dim_t, rank()>, lens>();
    constexpr static auto ssteps = mp::tuple_values<std::array<dim_t, rank()>, steps>();
    constexpr static dim_t len(int k) { return slens[k]; }
    constexpr static dim_t len_s(int k) { return slens[k]; }
    constexpr static dim_t step(int k) { return ssteps[k]; }
    constexpr static auto shape() { return SmallView<ra::dim_t const, mp::int_list<rank_s()>, mp::int_list<1>>(slens.data()); }

// allowing rank 1 for coord types
    constexpr static bool convertible_to_scalar = size()==1; // rank()==0 || (rank()==1 && size()==1);

    constexpr T * data() { return static_cast<Child &>(*this).p; }
    constexpr T const * data() const { return static_cast<Child const &>(*this).p; }

// Specialize for rank() integer-args -> scalar, same in ra::View in big.hh.
#define RA_CONST_OR_NOT(CONST)                                          \
    template <class ... I>                                              \
    requires ((0 + ... + std::is_integral_v<I>)<rank() && (is_beatable<I>::static_p && ...)) \
    constexpr auto                                                      \
    operator()(I ... i) CONST                                           \
    {                                                                   \
        using FD = FilterDims<lens, steps, I ...>;                      \
        return SmallView<T CONST, typename FD::lens, typename FD::steps> \
            (data()+select_loop<lens, steps>(i ...));                   \
    }                                                                   \
    template <class ... I>                                              \
    requires ((0 + ... + std::is_integral_v<I>)==rank())                \
    constexpr decltype(auto)                                            \
    operator()(I ... i) CONST                                           \
    {                                                                   \
        return data()[select_loop<lens, steps>(i ...)];                 \
    } /* TODO More than one selector... */                              \
    template <class ... I>                                              \
    requires (!is_beatable<I>::static_p || ...)                         \
    constexpr auto \
    operator()(I && ... i) CONST                                        \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
    /* BUG I must be fixed size, otherwise we can't make out the output type. */ \
    template <class I>                                                  \
    constexpr auto                                                      \
    at(I const & i) CONST                                               \
    {                                                                   \
        return SmallView<T CONST, mp::drop<lens, ra::size_s<I>()>, mp::drop<steps, ra::size_s<I>()>> \
            (data()+indexer0::shorter<lens, steps>(i));                 \
    }                                                                   \
    template <class ... I>                                              \
    constexpr decltype(auto)                                            \
    operator[](I && ... i) CONST                                        \
    {                                                                   \
        return (*this)(std::forward<I>(i) ...);                         \
    }                                                                   \
    /* TODO would replace by s(ra::iota) if that could be made constexpr */ \
    template <int ss, int oo=0>                                         \
    constexpr auto                                                      \
    as() CONST                                                          \
    {                                                                   \
        static_assert(rank()>=1, "bad rank for as<>");                  \
        static_assert(ss>=0 && oo>=0 && ss+oo<=size(), "bad size for as<>"); \
        return SmallView<T CONST, mp::cons<mp::int_c<ss>, mp::drop1<lens>>, steps>(this->data()+oo*this->step(0)); \
    }                                                                   \
    /* BUG these make SmallArray<T, N> std::is_convertible to T even though conversion isn't possible bc of the assert */ \
    constexpr operator T CONST & () CONST requires (convertible_to_scalar) { return data()[0]; } \
    T CONST & back() CONST                                              \
    {                                                                   \
        static_assert(rank()==1 && size()>0, "back() is not available"); \
        return (*this)[size()-1];                                       \
    }
    FOR_EACH(RA_CONST_OR_NOT, /*const*/, const)
#undef RA_CONST_OR_NOT

// see same thing for View.
#define DEF_ASSIGNOPS(OP)                                               \
    template <class X>                                                  \
    requires (!mp::is_tuple_v<std::decay_t<X>>)                         \
    constexpr Child &                                                   \
    operator OP(X && x)                                                 \
    {                                                                   \
        ra::start(static_cast<Child &>(*this)) OP x;                    \
        return static_cast<Child &>(*this);                             \
    }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS

// braces don't match X &&
    constexpr Child &
    operator=(nested_arg<T, lens> const & x)
    {
        ra::iter<-1>(static_cast<Child &>(*this)) = mp::from_tuple<std::array<typename nested_tuple<T, lens>::sub, len(0)>>(x);
        return static_cast<Child &>(*this);
    }
// braces row-major ravel for rank!=1
    constexpr Child &
    operator=(ravel_arg<T, lens> const & x_)
    {
        auto x = mp::from_tuple<std::array<T, size()>>(x_);
        std::copy(x.begin(), x.end(), this->begin());
        return static_cast<Child &>(*this);
    }

    template <rank_t c=0> using iterator = ra::CellSmall<SmallBase<SmallView, T, lens, steps>, c>;
    template <rank_t c=0> using const_iterator = ra::CellSmall<SmallBase<SmallView, T const, lens, steps>, c>;
    template <rank_t c=0> constexpr iterator<c> iter() { return data(); }
    template <rank_t c=0> constexpr const_iterator<c> iter() const { return data(); }

// FIXME see if we need to extend this for cellr!=0.
// template <class P> using STLIterator = std::conditional_t<have_default_steps, P, STLIterator<Iterator<P>>>;
    constexpr static bool have_default_steps = std::same_as<steps, default_steps<lens>>;
    template <class I, class P> using pick_STLIterator = std::conditional_t<have_default_steps, P, ra::STLIterator<I>>;
    using STLIterator = pick_STLIterator<iterator<0>, T *>;
    using STLConstIterator = pick_STLIterator<const_iterator<0>, T const *>;

// TODO begin() end() may be different types for ranged for (https://en.cppreference.com/w/cpp/language/range-for), but not for stl algos like std::copy. That's unfortunate as it would allow simplifying end().
// TODO With default steps I can just return p. Make sure to test before changing this.
    constexpr STLIterator begin() { if constexpr (have_default_steps) return data(); else return iter(); }
    constexpr STLConstIterator begin() const { if constexpr (have_default_steps) return data(); else return iter(); }
    constexpr STLIterator end() { if constexpr (have_default_steps) return data()+size(); else return iterator<0>(nullptr); }
    constexpr STLConstIterator end() const { if constexpr (have_default_steps) return data()+size(); else return const_iterator<0>(nullptr); }
};


// ---------------------
// Small view & container
// ---------------------
// Strides are compile time, so we can put most members in the view type.

template <class T, class lens, class steps>
struct SmallView: public SmallBase<SmallView, T, lens, steps>
{
    using Base = SmallBase<SmallView, T, lens, steps>;
    using Base::rank, Base::size, Base::operator=;

    T * p;
    constexpr SmallView(T * p_): p(p_) {}
    constexpr SmallView(SmallView const & s): p(s.p) {}

    constexpr operator T & () { static_assert(Base::convertible_to_scalar); return p[0]; }
    constexpr operator T const & () const { static_assert(Base::convertible_to_scalar); return p[0]; };
};

#if defined (__clang__)
template <class T, int N> using extvector __attribute__((ext_vector_type(N))) = T;
#else
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;
#endif

template <class Z>
struct equal_to_t
{
    template <class ... T> constexpr static bool value = (std::is_same_v<Z, T> || ... || false);
};

template <class T, size_t N>
consteval size_t
align_req()
{
    if constexpr (equal_to_t<T>::template value<char, unsigned char,
                  short, unsigned short,
                  int, unsigned int,
                  long, unsigned long,
                  long long, unsigned long long,
                  float, double>
                  && 0<N && 0==(N & (N-1))) {
        return alignof(extvector<T, N>);
    } else {
        return alignof(T[N]);
    }
}

template <class T, class lens, class steps, class ... nested_args, class ... ravel_args>
struct
#if RA_DO_OPT_SMALLVECTOR==1
alignas(align_req<T, mp::apply<mp::prod, lens>::value>())
#else
#endif
SmallArray<T, lens, steps, std::tuple<nested_args ...>, std::tuple<ravel_args ...>>
    : public SmallBase<SmallArray, T, lens, steps>
{
    using Base = SmallBase<SmallArray, T, lens, steps>;
    using Base::rank, Base::size;

    T p[Base::size()]; // cf what std::array does for zero size; wish zero size just worked :-/

    constexpr SmallArray(): p() {}
// braces don't match (X &&)
    constexpr SmallArray(nested_args const & ... x): p()
    {
        static_cast<Base &>(*this) = nested_arg<T, lens> { x ... };
    }
// braces row-major ravel for rank!=1
    constexpr SmallArray(ravel_args const & ... x): p()
    {
        static_cast<Base &>(*this) = ravel_arg<T, lens> { x ... };
    }
// needed if T isn't registered as scalar [ra44]
    constexpr SmallArray(T const & t): p()
    {
        for (auto & x: p) { x = t; }
    }
// X && x makes this a better match than nested_args ... for 1 argument.
    template <class X>
    requires (!std::is_same_v<T, std::decay_t<X>> && !mp::is_tuple_v<std::decay_t<X>>)
    constexpr SmallArray(X && x): p()
    {
        static_cast<Base &>(*this) = x;
    }

    constexpr operator SmallView<T, lens, steps> () { return SmallView<T, lens, steps>(p); }
    constexpr operator SmallView<T const, lens, steps> const () { return SmallView<T const, lens, steps>(p); }
};

// FIXME unfortunately necessary. Try to remove the need, also of (S, begin, end) in Container, once the nested_tuple constructors work.
template <class A, class I, class J>
A ravel_from_iterators(I && begin, J && end)
{
    A a;
    std::copy(std::forward<I>(begin), std::forward<J>(end), a.begin());
    return a;
}


// ---------------------
// Builtin arrays
// ---------------------

template <class T, class I=mp::iota<std::rank_v<T>>>
struct builtin_array_lens;

template <class T, int ... I>
struct builtin_array_lens<T, mp::int_list<I ...>>
{
    using type = mp::int_list<std::extent_v<T, I> ...>;
};

template <class T> using builtin_array_lens_t = typename builtin_array_lens<T>::type;

template <class T>
struct builtin_array_types
{
    using A = std::remove_volatile_t<std::remove_reference_t<T>>; // preserve const
    using E = std::remove_all_extents_t<A>;
    using lens = builtin_array_lens_t<A>;
    using view = SmallView<E, lens>;
};

// forward declared in bootstrap.hh.
template <class T> requires (is_builtin_array<T>)
constexpr auto
start(T && t)
{
    using Z = builtin_array_types<T>;
    return typename Z::view((typename Z::E *)(t)).iter();
}

template <class T> requires (is_builtin_array<T>)
struct ra_traits_def<T>
{
    using S = typename builtin_array_types<T>::view;
    constexpr static decltype(auto) shape(T const & t) { return S::shape(); }
    constexpr static dim_t size(T const & t) { return S::size_s(); }
    constexpr static dim_t size_s() { return S::size_s(); }
    constexpr static rank_t rank(T const & t) { return S::rank(); }
    constexpr static rank_t rank_s() { return S::rank_s(); }
};


// --------------------
// Small ops; cf view-ops.hh.
// FIXME maybe there, or separate file.
// TODO See if this can be merged with Reframe (e.g. beat(reframe(a)) -> transpose(a) ?)
// --------------------

template <class A, class i>
struct axis_indices
{
    template <class T> using match_index = mp::int_c<(T::value==i::value)>;
    using I = mp::iota<mp::len<A>>;
    using type = mp::Filter_<mp::map<match_index, A>, I>;
// don't enforce, so allow dead axes (e.g. in transpose<1>(rank 1 array)).
    // static_assert((mp::len<type>)>0, "dst axis doesn't appear in transposed axes list");
};

template <class axes_list, class src_lens, class src_steps>
struct axes_list_indices
{
    static_assert(mp::len<axes_list> == mp::len<src_lens>, "Bad size for transposed axes list.");
    constexpr static int talmax = mp::fold<mp::max, void, axes_list>::value;
    constexpr static int talmin = mp::fold<mp::min, void, axes_list>::value;
    static_assert(talmin >= 0, "Bad index in transposed axes list.");
// don't enforce, so allow dead axes (e.g. in transpose<1>(rank 1 array)).
    // static_assert(talmax < mp::len<src_lens>, "bad index in transposed axes list");

    template <class dst_i> struct dst_indices_
    {
        using type = typename axis_indices<axes_list, dst_i>::type;
        template <class i> using lensi = mp::ref<src_lens, i::value>;
        template <class i> using stepsi = mp::ref<src_steps, i::value>;
        using step = mp::fold<mp::sum, void, mp::map<stepsi, type>>;
        using len = mp::fold<mp::min, void, mp::map<lensi, type>>;
    };

    template <class dst_i> using dst_indices = typename dst_indices_<dst_i>::type;
    template <class dst_i> using dst_len = typename dst_indices_<dst_i>::len;
    template <class dst_i> using dst_step = typename dst_indices_<dst_i>::step;

    using dst = mp::iota<(talmax>=0 ? (1+talmax) : 0)>;
    using type = mp::map<dst_indices, dst>;
    using lens =  mp::map<dst_len, dst>;
    using steps =  mp::map<dst_step, dst>;
};

#define DEF_TRANSPOSE(CONST)                                            \
    template <int ... Iarg, template <class ...> class Child, class T, class lens, class steps> \
    constexpr auto                                                      \
    transpose(SmallBase<Child, T, lens, steps> CONST & a)               \
    {                                                                   \
        using ti = axes_list_indices<mp::int_list<Iarg ...>, lens, steps>; \
        return SmallView<T CONST, typename ti::lens, typename ti::steps>(a.data()); \
    };                                                                  \
                                                                        \
    template <template <class ...> class Child, class T, class lens, class steps> \
    constexpr auto                                                      \
    diag(SmallBase<Child, T, lens, steps> CONST & a)                    \
    {                                                                   \
        return transpose<0, 0>(a);                                      \
    }
FOR_EACH(DEF_TRANSPOSE, /* const */, const)
#undef DEF_TRANSPOSE

// TODO Used by ProductRule; waiting for proper generalization.
template <template <class ...> class Child1, class T1, class lens1, class steps1,
          template <class ...> class Child2, class T2, class lens2, class steps2>
constexpr auto
cat(SmallBase<Child1, T1, lens1, steps1> const & a1, SmallBase<Child2, T2, lens2, steps2> const & a2)
{
    using A1 = SmallBase<Child1, T1, lens1, steps1>;
    using A2 = SmallBase<Child2, T2, lens2, steps2>;
    static_assert(A1::rank()==1 && A2::rank()==1, "Bad ranks for cat."); // gcc accepts a1.rank(), etc.
    using T = std::decay_t<decltype(a1[0])>;
    Small<T, A1::size()+A2::size()> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    std::copy(a2.begin(), a2.end(), val.begin()+a1.size());
    return val;
}

template <template <class ...> class Child1, class T1, class lens1, class steps1, class A2>
requires (is_scalar<A2>)
constexpr auto
cat(SmallBase<Child1, T1, lens1, steps1> const & a1, A2 const & a2)
{
    using A1 = SmallBase<Child1, T1, lens1, steps1>;
    static_assert(A1::rank()==1, "bad ranks for cat");
    using T = std::decay_t<decltype(a1[0])>;
    Small<T, A1::size()+1> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    val[a1.size()] = a2;
    return val;
}

template <class A1, template <class ...> class Child2, class T2, class lens2, class steps2>
requires (is_scalar<A1>)
constexpr auto
cat(A1 const & a1, SmallBase<Child2, T2, lens2, steps2> const & a2)
{
    using A2 = SmallBase<Child2, T2, lens2, steps2>;
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
        using type = mp::int_c<T::value / s>;
    };
    template <class T> using op = typename op_<T>::type;
};

// See view-ops.hh:explode, collapse. FIXME support real->complex, etc.
template <class super_t,
          template <class ...> class Child, class T, class lens, class steps>
constexpr auto
explode(SmallBase<Child, T, lens, steps> & a)
{
    using ta = SmallBase<Child, T, lens, steps>;
// the returned type has steps in super_t, but to support general steps we'd need steps in T. Maybe FIXME?
    static_assert(super_t::have_default_steps);
    constexpr rank_t ra = ta::rank_s();
    constexpr rank_t rb = super_t::rank_s();
    static_assert(std::is_same_v<mp::drop<lens, ra-rb>, typename super_t::lens>);
    static_assert(std::is_same_v<mp::drop<steps, ra-rb>, typename super_t::steps>);
    using csteps = mp::map<explode_divop<ra::size_s<super_t>()>::template op, mp::take<steps, ra-rb>>;
    return SmallView<super_t, mp::take<lens, ra-rb>, csteps>((super_t *) a.data());
}

} // namespace ra
