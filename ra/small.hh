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

constexpr rank_t
rank_sum(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a+b; }

constexpr rank_t
rank_diff(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a-b; }

// cr>=0 is cell rank. -cr>0 is frame rank. TODO A way to indicate frame rank 0.
constexpr rank_t
rank_cell(rank_t r, rank_t cr) { return cr>=0 ? cr /* independent */ : r==ANY ? ANY /* defer */ : (r+cr); }

constexpr rank_t
rank_frame(rank_t r, rank_t cr) { return r==ANY ? ANY /* defer */ : cr>=0 ? (r-cr) /* independent */ : -cr; }

struct Dim { dim_t len=0, step=0; }; // cf View::end() [ra17]

inline std::ostream &
operator<<(std::ostream & o, Dim const & dim) { return (o << "[Dim " << dim.len << " " << dim.step << "]"); }


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

template <class I> struct is_constant_iota
{
    using Ilen = std::decay_t<decltype(with_len(ic<1>, std::declval<I>()))>; // arbitrary constant len
    constexpr static bool value = is_constant<typename Ilen::N> && is_constant<typename Ilen::S>;
};

template <class I> requires (is_iota<I>) constexpr beatable_t beatable_def<I>
    = { .rt=(BAD!=I::nn), .ct=is_constant_iota<I>::value, .src=1, .dst=1, .add=0 };

template <class I> constexpr beatable_t beatable = beatable_def<std::decay_t<I>>;

template <int k=0, class V>
constexpr decltype(auto)
maybe_len(V && v)
{
    if constexpr (ANY!=std::decay_t<V>::len_s(k)) {
        return ic<std::decay_t<V>::len_s(k)>;
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
// Develop indices
// --------------------

template <rank_t k, rank_t end, class Q, class P, class S>
constexpr dim_t
indexer(Q const & q, P && pp, S const & ss0)
{
    if constexpr (k==end) {
        return 0;
    } else {
        auto pk = *pp;
        RA_CHECK(inside(pk, q.len(k)) || (BAD==q.len(k) && 0==q.step(k)));
        return (q.step(k) * pk) + (pp+=ss0, indexer<k+1, end>(q, pp, ss0));
    }
}

template <class Q, class P, class S>
constexpr dim_t
indexer(rank_t end, Q const & q, P && pp, S const & ss0)
{
    dim_t c = 0;
    for (rank_t k=0; k<end; ++k, pp+=ss0) {
        auto pk = *pp;
        RA_CHECK(inside(pk, q.len(k)) || (BAD==q.len(k) && 0==q.step(k)));
        c += q.step(k) * pk;
    }
    return c;
}

template <class Q, class P>
constexpr dim_t
longer(Q const & q, P const & pp)
{
    decltype(auto) p = start(pp);
    if constexpr (ANY==rank_s<P>()) {
        RA_CHECK(1==rank(p), "Bad rank ", rank(p), " for subscript.");
    } else {
        static_assert(1==rank_s<P>(), "Bad rank for subscript.");
    }
    if constexpr (ANY==size_s<P>() || ANY==rank_s<Q>()) {
        RA_CHECK(p.len(0) >= q.rank(), "Too few indices.");
    } else {
        static_assert(size_s<P>() >= rank_s<Q>(), "Too few indices.");
    }
    if constexpr (ANY==rank_s<Q>()) {
        return indexer(q.rank(), q, p.flat(), p.step(0));
    } else {
        return indexer<0, rank_s<Q>()>(q, p.flat(), p.step(0));
    }
}


// --------------------
// Small iterator
// --------------------

template <class C>
struct CellFlat
{
    C c;
    constexpr void operator+=(dim_t const s) { c.cp += s; }
    constexpr C & operator*() { return c; }
};

// TODO Refactor with CellBig / STLIterator
template <class T, class Dimv, rank_t spec=0>
struct CellSmall
{
    constexpr static auto dimv = Dimv::value;
    static_assert(spec!=ANY && spec!=BAD, "Bad cell rank.");
    constexpr static rank_t fullr = ssize(dimv);
    constexpr static rank_t cellr = rank_cell(fullr, spec);
    constexpr static rank_t framer = rank_frame(fullr, spec);
    static_assert(cellr>=0 || cellr==ANY, "Bad cell rank.");
    static_assert(framer>=0 || framer==ANY, "Bad frame rank.");
    static_assert(choose_rank(fullr, cellr)==fullr, "Bad cell rank.");

// FIXME Small take dimv instead of lens/steps
    using clens = decltype(std::apply([](auto ... i) { return std::tuple<int_c<dimv[i].len> ...> {}; }, mp::iota<cellr, framer> {}));
    using csteps = decltype(std::apply([](auto ... i) { return std::tuple<int_c<dimv[i].step> ...> {}; }, mp::iota<cellr, framer> {}));
    using ctype = SmallView<T, clens, csteps>;
    using value_type = std::conditional_t<0==cellr, T, ctype>;

    ctype c;

    constexpr static rank_t rank_s() { return framer; }
    constexpr static rank_t rank() { return framer; }
#pragma GCC diagnostic push // gcc 13.2
#pragma GCC diagnostic warning "-Warray-bounds"
    constexpr static dim_t len(int k) { return dimv[k].len; } // len(0<=k<rank) or step(0<=k)
#pragma GCC diagnostic pop
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return k<rank() ? dimv[k].step : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { c.cp += step(k)*d; }

// see STLIterator for the case of s_[0]=0, etc. [ra12].
    constexpr CellSmall(T * p): c { p } {}
    constexpr CellSmall(CellSmall const & ci) = default;
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr auto
    flat() const
    {
        if constexpr (0==cellr) {
            return c.cp;
        } else {
            return CellFlat<ctype> { c };
        }
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        auto d = longer(*this, i);
        if constexpr (0==cellr) {
            return c.cp[d];
        } else {
            ctype cc(c); cc.cp += d;
            return cc;
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
    constexpr static bool stretch = (beatable<I0>.dst==BAD);
    static_assert(!stretch || ((beatable<I>.dst!=BAD) && ...), "Cannot repeat stretch index.");
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

    static_assert(mp::len<lens> == mp::len<steps>, "Mismatched lengths & steps.");
    consteval static rank_t rank() { return mp::len<lens>; }
    constexpr static auto dimv = std::apply([](auto ... i) { return std::array<Dim, rank()> { Dim { mp::ref<lens, i>::value, mp::ref<steps, i>::value } ... }; }, mp::iota<rank()> {});
    constexpr static auto theshape = mp::tuple_values<dim_t, lens>();
    consteval static dim_t size() { return std::apply([](auto ... s) { return (s * ... * 1); }, theshape); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    consteval static rank_t rank_s() { return rank(); }
    consteval static dim_t size_s() { return size(); }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static auto shape() { return SmallView<ra::dim_t const, mp::int_list<rank()>, mp::int_list<1>>(theshape.data()); }
// TODO check steps
    static_assert(!std::apply([](auto ... s) { return ((0>s || ANY==s || BAD==s) || ...); }, theshape), "Bad dimensions.");

    constexpr static bool convertible_to_scalar = (1==size()); // allowed for 1 for coord types

    template <int k>
    constexpr static dim_t
    select(dim_t i)
    {
        RA_CHECK(inside(i, len(k)),
                 "Out of range for len[", k, "]=", len(k), ": ", i, ".");
        return step(k)*i;
    };

    template <int k, class I> requires (is_iota<I>)
    constexpr static dim_t
    select(I i)
    {
        if constexpr (0==i.n) {
            return 0;
        } else if constexpr ((1==i.n ? 1 : (i.s<0 ? -i.s : i.s)*(i.n-1)+1) > len(k)) { // FIXME c++23 std::abs
            static_assert(always_false<I>, "Out of range.");
        } else {
            RA_CHECK(inside(i, len(k)),
                     "Out of range for len[", k, "]=", len(k), ": iota [", i.n, " ", i.i, " ", i.s, "]");
        }
        return step(k)*i.i;
    }

    template <int k, int n>
    constexpr static dim_t
    select(dots_t<n> i)
    {
        return 0;
    }

    template <int k, class I0, class ... I>
    constexpr static dim_t
    select_loop(I0 && i0, I && ... i)
    {
        constexpr int nn = (BAD==beatable<I0>.src) ? (rank() - k - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
        return select<k>(with_len(ic<len(k)>, std::forward<I0>(i0)))
            + select_loop<k + nn>(std::forward<I>(i) ...);
    }

    template <int k>
    constexpr static dim_t
    select_loop()
    {
        return 0;
    }

#define RA_CONST_OR_NOT(CONST)                                          \
    constexpr T CONST * data() CONST { return static_cast<Child CONST &>(*this).cp; } \
    template <class ... I>                                              \
    constexpr decltype(auto)                                            \
    operator()(I && ... i) CONST                                        \
    {                                                                   \
        constexpr int stretch = (0 + ... + (beatable<I>.dst==BAD));     \
        static_assert(stretch<=1, "Cannot repeat stretch index.");      \
        if constexpr ((0 + ... + is_scalar_index<I>)==rank()) {         \
            return data()[select_loop<0>(i ...)];                       \
            /* FIXME with_len before this, cf is_constant_iota */       \
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
    template <class I>                                                  \
    constexpr decltype(auto)                                            \
    at(I && i) CONST                                                    \
    {                                                                   \
        /* FIXME there's no way to say 'frame rank 0' so -size wouldn't work. */ \
        constexpr rank_t crank = rank_diff(rank(), ra::size_s<I>());    \
        static_assert(crank>=0); /* else we can't make out the output type */ \
        return iter<crank>().at(std::forward<I>(i));                    \
    }                                                                   \
    /* maybe remove if ic becomes easier to use */                      \
    template <int ss, int oo=0>                                         \
    constexpr auto                                                      \
    as() CONST                                                          \
    {                                                                   \
        return operator()(ra::iota(ra::ic<ss>, oo));                    \
    }                                                                   \
    decltype(auto)                                                      \
    back() CONST                                                        \
    {                                                                   \
        static_assert(rank()>=1 && size()>0, "back() is not available"); \
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

    template <rank_t c=0> using iterator = CellSmall<T, ic_t<dimv>, c>;
    template <rank_t c=0> using const_iterator = CellSmall<T const, ic_t<dimv>, c>;
    template <rank_t c=0> constexpr iterator<c> iter() { return data(); }
    template <rank_t c=0> constexpr const_iterator<c> iter() const { return data(); }

    constexpr static bool def = std::same_as<steps, default_steps<lens>>;
    constexpr auto begin() const { if constexpr (def) return data(); else return STLIterator(iter()); }
    constexpr auto begin() { if constexpr (def) return data(); else return STLIterator(iter()); }
    constexpr auto end() const { if constexpr (def) return data()+size(); else return STLIterator(const_iterator<0>(nullptr)); }
    constexpr auto end() { if constexpr (def) return data()+size(); else return STLIterator(iterator<0>(nullptr)); }
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
    constexpr SmallView(SmallView const & s): cp(s.cp) {}

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

template <class Z, class ... T>
constexpr static bool equal_to_any = (std::is_same_v<Z, T> || ...);

template <class T, size_t N>
consteval size_t
align_req()
{
    if constexpr (equal_to_any<T, char, unsigned char, short, unsigned short,
                  int, unsigned int, long, unsigned long, long long, unsigned long long,
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
    template <class T> using match_index = ic_t<(T::value==i::value)>;
    using I = mp::iota<mp::len<A>>;
    using type = mp::Filter_<mp::map<match_index, A>, I>;
};

template <class axes_list, class src_lens, class src_steps>
struct axes_list_indices
{
    static_assert(mp::len<axes_list> == mp::len<src_lens>, "Bad size for transposed axes list.");
    constexpr static int talmax = mp::fold<mp::max, void, axes_list>::value;
    constexpr static int talmin = mp::fold<mp::min, void, axes_list>::value;
    static_assert(talmin >= 0, "Bad index in transposed axes list.");

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
        static_assert(always_false<A1, A2>);
    }
}

// FIXME should be local (constexpr lambda + mp::apply?)
template <int s>
struct explode_divop
{
    template <class T> struct op_
    {
        static_assert((T::value/s)*s==T::value);
        using type = ic_t<T::value / s>;
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
    static_assert(super_t::def);
    constexpr rank_t ra = AA::rank_s();
    constexpr rank_t rb = super_t::rank_s();
    static_assert(std::is_same_v<mp::drop<typename AA::lens, ra-rb>, typename super_t::lens>);
    static_assert(std::is_same_v<mp::drop<typename AA::steps, ra-rb>, typename super_t::steps>);
    using csteps = mp::map<explode_divop<ra::size_s<super_t>()>::template op, mp::take<typename AA::steps, ra-rb>>;
    return SmallView<super_t, mp::take<typename AA::lens, ra-rb>, csteps>((super_t *) a.data());
}

} // namespace ra
