// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with static lengths/strides, cf big.hh.

// (c) Daniel Llorens - 2013-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"

namespace ra {


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
    using shape_type = decltype(ra::shape(std::declval<Iterator>()));

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
          i([&] {
              if constexpr (DIM_ANY==size_s<shape_type>()) {
                  return shape_type(ii.rank(), 0);
              } else {
                  return shape_type {0};
              }
          }())
    {
// [ra12] Null p_ so begin()==end() for empty range. ply() uses lens so this doesn't matter.
        if (0==ra::size(ii)) {
            ii.c.cp = nullptr;
        }
    };

    template <class PP> bool operator==(PP const & j) const { return ii.c.cp==j.ii.c.cp; }
    template <class PP> bool operator!=(PP const & j) const { return ii.c.cp!=j.ii.c.cp; }

    decltype(auto) operator*() const { if constexpr (0==Iterator::cellr) return *ii.c.cp; else return ii.c; }
    decltype(auto) operator*() { if constexpr (0==Iterator::cellr) return *ii.c.cp; else return ii.c; }
    STLIterator & operator++()
    {
        if constexpr (0==Iterator::rank_s()) { // when rank==0, DIM_ANY check isn't enough
            ii.c.cp = nullptr;
        } else if constexpr (DIM_ANY != ra::size_s<Iterator>()) {
            next_in_cube<Iterator::rank()-1, typename Iterator::lens, typename Iterator::steps>(i, ii.c.cp);
        } else {
            next_in_cube(ii.rank(), ii.dimv, i, ii.c.cp);
        }
        return *this;
    }
};


// --------------------
// Slicing helpers
// --------------------

// FIXME condition should be zero rank, maybe convertibility, not is_integral
template <class I> constexpr bool is_scalar_index = ra::is_zero_or_scalar<I>;

struct beatable_t
{
    bool rt, ct; // beatable at all and statically
    int src, dst, add; // axes on src, dst, and dst-src
};

template <class I> constexpr beatable_t beatable_def
    = { .rt=is_scalar_index<I>, .ct=is_scalar_index<I>, .src=1, .dst=0, .add=-1 };

template <int n> constexpr beatable_t beatable_def<dots_t<n>>
    = { .rt=true, .ct = true, .src=n, .dst=n, .add=0 };

template <int n> constexpr beatable_t beatable_def<insert_t<n>>
    = { .rt=true, .ct = true, .src=0, .dst=n, .add=n };

template <class I> requires (is_iota<I>) constexpr beatable_t beatable_def<I>
    = { .rt=(DIM_BAD!=I::nn), .ct=(is_constant<typename I::N> && is_constant<typename I::N>), .src=1, .dst=1, .add=0 };

template <class I> constexpr beatable_t beatable = beatable_def<std::decay_t<I>>;

template <int k, class V>
constexpr decltype(auto)
maybe_len(V && v)
{
    if constexpr (v.len_s(k)>=0) {
        return int_c<v.len(k)> {};
    } else {
        return v.len(k);
    }
}

template <class II, class KK=mp::iota<mp::len<II>>>
struct unbeat;

template <class ... I, int ... k>
struct unbeat<std::tuple<I ...>, mp::int_list<k ...>>
{
    template <class V>
    constexpr static decltype(auto)
    op(V & v, I && ... i)
    {
        return from(v, with_len(maybe_len<k>(v), std::forward<I>(i)) ...);
    }
};


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

// Used by CellBig / CellSmall.
template <class C>
struct CellFlat
{
    C c;
    constexpr void operator+=(dim_t const s) { c.cp += s; }
    constexpr C & operator*() { return c; }
};

// V is always SmallBase<SmallView, ...>
template <class V, rank_t cellr_spec=0>
struct CellSmall
{
    static_assert(cellr_spec!=RANK_ANY && cellr_spec!=RANK_BAD, "Bad cell rank.");
    constexpr static rank_t fullr = ra::rank_s<V>();
    constexpr static rank_t cellr = dependent_cell_rank(fullr, cellr_spec);
    constexpr static rank_t framer = dependent_frame_rank(fullr, cellr_spec);
    static_assert(cellr>=0 || cellr==RANK_ANY, "Bad cell rank.");
    static_assert(framer>=0 || framer==RANK_ANY, "Bad frame rank.");
    static_assert(choose_rank(fullr, cellr)==fullr, "Bad cell rank.");

    using cell_lens = mp::drop<typename V::lens, framer>;
    using cell_steps = mp::drop<typename V::steps, framer>;
    using lens = mp::take<typename V::lens, framer>; // these are steps on atom_type * p !!
    using steps = mp::take<typename V::steps, framer>;

    using atom_type = std::remove_reference_t<decltype(*(std::declval<V>().data()))>;
    using cell_type = SmallView<atom_type, cell_lens, cell_steps>;
    using value_type = std::conditional_t<0==cellr, atom_type, cell_type>;

    cell_type c;

    constexpr CellSmall(CellSmall const & ci): c { ci.c.cp } {}
// see STLIterator for the case of s_[0]=0, etc. [ra12].
    constexpr CellSmall(atom_type * p_): c { p_ } {}
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr static rank_t rank_s() { return framer; }
    constexpr static rank_t rank() { return framer; }
    constexpr static dim_t len_s(int k) { RA_CHECK(inside(k, rank_s())); return V::len(k); }
    constexpr static dim_t len(int k) { RA_CHECK(inside(k, rank())); return V::len(k); }
    constexpr static dim_t step(int k) { return k<rank() ? V::step(k) : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { c.cp += step(k)*d; }

    constexpr auto
    flat() const
    {
        if constexpr (0==cellr) {
            return c.cp;
        } else {
            return CellFlat<cell_type> { c };
        }
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        if constexpr (0==cellr) {
            return c.cp[indexer0::longer<lens, steps>(i)];
        } else {
            return cell_type(c.cp + indexer0::longer<lens, steps>(i));
        }
    }
};


// --------------------
// Base for both small view & container
// --------------------

template <class lens_, class steps_, class ... I>
struct FilterDims
{
    using lens = lens_;
    using steps = steps_;
};

template <class lens_, class steps_, class I0, class ... I> requires (!is_iota<I0>)
struct FilterDims<lens_, steps_, I0, I ...>
{
    constexpr static bool stretch = (beatable<I0>.dst==DIM_BAD);
    static_assert(!stretch || ((beatable<I>.dst!=DIM_BAD) && ...), "Cannot repeat stretch index.");
    constexpr static int dst = stretch ? (mp::len<lens_> - (0 + ... + beatable<I>.src)) : beatable<I0>.dst;
    constexpr static int src = stretch ? (mp::len<lens_> - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
    using next = FilterDims<mp::drop<lens_, src>, mp::drop<steps_, src>, I ...>;
    using lens = mp::append<mp::take<lens_, dst>, typename next::lens>;
    using steps = mp::append<mp::take<steps_, dst>, typename next::steps>;
};

template <class lens_, class steps_, class I0, class ... I> requires (is_iota<I0>)
struct FilterDims<lens_, steps_, I0, I ...>
{
    constexpr static int dst = beatable<I0>.dst;
    constexpr static int src = beatable<I0>.src;
    using next = FilterDims<mp::drop<lens_, src>, mp::drop<steps_, src>, I ...>;
    using lens = mp::append<mp::int_list<I0::nn>, typename next::lens>;
    using steps = mp::append<mp::int_list<(mp::ref<steps_, 0>::value * I0::gets())>, typename next::steps>;
};

template <template <class ...> class Child_, class T_, class lens_, class steps_>
struct SmallBase
{
    using lens = lens_;
    using steps = steps_;
    using T = T_;

    using Child = Child_<T, lens, steps>;
    template <class TT> using BadDimension = int_c<(TT::value<0 || TT::value==DIM_ANY || TT::value==DIM_BAD)>;
    static_assert(!mp::apply<mp::orb, mp::map<BadDimension, lens>>::value, "Negative dimensions.");
    static_assert(mp::len<lens> == mp::len<steps>, "Mismatched lengths & steps."); // TODO static check on steps.

    constexpr static rank_t rank() { return mp::len<lens>; }
    constexpr static rank_t rank_s() { return mp::len<lens>; }
    constexpr static auto slens = mp::tuple_values<std::array<dim_t, rank()>, lens>();
    constexpr static auto ssteps = mp::tuple_values<std::array<dim_t, rank()>, steps>();
    constexpr static dim_t size() { return mp::apply<mp::prod, lens>::value; }
    constexpr static dim_t size_s() { return size(); }
    constexpr static dim_t len(int k) { return slens[k]; }
    constexpr static dim_t len_s(int k) { return slens[k]; }
    constexpr static dim_t step(int k) { return ssteps[k]; }
    constexpr static decltype(auto) shape() { return SmallView<ra::dim_t const, mp::int_list<rank_s()>, mp::int_list<1>>(slens.data()); }

    constexpr static bool convertible_to_scalar = (1==size()); // allowed for 1 for coord types

    template <int k>
    constexpr static dim_t
    select(dim_t i)
    {
        RA_CHECK(inside(i, slens[k]),
                 "Out of range for len[", k, "]=", slens[k], ": ", i, ".");
        return ssteps[k]*i;
    };

    template <int k, int n>
    constexpr static dim_t
    select(dots_t<n> i)
    {
        return 0;
    }

    template <int k, class I> requires (is_iota<std::decay_t<I>>)
    constexpr static dim_t
    select(I i)
    {
        if constexpr (0==i.n) {
            return 0;
        } else if constexpr ((1==i.n ? 1 : (i.s<0 ? -i.s : i.s)*(i.n-1)+1) > slens[k]) { // FIXME c++23 std::abs
            static_assert(mp::always_false<I>, "Out of range.");
        } else {
            RA_CHECK(inside(i, slens[k]),
                     "Out of range for len[", k, "]=", slens[k], ": iota [", i.n, " ", i.i, " ", i.s, "]");
        }
        return ssteps[k]*i.i;
    }

    template <int k>
    constexpr static dim_t
    select_loop()
    {
        return 0;
    }

    template <int k, class I0, class ... I>
    constexpr static dim_t
    select_loop(I0 && i0, I && ... i)
    {
        constexpr int nn = (DIM_BAD==beatable<I0>.src) ? (rank() - k - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
        return select<k>(with_len(int_c<slens[k]> {}, std::forward<I0>(i0)))
            + select_loop<k + nn>(std::forward<I>(i) ...);
    }

#define RA_CONST_OR_NOT(CONST)                                          \
    constexpr T CONST * data() CONST { return static_cast<Child CONST &>(*this).cp; } \
    template <class ... I>                                              \
    constexpr decltype(auto)                                            \
    operator()(I && ... i) CONST                                        \
    {                                                                   \
        if constexpr ((0 + ... + is_scalar_index<I>)==rank()) {         \
            return data()[select_loop<0>(i ...)];                       \
        } else if constexpr ((beatable<I>.ct && ...)) {                 \
            using FD = FilterDims<lens, steps, std::decay_t<I> ...>;    \
            return SmallView<T CONST, typename FD::lens, typename FD::steps> (data() + select_loop<0>(i ...)); \
        } else { /* TODO partial beating */                             \
            return unbeat<std::tuple<I ...>>::op(*this, std::forward<I>(i) ...); \
        }                                                               \
    }                                                                   \
    template <class ... I>                                              \
    constexpr decltype(auto)                                            \
    operator[](I && ... i) CONST                                        \
    {                                                                   \
        return (*this)(std::forward<I>(i) ...);                         \
    }                                                                   \
    /* BUG I must be fixed size, otherwise we can't make out the output type. */ \
    template <class I>                                                  \
    constexpr decltype(auto)                                            \
    at(I const & i) CONST                                               \
    {                                                                   \
        return SmallView<T CONST, mp::drop<lens, ra::size_s<I>()>, mp::drop<steps, ra::size_s<I>()>> \
            (data()+indexer0::shorter<lens, steps>(i));                 \
    }                                                                   \
    /* vestigial, maybe remove if int_c becomes easier to use */        \
    template <int ss, int oo=0>                                         \
    constexpr auto                                                      \
    as() CONST                                                          \
    {                                                                   \
        return operator()(ra::iota(ra::int_c<ss>(), oo));               \
    }                                                                   \
    T CONST &                                                           \
    back() CONST                                                        \
    {                                                                   \
        static_assert(rank()==1 && size()>0, "back() is not available"); \
        return (*this)[size()-1];                                       \
    }                                                                   \
    constexpr operator T CONST & () CONST requires (convertible_to_scalar) { return data()[0]; }
    FOR_EACH(RA_CONST_OR_NOT, /*const*/, const)
#undef RA_CONST_OR_NOT

// see same thing for View.
#define DEF_ASSIGNOPS(OP)                                               \
    template <class X>                                                  \
    requires (!mp::is_tuple<std::decay_t<X>>)                           \
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

// FIXME extend for cellr!=0?
    constexpr static bool steps_default = std::same_as<steps, default_steps<lens>>;
    constexpr auto begin() const { if constexpr (steps_default) return data(); else return STLIterator(iter()); }
    constexpr auto begin() { if constexpr (steps_default) return data(); else return STLIterator(iter()); }
    constexpr auto end() const { if constexpr (steps_default) return data()+size(); else return STLIterator(const_iterator<0>(nullptr)); }
    constexpr auto end() { if constexpr (steps_default) return data()+size(); else return STLIterator(iterator<0>(nullptr)); }
};


// ---------------------
// Small view & container
// ---------------------
// Strides are compile time, so we can put most members in the view type.

template <class T, class lens, class steps>
struct SmallView: public SmallBase<SmallView, T, lens, steps>
{
    using Base = SmallBase<SmallView, T, lens, steps>;
    using Base::operator=;

    T * cp;
    constexpr SmallView(T * cp_): cp(cp_) {}
    constexpr SmallView(SmallView const & s) = default;

    constexpr operator T & () { static_assert(Base::convertible_to_scalar); return cp[0]; }
    constexpr operator T const & () const { static_assert(Base::convertible_to_scalar); return cp[0]; };

    using ViewConst = SmallView<T const, lens, steps>;
    constexpr operator ViewConst () const requires (!std::is_const_v<T>) { return ViewConst(cp); }
    constexpr SmallView & view() { return *this; }
    constexpr SmallView const & view() const { return *this; }
};

#if defined (__clang__)
template <class T, int N> using extvector __attribute__((ext_vector_type(N))) = T;
#else
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;
#endif

template <class Z>
struct equal_to_t
{
    template <class ... T> constexpr static bool value = (std::is_same_v<Z, T> || ...);
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

    T cp[Base::size()]; // cf what std::array does for zero size; wish zero size just worked :-/

    constexpr SmallArray() {}
// braces don't match (X &&)
    constexpr SmallArray(nested_args const & ... x)
    {
        static_cast<Base &>(*this) = nested_arg<T, lens> { x ... };
    }
// braces row-major ravel for rank!=1
    constexpr SmallArray(ravel_args const & ... x)
    {
        static_cast<Base &>(*this) = ravel_arg<T, lens> { x ... };
    }
// needed if T isn't registered as scalar [ra44]
    constexpr SmallArray(T const & t)
    {
        for (auto & x: cp) { x = t; }
    }
// X && x makes this a better match than nested_args ... for 1 argument.
    template <class X>
    requires (!std::is_same_v<std::decay_t<X>, T> && !mp::is_tuple<std::decay_t<X>>)
    constexpr SmallArray(X && x)
    {
        static_cast<Base &>(*this) = x;
    }

    using View = SmallView<T, lens, steps>;
    using ViewConst = SmallView<T const, lens, steps>;
    constexpr View view() { return View(cp); }
    constexpr ViewConst view() const { return ViewConst(cp); }
// conversion to const
    constexpr operator View () { return View(cp); }
    constexpr operator ViewConst () const { return ViewConst(cp); }
};

template <class A0, class ... A>
SmallArray(A0, A ...) -> SmallArray<A0, mp::int_list<1+sizeof...(A)>>;

// FIXME remove the need, also of (S, begin, end) in Container, once nested_tuple constructors work.
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
    constexpr static rank_t rank_s() { return S::rank_s(); }
    constexpr static rank_t rank(T const & t) { return S::rank(); }
    constexpr static dim_t size_s() { return S::size_s(); }
    constexpr static dim_t size(T const & t) { return S::size_s(); }
    constexpr static decltype(auto) shape(T const & t) { return S::shape(); }
};

RA_IS_DEF(cv_smallview, (std::is_convertible_v<A, SmallView<typename A::T, typename A::lens, typename A::steps>>));


// --------------------
// Small view ops; cf View ops in big.hh.
// TODO Merge with Reframe (eg beat(reframe(a)) -> transpose(a) ?)
// --------------------

template <class A, class i>
struct axis_indices
{
    template <class T> using match_index = int_c<(T::value==i::value)>;
    using I = mp::iota<mp::len<A>>;
    using type = mp::Filter_<mp::map<match_index, A>, I>;
// allow dead axes (e.g. transpose<1>(rank 1 array)).
    // static_assert((mp::len<type>)>0, "dst axis doesn't appear in transposed axes list");
};

template <class axes_list, class src_lens, class src_steps>
struct axes_list_indices
{
    static_assert(mp::len<axes_list> == mp::len<src_lens>, "Bad size for transposed axes list.");
    constexpr static int talmax = mp::fold<mp::max, void, axes_list>::value;
    constexpr static int talmin = mp::fold<mp::min, void, axes_list>::value;
    static_assert(talmin >= 0, "Bad index in transposed axes list.");
// allow dead axes (e.g. transpose<1>(rank 1 array)).
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
    using lens = mp::map<dst_len, dst>;
    using steps = mp::map<dst_step, dst>;
};

template <int ... Iarg, class A>
requires (cv_smallview<A>)
constexpr auto
transpose(A && a_)
{
    decltype(auto) a = a_.view();
    using AA = typename std::decay_t<decltype(a)>;
    using ti = axes_list_indices<mp::int_list<Iarg ...>, typename AA::lens, typename AA::steps>;
    return SmallView<typename AA::T, typename ti::lens, typename ti::steps>(a.data());
};

template <class A>
requires (cv_smallview<A>)
constexpr auto
diag(A && a)
{
    return transpose<0, 0>(a);
}

// TODO generalize
template <class A1, class A2>
requires (cv_smallview<A1> || cv_smallview<A2>)
constexpr auto
cat(A1 && a1_, A2 && a2_)
{
    if constexpr (cv_smallview<A1> && cv_smallview<A2>) {
        decltype(auto) a1 = a1_.view();
        decltype(auto) a2 = a2_.view();
        static_assert(1==a1.rank() && 1==a2.rank(), "Bad ranks for cat."); // gcc accepts a1.rank(), etc.
        using T = std::common_type_t<std::decay_t<decltype(a1[0])>, std::decay_t<decltype(a2[0])>>;
        Small<T, a1.size()+a2.size()> val;
        std::copy(a1.begin(), a1.end(), val.begin());
        std::copy(a2.begin(), a2.end(), val.begin()+a1.size());
        return val;
    } else if constexpr (cv_smallview<A1> && is_scalar<A2>) {
        decltype(auto) a1 = a1_.view();
        static_assert(1==a1.rank(), "bad ranks for cat");
        using T = std::common_type_t<std::decay_t<decltype(a1[0])>, A2>;
        Small<T, a1.size()+1> val;
        std::copy(a1.begin(), a1.end(), val.begin());
        val[a1.size()] = a2_;
        return val;
    } else if constexpr (is_scalar<A1> && cv_smallview<A2>) {
        decltype(auto) a2 = a2_.view();
        static_assert(1==a2.rank(), "bad ranks for cat");
        using T = std::common_type_t<A1, std::decay_t<decltype(a2[0])>>;
        Small<T, 1+a2.size()> val;
        val[0] = a1_;
        std::copy(a2.begin(), a2.end(), val.begin()+1);
        return val;
    } else {
        static_assert(mp::always_false<A1, A2>); /* p2593r0 */ \
    }
}

// FIXME should be local (constexpr lambda + mp::apply?)
template <int s>
struct explode_divop
{
    template <class T> struct op_
    {
        static_assert((T::value/s)*s==T::value);
        using type = int_c<T::value / s>;
    };
    template <class T> using op = typename op_<T>::type;
};

template <class super_t, class A>
requires (cv_smallview<A>)
constexpr auto
explode(A && a_)
{
// the returned type has steps in super_t, but to support general steps we'd need steps in T. Maybe FIXME?
    decltype(auto) a = a_.view();
    using AA = std::decay_t<decltype(a)>;
    static_assert(super_t::steps_default);
    constexpr rank_t ra = AA::rank_s();
    constexpr rank_t rb = super_t::rank_s();
    static_assert(std::is_same_v<mp::drop<typename AA::lens, ra-rb>, typename super_t::lens>);
    static_assert(std::is_same_v<mp::drop<typename AA::steps, ra-rb>, typename super_t::steps>);
    using csteps = mp::map<explode_divop<ra::size_s<super_t>()>::template op, mp::take<typename AA::steps, ra-rb>>;
    return SmallView<super_t, mp::take<typename AA::lens, ra-rb>, csteps>((super_t *) a.data());
}

} // namespace ra
