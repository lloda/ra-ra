// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Basic macros & tuple library.

// (c) Daniel Llorens - 2005-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <tuple>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <functional>
#include <utility>

#define STRINGIZE_( x ) #x
#define STRINGIZE( x ) STRINGIZE_( x )
#define JOIN_( x, y ) x##y
#define JOIN( x, y ) JOIN_( x, y )

// see http://stackoverflow.com/a/1872506
#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...) what(x) FOR_EACH_1(what, __VA_ARGS__)
#define FOR_EACH_3(what, x, ...) what(x) FOR_EACH_2(what, __VA_ARGS__)
#define FOR_EACH_4(what, x, ...) what(x) FOR_EACH_3(what, __VA_ARGS__)
#define FOR_EACH_5(what, x, ...) what(x) FOR_EACH_4(what, __VA_ARGS__)
#define FOR_EACH_6(what, x, ...) what(x) FOR_EACH_5(what, __VA_ARGS__)
#define FOR_EACH_7(what, x, ...) what(x) FOR_EACH_6(what, __VA_ARGS__)
#define FOR_EACH_8(what, x, ...) what(x) FOR_EACH_7(what, __VA_ARGS__)
#define FOR_EACH_9(what, x, ...) what(x) FOR_EACH_8(what, __VA_ARGS__)
#define FOR_EACH_10(what, x, ...) what(x) FOR_EACH_9(what, __VA_ARGS__)
#define FOR_EACH_11(what, x, ...) what(x) FOR_EACH_10(what, __VA_ARGS__)
#define FOR_EACH_12(what, x, ...) what(x) FOR_EACH_11(what, __VA_ARGS__)
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) N
#define FOR_EACH_RSEQ_N() 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOR_EACH_(N, what, ...) JOIN(FOR_EACH_, N)(what, __VA_ARGS__)
#define FOR_EACH(what, ...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)

#define RA_FWD(a) std::forward<decltype(a)>(a)

namespace ra {

template <class T> constexpr bool is_constant = false;
template <class T, T N> constexpr bool is_constant<std::integral_constant<T, N>> = true;
template <int V> using int_c = std::integral_constant<int, V>;
template <bool V> using bool_c = std::integral_constant<bool, V>;
template <auto V> using ic_t = std::integral_constant<decltype(V), V>;
template <auto V> constexpr std::integral_constant<decltype(V), V> ic {};
template <class ... T> constexpr bool always_false = false; // p2593r0

} // namespace ra

namespace ra::mp {

// xxx<...> is user facing and xxx_<...>::type (if needed) is implementation.

using std::tuple;
using nil = tuple<>;

template <class T> constexpr bool nilp = std::is_same_v<nil, T>;
template <class A> constexpr int len = std::tuple_size_v<A>;
template <int ... I> using int_list = tuple<int_c<I> ...>;
template <class T> constexpr bool is_tuple = false;
template <class ... A> constexpr bool is_tuple<tuple<A ...>> = true;

// A is a nested list, I the indices at each level.
template <class A, int ... I> struct ref_ { using type = A; };
template <class A, int ... I> using ref = typename ref_<A, I ...>::type;
template <class A, int I0, int ... I> struct ref_<A, I0, I ...> { using type = ref<std::tuple_element_t<I0, A>, I ...>; };
template <class A> using first = ref<A, 0>;
template <class A> using last = ref<A, len<A>-1>;

template <class A, class B> struct cons_ { static_assert(is_tuple<B>); };
template <class A0, class ... A> struct cons_<A0, tuple<A ...>> { using type = tuple<A0, A ...>; };
template <class A, class B> using cons = typename cons_<A, B>::type;

template <int n, int o=0, int s=1> struct iota_ { static_assert(n>0); using type = cons<int_c<o>, typename iota_<n-1, o+s, s>::type>; };
template <int o, int s> struct iota_<0, o, s> { using type = nil; };
template <int n, int o=0, int s=1> using iota = typename iota_<n, o, s>::type;

template <class A, class B> struct append_ { static_assert(is_tuple<A> && is_tuple<B>); };
template <class ... A, class ... B> struct append_<tuple<A ...>, tuple<B ...>> { using type = tuple<A ..., B ...>; };
template <class A, class B> using append = typename append_<A, B>::type;

template <class A, class B> struct zip_ { static_assert(is_tuple<A> && is_tuple<B>); };
template <class ... A, class ... B> struct zip_<tuple<A ...>, tuple<B ...>> { using type = tuple<tuple<A, B> ...>; };
template <class A, class B> using zip = typename zip_<A, B>::type;

template <int n, class T> struct makelist_ { static_assert(n>0); using type = cons<T, typename makelist_<n-1, T>::type>; };
template <class T> struct makelist_<0, T> { using type = nil; };
template <int n, class T> using makelist = typename makelist_<n, T>::type;

// Return the index of a type in a type list, or -1 if not found.
template <class A, class T, int i=0> struct index_ { using type = int_c<-1>; };
template <class A, class T, int i=0> using index = typename index_<A, T, i>::type;
template <class ... A, class T, int i> struct index_<tuple<T, A ...>, T, i> { using type = int_c<i>; };
template <class A0, class ... A, class T, int i> struct index_<tuple<A0, A ...>, T, i> { using type = index<tuple<A ...>, T, i+1>; };

// Index (& type) of the 1st item for which Pred<> is true, or -1 (& nil).
template <class A, template <class> class Pred, int i=0>
struct IndexIf
{
    constexpr static int value = -1;
    using type = nil;
};
template <class A0, class ... A, template <class> class Pred, int i>
requires (Pred<A0>::value)
struct IndexIf<tuple<A0, A ...>, Pred, i>
{
    using type = A0;
    constexpr static int value = i;
};
template <class A0, class ... A, template <class> class Pred, int i>
requires (!(Pred<A0>::value))
struct IndexIf<tuple<A0, A ...>, Pred, i>
{
    using next = IndexIf<tuple<A ...>, Pred, i+1>;
    using type = typename next::type;
    constexpr static int value = next::value;
};

// Index (& type) of pairwise winner. A variant of fold.
template <template <class A, class B> class pick_i, class T, int k=1, int sel=0> struct indexof_;
template <template <class A, class B> class pick_i, class T0, int k, int sel>
struct indexof_<pick_i, tuple<T0>, k, sel>
{
    constexpr static int value = sel;
    using type = T0;
};
template <template <class A, class B> class pick_i, class T0, class T1, class ... Ti, int k, int sel>
struct indexof_<pick_i, tuple<T0, T1, Ti ...>, k, sel>
{
    constexpr static int i = pick_i<std::decay_t<T0>, std::decay_t<T1>>::value;
    using next = indexof_<pick_i, tuple<std::conditional_t<i==0, T0, T1>, Ti ...>, k+1, i==0 ? sel : k>;
    using type = typename next::type;
    constexpr static int value = next::value;
};
template <template <class A, class B> class pick_i, class T>
constexpr int indexof = indexof_<pick_i, T>::value;

template <class A, class Val> struct findtail_;
template <class A, class Val> using findtail = typename findtail_<A, Val>::type;
template <class Val> struct findtail_<nil, Val> { using type = nil; };
template <class ... A, class Val> struct findtail_<tuple<Val, A ...>, Val> { using type = tuple<Val, A ...>; };
template <class A0, class ... A, class Val> struct findtail_<tuple<A0, A ...>, Val> { using type = findtail<tuple<A ...>, Val>; };

template <class A, class B=nil> struct reverse_ { using type = B; };
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

// As map.
template <template <class ... A> class F, class ... L>
struct map_ { using type = cons<F<first<L> ...>, typename map_<F, drop1<L> ...>::type>; };
template <template <class ... A> class F, class ... L>
struct map_<F, nil, L ...> { using type = nil; };
template <template <class ... A> class F>
struct map_<F> { using type = nil; };
template <template <class ... A> class F, class ... L> using map = typename map_<F, L ...>::type;

template <class A, class B> struct Filter
{
    using type = mp::append<std::conditional_t<mp::first<A>::value, mp::take<B, 1>, mp::nil>,
                            typename Filter<mp::drop1<A>, mp::drop1<B>>::type>;
};
template <class B> struct Filter<mp::nil, B> { using type = B; };
template <class A, class B> using Filter_ = typename Filter<A, B>::type;

// As SRFI-1 fold (= fold-left).
template <template <class ... A> class F, class Def, class ... L>
struct fold_
{
    using def = std::conditional_t<std::is_same_v<void, Def>, F<>, Def>;
    using type = typename fold_<F, F<def, first<L> ...>, drop1<L> ...>::type;
};
template <template <class ... A> class F, class Def, class ... L>
struct fold_<F, Def, nil, L ...>
{
    using type = std::conditional_t<std::is_same_v<void, Def>, F<>, Def>;
};
template <template <class ... A> class F, class Def>
struct fold_<F, Def>
{
    using type = std::conditional_t<std::is_same_v<void, Def>, F<>, Def>;
};
template <template <class ... A> class F, class Def, class ... L>
using fold = typename fold_<F, Def, L ...>::type;

template <class ... A> struct max_ { using type = int_c<std::numeric_limits<int>::min()>; };
template <class ... A> using max = typename max_<A ...>::type;
template <class A0, class ... A> struct max_<A0, A ...> { using type = int_c<std::max(A0::value, max<A ...>::value)>; };

template <class ... A> struct min_ { using type = int_c<std::numeric_limits<int>::max()>; };
template <class ... A> using min = typename min_<A ...>::type;
template <class A0, class ... A> struct min_<A0, A ...> { using type = int_c<std::min(A0::value, min<A ...>::value)>; };

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

template <class T>
struct complement_sorted_list_<nil, T>
{
    using type = T;
};
template <class F, class ... S, class ... T>
struct complement_sorted_list_<tuple<F, S ...>, tuple<F, T ...>>
{
    using type = complement_sorted_list<tuple<S ...>, tuple<T ...>>;
};
template <class S0, class ... S, class T0, class ... T>
struct complement_sorted_list_<tuple<S0, S ...>, tuple<T0, T ...>>
{
    static_assert(T0::value<=S0::value, "bad lists for complement_sorted_list<>");
    using type = cons<T0, complement_sorted_list<tuple<S0, S ...>, tuple<T ...>>>;
};

// Variant of complement_list where the second argument is [0 .. end-1].
template <class S, int end> using complement = complement_sorted_list<S, iota<end>>;

// Prepend an element to each of a list of lists.
template <class c, class A> struct MapCons_;
template <class c, class A> using MapCons = typename MapCons_<c, A>::type;
template <class c, class ... A> struct MapCons_<c, tuple<A ...>> { using type = tuple<cons<c, A> ...>; };

// Prepend a list to each list in a list of lists.
template <class c, class A> struct MapPrepend_;
template <class c, class A> using MapPrepend = typename MapPrepend_<c, A>::type;
template <class c, class ... A> struct MapPrepend_<c, tuple<A ...>> { using type = tuple<append<c, A> ...>; };

// Form all possible lists by prepending an element of A to an element of B.
template <class A, class B> struct ProductAppend_ { using type = nil; };
template <class A, class B> using ProductAppend = typename ProductAppend_<A, B>::type;
template <class A0, class ... A, class B> struct ProductAppend_<tuple<A0, A ...>, B> { using type = append<MapPrepend<A0, B>, ProductAppend<tuple<A ...>, B>>; };

// Compute the K-combinations of the N elements of list A.
template <class A, int K, int N=len<A>> struct combinations_;
template <class A, int k, int N=len<A>> using combinations = typename combinations_<A, k, N>::type;
template <class A, int N> struct combinations_<A, 0, N> { using type = tuple<nil>; };
template <class A, int N> struct combinations_<A, N, N> { using type = tuple<A>; };
// Resolve ambiguity of 0/N and N/N when N=0.
template <> struct combinations_<nil, 0> { using type = tuple<nil>; };
template <class A, int K, int N>
struct combinations_
{
    static_assert(is_tuple<A>);
    static_assert(N>=0 && K>=0);
    static_assert(K<=N);
    using Rest = drop1<A>;
    using type = append<MapCons<first<A>, combinations<Rest, K-1, N-1>>, combinations<Rest, K, N-1>>;
};

// Sign of permutations.
template <class C, class R> struct PermutationSign;

template <int w, class C, class R>
constexpr int PermutationSignIfFound = PermutationSign<append<take<C, w>, drop<C, w+1>>, drop1<R>>::value
    * ((w & 1) ? -1 : +1);

template <class C, class R>
constexpr int PermutationSignIfFound<-1, C, R>  = 0;

template <> struct PermutationSign<nil, nil> { constexpr static int value = 1; };
template <class C> struct PermutationSign<C, nil> { constexpr static int value = 0; };
template <class R> struct PermutationSign<nil, R> { constexpr static int value = 0; };

template <class C, class Org>
struct PermutationSign
{
    constexpr static int value = PermutationSignIfFound<index<C, first<Org>>::value, C, Org>;
};

template <class A> struct InvertIndex_;
template <class ... A> struct InvertIndex_<tuple<A ...>>
{
    using AT = tuple<A ...>;
    template <class T> using IndexA = int_c<index<AT, T>::value>;
    constexpr static int N = apply<max, AT>::value;
    using type = map<IndexA, iota<(N>=0 ? N+1 : 0)>>;
};
template <class A> using InvertIndex = typename InvertIndex_<A>::type;


// ---------------------
// tuples in dynamic context
// ---------------------

// like std::make_trom_tuple, but use brace constructor.
template <class C, class T>
constexpr C
from_tuple(T && t)
{
    return std::apply([](auto && ... x) { return C { RA_FWD(x) ... }; }, t);
}

template <class C, class T, auto f = std::identity {}>
consteval auto
tuple2array()
{
    return std::apply([](auto ... t) { return std::array<C, len<T>> { C(f(t)) ... }; }, T {});
}

template <class C, class T, class I>
constexpr C
map_indices(I const & i)
{
    return std::apply([&i](auto ... t) { return std::array<C, len<T>> { i[t] ... }; }, T {});
};

template <class T, int k=0>
constexpr int
int_list_index(int i)
{
    if constexpr (k>=mp::len<T>) {
        return -1;
    } else {
        return (i==mp::ref<T, k>::value) ? k : int_list_index<T, k+1>(i);
    }
}

template <class K, class T, class F, class I = int_c<0>>
constexpr auto
fold_tuple(K && k, T && t, F && f, I && i = int_c<0> {})
{
    if constexpr (I::value==len<std::decay_t<T>>) {
        return k;
    } else {
        return fold_tuple(f(k, std::get<I::value>(t)), t, f, int_c<I::value+1> {});
    }
}

} // namespace ra::mp
