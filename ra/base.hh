// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Before all other ra:: includes.

// (c) Daniel Llorens - 2005-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <tuple>
#include <array>
#include <ranges>
#include <vector>
#include <format>
#include <limits>
#include <iosfwd> // for format, ss.
#include <sstream>
#include <version>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <source_location>

#ifdef RA_AFTER_CHECK
#error Bad header include order! Do not include ra/base.hh after other ra:: headers.
#endif

#define RA_STRINGIZE_( x ) #x
#define RA_STRINGIZE( x ) RA_STRINGIZE_( x )
#define RA_JOIN_( x, y ) x##y
#define RA_JOIN( x, y ) RA_JOIN_( x, y )
#define RA_FW(a) std::forward<decltype(a)>(a)
// see http://stackoverflow.com/a/1872506
#define RA_FE_1(w, x, ...) w(x)
#define RA_FE_2(w, x, ...) w(x) RA_FE_1(w, __VA_ARGS__)
#define RA_FE_3(w, x, ...) w(x) RA_FE_2(w, __VA_ARGS__)
#define RA_FE_4(w, x, ...) w(x) RA_FE_3(w, __VA_ARGS__)
#define RA_FE_5(w, x, ...) w(x) RA_FE_4(w, __VA_ARGS__)
#define RA_FE_6(w, x, ...) w(x) RA_FE_5(w, __VA_ARGS__)
#define RA_FE_7(w, x, ...) w(x) RA_FE_6(w, __VA_ARGS__)
#define RA_FE_8(w, x, ...) w(x) RA_FE_7(w, __VA_ARGS__)
#define RA_FE_9(w, x, ...) w(x) RA_FE_8(w, __VA_ARGS__)
#define RA_FE_10(w, x, ...) w(x) RA_FE_9(w, __VA_ARGS__)
#define RA_FE_11(w, x, ...) w(x) RA_FE_10(w, __VA_ARGS__)
#define RA_FE_12(w, x, ...) w(x) RA_FE_11(w, __VA_ARGS__)
#define RA_FE_NARG(...) RA_FE_NARG_(__VA_ARGS__, RA_FE_RSEQ_N())
#define RA_FE_NARG_(...) RA_FE_ARG_N(__VA_ARGS__)
#define RA_FE_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) N
#define RA_FE_RSEQ_N() 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define RA_FE_(N, w, ...) RA_JOIN(RA_FE_, N)(w, __VA_ARGS__)
#define RA_FE(w, ...) RA_FE_(RA_FE_NARG(__VA_ARGS__), w, __VA_ARGS__)

// FIMXE bench shows it's bad; maybe requires optimizing += etc.
#ifndef RA_OPT_SMALL
#define RA_OPT_SMALL 0
#endif

namespace ra {

template <auto V> using ic_t = std::integral_constant<std::remove_const_t<decltype(V)>, V>;
template <auto V> constexpr std::integral_constant<std::remove_const_t<decltype(V)>, V> ic {};
template <class A> concept is_constant = requires (A a) { []<auto X>(ic_t<X> const &){}(a); };
template <int ... I> using ilist_t = std::tuple<ic_t<I> ...>;
template <int ... I> constexpr ilist_t<I ...> ilist {};


// ---------------------
// Tuple library.
// ---------------------

namespace mp {

using std::tuple;
using nil = tuple<>;

template <class A> constexpr int len = std::tuple_size_v<A>;
template <class T> constexpr bool is_tuple = false;
template <class ... A> constexpr bool is_tuple<tuple<A ...>> = true;
template <class ... A> using append = decltype(std::tuple_cat(std::declval<A>() ...));
template <class A, class B> using cons = append<std::tuple<A>, B>;

// ref<A, I0, I ...> = ref<ref<A, I0>, I ...>
template <class A, int ... I> struct ref_ { using type = A; };
template <class A, int ... I> using ref = typename ref_<A, I ...>::type;
template <class A, int I0, int ... I> struct ref_<A, I0, I ...> { using type = ref<std::tuple_element_t<I0, A>, I ...>; };
template <class A> using first = ref<A, 0>;
template <class A> using last = ref<A, len<A>-1>;

template <int n, int o, int s> struct iota_ { static_assert(n>0); using type = cons<ic_t<o>, typename iota_<n-1, o+s, s>::type>; };
template <int o, int s> struct iota_<0, o, s> { using type = nil; };
template <int n, int o=0, int s=1> using iota = typename iota_<n, o, s>::type;

template <class A, class B> struct zip_ { static_assert(is_tuple<A> && is_tuple<B>); };
template <class ... A, class ... B> struct zip_<tuple<A ...>, tuple<B ...>> { using type = tuple<tuple<A, B> ...>; };
template <class A, class B> using zip = typename zip_<A, B>::type;

template <int n, class T> struct makelist_ { static_assert(n>0); using type = cons<T, typename makelist_<n-1, T>::type>; };
template <class T> struct makelist_<0, T> { using type = nil; };
template <int n, class T> using makelist = typename makelist_<n, T>::type;

// Return the index of a type in a type list, or -1 if not found.
template <class A, class T, int i=0> struct index_ { using type = ic_t<-1>; };
template <class A, class T, int i=0> using index = typename index_<A, T, i>::type;
template <class ... A, class T, int i> struct index_<tuple<T, A ...>, T, i> { using type = ic_t<i>; };
template <class A0, class ... A, class T, int i> struct index_<tuple<A0, A ...>, T, i> { using type = index<tuple<A ...>, T, i+1>; };

// Index (& type) of the 1st item for which Pred<> is true, or -1 (& nil).
template <class A, template <class> class Pred, int i=0>
struct indexif
{
    constexpr static int value = -1;
    using type = nil;
};
template <class A0, class ... A, template <class> class Pred, int i>
requires (Pred<A0>::value)
struct indexif<tuple<A0, A ...>, Pred, i>
{
    using type = A0;
    constexpr static int value = i;
};
template <class A0, class ... A, template <class> class Pred, int i>
requires (!(Pred<A0>::value))
struct indexif<tuple<A0, A ...>, Pred, i>
{
    using next = indexif<tuple<A ...>, Pred, i+1>;
    using type = typename next::type;
    constexpr static int value = next::value;
};

template <class A, class Val> struct findtail_;
template <class A, class Val> using findtail = typename findtail_<A, Val>::type;
template <class Val> struct findtail_<nil, Val> { using type = nil; };
template <class ... A, class Val> struct findtail_<tuple<Val, A ...>, Val> { using type = tuple<Val, A ...>; };
template <class A0, class ... A, class Val> struct findtail_<tuple<A0, A ...>, Val> { using type = findtail<tuple<A ...>, Val>; };

template <class A, class B> struct reverse_ { using type = B; };
template <class A, class B=nil> using reverse = typename reverse_<A, B>::type;
template <class A0, class ... A, class B> struct reverse_<tuple<A0, A ...>, B> { using type = reverse<tuple<A ...>, cons<A0, B>>; };

// drop1 is needed to avoid ambiguity in the declarations of drop, take.
template <class A> struct drop1_;
template <class A0, class ... A> struct drop1_<tuple<A0, A ...>> { using type = tuple<A ...>; };
template <class A> using drop1 = typename drop1_<A>::type;

template <class A, int n> struct drop_ { static_assert(n>0); using type = typename drop_<drop1<A>, n-1>::type; };
template <class A> struct drop_<A, 0> { using type = A; };
template <class A, int n> using drop = typename drop_<A, n>::type;

template <class A, int n> struct take_ { static_assert(n>0); using type = cons<first<A>, typename take_<drop1<A>, n-1>::type>; };
template <class A> struct take_<A, 0> { using type = nil; };
template <class A, int n> using take = typename take_<A, n>::type;

template <template <class ... A> class F, class L> struct apply_;
template <template <class ... A> class F, class ... L> struct apply_<F, tuple<L ...>> { using type = F<L ...>; };
template <template <class ... A> class F, class L> using apply = typename apply_<F, L>::type;

template <template <class ... A> class F, class ... L> struct map_ { using type = cons<F<first<L> ...>, typename map_<F, drop1<L> ...>::type>; };
template <template <class ... A> class F, class ... L> struct map_<F, nil, L ...> { using type = nil; };
template <template <class ... A> class F> struct map_<F> { using type = nil; };
template <template <class ... A> class F, class ... L> using map = typename map_<F, L ...>::type;

template <class A, class B> struct filter_ { using type = append<std::conditional_t<first<A>::value, take<B, 1>, nil>, typename filter_<drop1<A>, drop1<B>>::type>; };
template <class B> struct filter_<nil, B> { using type = B; };
template <class A, class B> using filter = typename filter_<A, B>::type;

// Remove from the second list the elements of the first list. None may have repeated elements, but they may be unsorted.
template <class S, class T, class SS=S> struct complement_list_;
template <class S, class T, class SS=S> using complement_list = typename complement_list_<S, T, SS>::type;
// end of T.
template <class S, class SS>
struct complement_list_<S, nil, SS>
{
    using type = nil;
};
// end search on S, did not find.
template <class T0, class ... T, class SS>
struct complement_list_<nil, tuple<T0, T ...>, SS>
{
    using type = cons<T0, complement_list<SS, tuple<T ...>>>;
};
// end search on S, found.
template <class F, class ... S, class ... T, class SS>
struct complement_list_<tuple<F, S ...>, tuple<F, T ...>, SS>
{
    using type = complement_list<SS, tuple<T ...>>;
};
// keep searching on S.
template <class S0, class ... S, class T0, class ... T, class SS>
struct complement_list_<tuple<S0, S ...>, tuple<T0, T ...>, SS>
{
    using type = complement_list<tuple<S ...>, tuple<T0, T ...>, SS>;
};

// Like complement_list, but assume that both lists are sorted.
template <class S, class T> struct complement_sorted_list_ { using type = nil; };
template <class S, class T> using complement_sorted_list = typename complement_sorted_list_<S, T>::type;
template <class T> struct complement_sorted_list_<nil, T> { using type = T; };
template <class F, class ... S, class ... T>
struct complement_sorted_list_<tuple<F, S ...>, tuple<F, T ...>>
{
    using type = complement_sorted_list<tuple<S ...>, tuple<T ...>>;
};
template <class S0, class ... S, class T0, class ... T>
struct complement_sorted_list_<tuple<S0, S ...>, tuple<T0, T ...>>
{
    static_assert(T0::value<=S0::value, "Bad lists for complement_sorted_list<>.");
    using type = cons<T0, complement_sorted_list<tuple<S0, S ...>, tuple<T ...>>>;
};

// Like complement_list where the second argument is [0 .. end-1].
template <class S, int end> using complement = complement_sorted_list<S, iota<end>>;

// Prepend element to each of a list of lists.
template <class c, class A> struct mapcons_;
template <class c, class A> using mapcons = typename mapcons_<c, A>::type;
template <class c, class ... A> struct mapcons_<c, tuple<A ...>> { using type = tuple<cons<c, A> ...>; };

// Prepend list to each list in a list of lists.
template <class c, class A> struct mapprepend_;
template <class c, class A> using mapprepend = typename mapprepend_<c, A>::type;
template <class c, class ... A> struct mapprepend_<c, tuple<A ...>> { using type = tuple<append<c, A> ...>; };

// Form all lists by prepending an element of A to an element of B.
template <class A, class B> struct prodappend_ { using type = nil; };
template <class A, class B> using prodappend = typename prodappend_<A, B>::type;
template <class A0, class ... A, class B> struct prodappend_<tuple<A0, A ...>, B> { using type = append<mapprepend<A0, B>, prodappend<tuple<A ...>, B>>; };

// K-combinations of the N elements of list A.
template <class A, int K> struct combs_;
template <class A, int K> using combs = typename combs_<A, K>::type;
template <class A> struct combs_<A, 0> { using type = tuple<nil>; };
template <class A> struct combs_<A, len<A>> { using type = tuple<A>; };
template <> struct combs_<nil, 0> { using type = tuple<nil>; };
template <class A, int K>
struct combs_
{
    static_assert(is_tuple<A> && K>=0);
    using rest = drop1<A>;
    using type = append<mapcons<first<A>, combs<rest, K-1>>, combs<rest, K>>;
};

template <class C, class R> struct permsign;
template <int w, class C, class R> constexpr int permsignfound = permsign<append<take<C, w>, drop<C, w+1>>, drop1<R>>::value * ((w & 1) ? -1 : +1);
template <class C, class R> constexpr int permsignfound<-1, C, R>  = 0;
template <> struct permsign<nil, nil> { constexpr static int value = 1; };
template <class C> struct permsign<C, nil> { constexpr static int value = 0; };
template <class R> struct permsign<nil, R> { constexpr static int value = 0; };
template <class C, class O> struct permsign { constexpr static int value = permsignfound<index<C, first<O>>::value, C, O>; };

template <class P, class Plist>
struct findcomb
{
    template <class A> using match = ic_t<0 != permsign<P, A>::value>;
    using ii = indexif<Plist, match>;
    constexpr static int where = ii::value;
    constexpr static int sign = (where>=0) ? permsign<P, typename ii::type>::value : 0;
};

// Combination aC complementary to C wrt [0, 1, ... Dim-1], permuted so [C, aC] has the sign of [0, 1, ... Dim-1].
template <class C, int D>
struct anticomb
{
    using EC = complement<C, D>;
    static_assert(2<=len<EC>, "can't correct this complement");
    constexpr static int sign = permsign<append<C, EC>, iota<D>>::value;
// produce permutation of opposite sign if sign<0.
    using type = cons<ref<EC, (sign<0) ? 1 : 0>, cons<ref<EC, (sign<0) ? 0 : 1>, drop<EC, 2>>>;
};

template <class C, int D> struct mapanticomb;
template <int D, class ... C>
struct mapanticomb<std::tuple<C ...>, D>
{
    using type = std::tuple<typename anticomb<C, D>::type ...>;
};

template <int D, int O>
struct choose_
{
    static_assert(D>=O, "Bad dimension or form order.");
    using type = combs<iota<D>, O>;
};

template <int D, int O> using choose = typename choose_<D, O>::type;

template <int D, int O> requires ((D>1) && (2*O>D))
struct choose_<D, O>
{
    static_assert(D>=O, "Bad dimension or form order.");
    using type = typename mapanticomb<choose<D, D-O>, D>::type;
};

} // namespace ra::mp


// ---------------------
// Properly ra::.
// ---------------------

constexpr int VERSION = 31;
constexpr int ANY = -1944444444; // only static, meaning tbd at runtime
constexpr int UNB = -1988888888; // unbounded, eg dead axes
constexpr int MIS = -1922222222; // mismatch, only from choose_len

using rank_t = int;
using dim_t = std::ptrdiff_t;
static_assert(sizeof(rank_t)>=4 && sizeof(rank_t)>=sizeof(int) && sizeof(dim_t)>=sizeof(rank_t));
static_assert(std::is_signed_v<rank_t> && std::is_signed_v<dim_t>);

constexpr struct none_t {} none; // in constructors: don't initialize
struct noarg { noarg() = delete; }; // in constructors: don't instantiate

constexpr bool inside(dim_t i, dim_t b) { return 0<=i && i<b; }
constexpr bool any(bool const x) { return x; } // extended in ra.hh
constexpr bool every(bool const x) { return x; }

// adaptor that inserts construct() calls to convert value init into default init. See https://stackoverflow.com/a/21028912
// default storage for Big.
template <class T, class A=std::allocator<T>>
struct default_init_allocator: public A
{
    using traits = std::allocator_traits<A>;
    template <class U>
    struct rebind
    {
        using other = default_init_allocator<U, typename traits::template rebind_alloc<U>>;
    };
    template <class U>
    constexpr void construct(U * p) noexcept (std::is_nothrow_default_constructible<U>::value)
    {
        ::new(static_cast<void *>(p)) U; // constexpr in c++26
    }
    template <class U, class... Args>
    constexpr void construct(U * p, Args &&... args)
    {
        traits::construct(static_cast<A &>(*this), p, RA_FW(args)...);
    }
};

template <class T> using vector_default_init = std::vector<T, default_init_allocator<T>>;

template <class A>
concept Iterator = requires (A a, rank_t k, dim_t d, rank_t i, rank_t j)
{
    { a.rank() } -> std::same_as<rank_t>;
    { std::decay_t<A>::len_s(k) } -> std::same_as<dim_t>;
    { a.len(k) } -> std::same_as<dim_t>;
    { a.adv(k, d) } -> std::same_as<void>;
    { a.step(k) };
    { a.keep(d, i, j) } -> std::same_as<bool>;
    { a.save() };
    { a.load(std::declval<decltype(a.save())>()) } -> std::same_as<void>;
    { a.mov(d) } -> std::same_as<void>;
    { *a };
};

template <class A>
concept Slice = requires (A a)
{
    { a.rank() } -> std::same_as<rank_t>;
    { a.iter() } -> Iterator;
    { a.dimv };
};

// FIXME c++26 p2841 ?
#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A> constexpr bool RA_JOIN(NAME, _def) = requires { requires PRED; }; \
    template <class A> concept NAME = RA_JOIN(NAME, _def)<std::decay_t< A >>;

RA_IS_DEF(is_scalar, !std::is_pointer_v<A> && std::is_scalar_v<A> || is_constant<A>)
template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;
// template <> constexpr bool is_scalar_def<std::string_view> = true; // [ra13]

RA_IS_DEF(is_iterator, Iterator<A>)
template <class A> concept is_ra = is_iterator<A> || Slice<A>;
template <class A> concept is_builtin_array = std::is_array_v<std::remove_cvref_t<A>>;
RA_IS_DEF(is_fov, !is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::bidirectional_range<A>)

template <class VV> requires (!std::is_void_v<VV>)
consteval rank_t
rank_s()
{
    using V = std::remove_cvref_t<VV>;
    if constexpr (is_builtin_array<V>) {
        return std::rank_v<V>;
    } else if constexpr (is_fov<V>) {
        return 1;
    } else if constexpr (requires { V::rank(); }) {
        return V::rank();
    } else if constexpr (requires (V v) { v.rank(); }) {
        return ANY;
    } else {
        return 0;
    }
}

template <class V> consteval rank_t rank_s(V const &) { return rank_s<V>(); }

constexpr rank_t
rank(auto const & v)
{
    if constexpr (ANY!=rank_s(v)) {
        return rank_s(v);
    } else if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else {
        static_assert(false, "No rank() for this type.");
    }
}

// all args rank 0 (apply immediately), but at least one ra:: (disambiguate scalar version).
template <class A> concept is_ra_pos = is_ra<A> && 0!=rank_s<A>();
template <class A> concept is_ra_0 = (is_ra<A> && 0==rank_s<A>()) || is_scalar<A>;
RA_IS_DEF(is_special, false) // rank-0 types that we don't want reduced.
template <class ... A> constexpr bool toreduce = (!is_scalar<A> || ...) && ((is_ra_0<A> && !is_special<A>) && ...);
template <class ... A> constexpr bool tomap = ((is_ra_pos<A> || is_special<A>) || ...) && ((is_ra<A> || is_scalar<A> || is_fov<A> || is_builtin_array<A>) && ...);

// Sometimes we can't do shape(std::declval<V>()) even for static shape :-/ FIXME
template <class VV>
constexpr auto shape_s = []{
    using V = std::remove_cvref_t<VV>;
    if constexpr (constexpr rank_t rs=rank_s<V>(); 0==rs) {
        return std::array<dim_t, 0> {};
    } else if constexpr (is_builtin_array<V>) {
        return std::apply([](auto ... i){ return std::array { dim_t(std::extent_v<V, i>) ... }; }, mp::iota<rs> {});
    } else if constexpr (requires (V v) { []<class T, std::size_t N>(std::array<T, N> const &){}(v); }) {
        return std::array { dim_t(std::tuple_size_v<V>) };
    } else if constexpr (is_fov<V> && requires { V::extent; }) {
        return std::array { std::dynamic_extent==V::extent ? ANY : dim_t(V::extent) };
    } else if constexpr (is_fov<V>) {
        return std::array { ANY };
    } else {
        return std::apply([](auto ... i){ return std::array { V::len_s(i) ... }; }, mp::iota<rs> {});
    }
}();

template <class V> requires (!std::is_void_v<V>)
consteval dim_t
size_s()
{
    if constexpr (ANY==rank_s<V>()) {
        return ANY;
    } else {
        dim_t s = 1;
        for (dim_t len: shape_s<V>) { if (len>=0) s*=len; else return len; }; // ANY or UNB
        return s;
    }
}

template <class V> consteval dim_t size_s(V const &) { return size_s<V>(); }

template <class V>
constexpr dim_t
size(V const & v)
{
    if constexpr (ANY!=size_s(v)) {
        return size_s(v);
    } else if constexpr (is_fov<V>) {
        return std::ssize(v);
    } else {
        dim_t s = 1;
        for (rank_t k=0; k<rank(v); ++k) { s *= v.len(k); }
        return s;
    }
}

// Do not return expr objects. FIXME return ra:: types, but only if it's in all cases.
template <class V>
constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (ANY!=size_s<V>()) {
        return shape_s<V>;
    } else if constexpr (constexpr rank_t rs=rank_s<V>(); 1==rs) {
        return std::array<dim_t, 1> { ra::size(v) };
    } else if constexpr (ANY!=rs) {
        return std::apply([&v](auto ... i){ return std::array<dim_t, rs> { v.len(i) ... }; }, mp::iota<rs> {});
    } else {
        return std::ranges::to<vector_default_init<dim_t>>(
            std::ranges::iota_view { 0, rank(v) } | std::views::transform([&v](auto k){ return v.len(k); }));
    }
}


// --------------
// Format and print, forwards (see ply.hh).
// --------------

enum shape_t { defaultshape, withshape, noshape };

struct format_t
{
    shape_t shape = defaultshape;
    std::string open="", close="", sep0=" ", sepn="\n", rep="\n";
    bool align = false;
};

constexpr format_t jstyle = { };
constexpr format_t nstyle = { .shape=noshape };
constexpr format_t cstyle = { .shape=noshape, .open="{", .close="}", .sep0=", ", .sepn=",\n", .rep="", .align=true };
constexpr format_t lstyle = { .shape=noshape, .open="(", .close=")", .sep0=" ", .sepn="\n", .rep="", .align=true };
constexpr format_t pstyle = { .shape=noshape, .open="[", .close="]", .sep0=", ", .sepn=",\n", .rep="\n", .align=true };

template <class A> struct Fmt { format_t f = {}; A a; };
template <class A> constexpr auto fmt(format_t f, A && a) { return Fmt<A> { f, RA_FW(a) }; }

// exclude std::string_view so it still prints as a string [ra13].
RA_IS_DEF(is_array_formattable, is_ra<A> || (is_fov<A> && !std::is_convertible_v<A, std::string_view>));

constexpr std::ostream & operator<<(std::ostream & o, is_array_formattable auto && a) { return o << fmt({}, RA_FW(a)); }
template <class T>
constexpr std::ostream & operator<<(std::ostream & o, std::initializer_list<T> const & a) { return o << fmt({}, a); }

constexpr std::ostream &
operator<<(std::ostream & o, std::source_location const & loc)
{
    return o << loc.file_name() << ":" << loc.line() << "," << loc.column();
}

constexpr void print1(auto & o, std::formattable<char> auto && a) { std::print(o, "{}", RA_FW(a)); }
constexpr void print1(auto & o, auto && a) { o << RA_FW(a); }
constexpr auto & print(auto & o, auto && ... a) { (print1(o, RA_FW(a)), ...); return o; }
constexpr std::string format(auto && ... a) { std::ostringstream o; print(o, RA_FW(a) ...); return o.str(); }
constexpr std::string const & format(std::string const & s) { return s; }

} // namespace ra
