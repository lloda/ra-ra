// -*- mode: c++; coding: utf-8 -*-
/// @file tuple-list.H
/// @brief Using tuples as compile time lists.

// (c) Daniel Llorens - 2005-2013, 2016, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// The convention here is that the alias xxx<...> is user facing and xxx_<...>::type is
/// implementation. Ideally we have only the alias.

#pragma once
#include "ra/macros.H"
#include <tuple>
#include <limits>

namespace mp {

template <int V> using int_t = std::integral_constant<int, V>;
template <bool V> using bool_t = std::integral_constant<bool, V>;

using std::tuple;
using nil = tuple<>;

template <class T> constexpr bool nilp = std::is_same_v<nil, T>;
template <class A> constexpr int len = std::tuple_size_v<A>;
template <int ... I> using int_list = tuple<int_t<I> ...>; // shortcut for std::integer_sequence<int, I ...>

template <class T> struct is_tuple { constexpr static bool value = false; };
template <class ... A> struct is_tuple<tuple<A ...>> { constexpr static bool value = true; };
template <class T> constexpr bool is_tuple_v = is_tuple<T>::value;

template <class A, class B> struct cons_ { static_assert(is_tuple_v<B>); };
template <class A0, class ... A> struct cons_<A0, tuple<A ...>> { using type = tuple<A0, A ...>; };
template <class A, class B> using cons = typename cons_<A, B>::type;

template <class A, class B> struct append_ { static_assert(is_tuple_v<A> && is_tuple_v<B>); };
template <class ... A, class ... B> struct append_<tuple<A ...>, tuple<B ...>> { using type = tuple<A ..., B ...>; };
template <class A, class B> using append = typename append_<A, B>::type;

template <class A, class B> struct zip_ { static_assert(is_tuple_v<A> && is_tuple_v<B>); };
template <class ... A, class ... B> struct zip_<tuple<A ...>, tuple<B ...>> { using type = tuple<tuple<A, B> ...>; };
template <class A, class B> using zip = typename zip_<A, B>::type;

template <int n, int o=0, int s=1> struct iota_ { static_assert(n>0); using type = cons<int_t<o>, typename iota_<n-1, o+s, s>::type>; };
template <int o, int s> struct iota_<0, o, s> { using type = nil; };
template <int n, int o=0, int s=1> using iota = typename iota_<n, o, s>::type;

template <int n, class T> struct makelist_ { static_assert(n>0); using type = cons<T, typename makelist_<n-1, T>::type>; };
template <class T> struct makelist_<0, T> { using type = nil; };
template <int n, class T> using makelist = typename makelist_<n, T>::type;

// A is a nested list, I the indices at each level.
template <class A, int ... I> struct ref_ { using type = A; };
template <class A, int ... I> using ref = typename ref_<A, I ...>::type;
template <class A, int I0, int ... I> struct ref_<A, I0, I ...> { using type = ref<std::tuple_element_t<I0, A>, I ...>; };

template <class A> using first = ref<A, 0>;
template <class A> using last = ref<A, (len<A> - 1)>;

template <bool a> using when = bool_t<a>;
template <bool a> using unless = bool_t<(!a)>;

// Return the index of a type in a type list, or -1 if not found.
template <class A, class T, int i=0> struct index_ { using type = int_t<-1>; };
template <class A, class T, int i=0> using index = typename index_<A, T, i>::type;
template <class ... A, class T, int i> struct index_<tuple<T, A ...>, T, i> { using type = int_t<i>; };
template <class A0, class ... A, class T, int i> struct index_<tuple<A0, A ...>, T, i> { using type = index<tuple<A ...>, T, i+1>; };

// Index (& type) of the 1st item for which Pred<> is true, or -1 (& nil).
template <class A, template <class> class Pred, int i=0, class Enable=void>
struct IndexIf
{
    constexpr static int value = -1;
    using type = nil;
};
template <class A0, class ... A, template <class> class Pred, int i>
struct IndexIf<tuple<A0, A ...>, Pred, i, std::enable_if_t<Pred<A0>::value>>
{
    using type = A0;
    constexpr static int value = i;
};
template <class A0, class ... A, template <class> class Pred, int i>
struct IndexIf<tuple<A0, A ...>, Pred, i, std::enable_if_t<!(Pred<A0>::value)>>
{
    using next = IndexIf<tuple<A ...>, Pred, i+1>;
    using type = typename next::type;
    constexpr static int value = next::value;
};

// Index (& type) of pairwise winner. A variant of fold.
template <template <class A, class B> class pick_i, class T, int k=1, int sel=0>
struct IndexOf;
template <template <class A, class B> class pick_i, class T0, int k, int sel>
struct IndexOf<pick_i, tuple<T0>, k, sel>
{
    constexpr static int value = sel;
    using type = T0;
};
template <template <class A, class B> class pick_i, class T0, class T1, class ... Ti, int k, int sel>
struct IndexOf<pick_i, tuple<T0, T1, Ti ...>, k, sel>
{
    constexpr static int i = pick_i<std::decay_t<T0>, std::decay_t<T1>>::value;
    using next = IndexOf<pick_i, tuple<std::conditional_t<i==0, T0, T1>, Ti ...>, k+1, i==0 ? sel : k>;
    using type = typename next::type;
    constexpr static int value = next::value;
};

// Return the first tail of A headed by Val, like find-tail.
template <class A, class Val> struct findtail_;
template <class A, class Val> using findtail = typename findtail_<A, Val>::type;
template <class Val> struct findtail_<nil, Val> { using type = nil; };
template <class ... A, class Val> struct findtail_<tuple<Val, A ...>, Val> { using type = tuple<Val, A ...>; };
template <class A0, class ... A, class Val> struct findtail_<tuple<A0, A ...>, Val> { using type = findtail<tuple<A ...>, Val>; };

// Reverse list. See TSPL^3, p. 137.
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

template <class ... A> struct max_ { using type = int_t<std::numeric_limits<int>::min()>; };
template <class ... A> using max = typename max_<A ...>::type;
template <class A0, class ... A> struct max_<A0, A ...> { using type = int_t<std::max(A0::value, max<A ...>::value)>; };

template <class ... A> struct min_ { using type = int_t<std::numeric_limits<int>::max()>; };
template <class ... A> using min = typename min_<A ...>::type;
template <class A0, class ... A> struct min_<A0, A ...> { using type = int_t<std::min(A0::value, min<A ...>::value)>; };

// Operations on int_t arguments.
template <class ... A> using sum = int_t<(A::value + ... + 0)>;
template <class ... A> using prod = int_t<(A::value * ... * 1)>;
template <class ... A> using andb = bool_t<(A::value && ... && true)>;
template <class ... A> using orb = bool_t<(A::value || ... || false)>;

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
template <class c, class A> struct MapCons;
template <class c, class A> using MapCons_ = typename MapCons<c, A>::type;
template <class c, class ... A> struct MapCons<c, tuple<A ...>> { using type = tuple<cons<c, A> ...>; };

// Prepend a list to each list in a list of lists.
template <class c, class A> struct MapPrepend;
template <class c, class A> using MapPrepend_ = typename MapPrepend<c, A>::type;
template <class c, class ... A> struct MapPrepend<c, tuple<A ...>> { using type = tuple<append<c, A> ...>; };

// Form all possible lists by prepending an element of A to an element of B.
template <class A, class B> struct ProductAppend { using type = nil; };
template <class A, class B> using ProductAppend_ = typename ProductAppend<A, B>::type;
template <class A0, class ... A, class B> struct ProductAppend<tuple<A0, A ...>, B> { using type = append<MapPrepend_<A0, B>, ProductAppend_<tuple<A ...>, B>>; };

// Compute the K-combinations of the N elements of list A.
template <class A, int K, int N=len<A>> struct combinations_;
template <class A, int k, int N=len<A>> using combinations = typename combinations_<A, k, N>::type;
// In this case, return a list with one element: the empty list.
template <class A, int N> struct combinations_<A, 0, N> { using type = tuple<nil>; };
// In this case, return a list with one element: the whole list.
template <class A, int N> struct combinations_<A, N, N> { using type = tuple<A>; };
// Special case for 0 over 0, to resolve ambiguity of 0/N and N/N when N=0.
template <> struct combinations_<nil, 0> { using type = tuple<nil>; };
template <class A, int K, int N>
struct combinations_
{
    static_assert(is_tuple_v<A>);
    static_assert(N>=0 && K>=0);
    static_assert(K<=N);
    using Rest = drop1<A>;
    using type = append<MapCons_<first<A>, combinations<Rest, K-1, N-1>>, combinations<Rest, K, N-1>>;
};

// Sign of permutations.
template <class C, class R> struct PermutationSign;

template <int w, class C, class R>
struct PermutationSignIfFound
{
    constexpr static int value = ((w & 1) ? -1 : +1) * PermutationSign<append<take<C, w>, drop<C, w+1>>,
                                                                       drop1<R>>::value;
};

template <class C, class R> struct PermutationSignIfFound<-1, C, R> { constexpr static int value = 0; };
template <> struct PermutationSign<nil, nil> { constexpr static int value = 1; };
template <class C> struct PermutationSign<C, nil> { constexpr static int value = 0; };
template <class R> struct PermutationSign<nil, R> { constexpr static int value = 0; };

template <class C, class Org>
struct PermutationSign
{
    constexpr static int value = PermutationSignIfFound<index<C, first<Org>>::value, C, Org>::value;
};

// increment the w-th element of an int_list
template <class L, int w> using inc = append<take<L, w>, cons<int_t<ref<L, w>::value+1>, drop<L, w+1>>>;

template <class A> struct InvertIndex;
template <class ... A> struct InvertIndex<tuple<A ...>>
{
    using AT = tuple<A ...>;
    template <class T> using IndexA = int_t<index<AT, T>::value>;
    constexpr static int N = apply<max, AT>::value;
    using type = map<IndexA, iota<(N>=0 ? N+1 : 0)>>;
};
template <class A> using InvertIndex_ = typename InvertIndex<A>::type;

// Used in tests.
template <class A, int ... I> struct check_idx { constexpr static bool value = false; };
template <> struct check_idx<nil> { constexpr static bool value = true; };

template <class A0, int I0, class ... A, int ... I>
struct check_idx<tuple<A0, A ...>, I0, I ...>
{
    constexpr static bool value = (A0::value==I0) && check_idx<tuple<A ...>, I ...>::value;
};

} // namespace mp
