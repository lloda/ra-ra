
// (c) Daniel Llorens - 2005-2013, 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_TUPLELIST_H
#define RA_TUPLELIST_H

/// @file tuple-list.H
/// @brief Using tuples as compile time lists.

#include "ra/int_t.H"
#include <tuple>
#include <limits>

namespace mp {

using std::tuple;
using nil = tuple<>;

template <class T> constexpr bool nilp = std::is_same<nil, T>::value;
template <class A> constexpr int len = std::tuple_size<A>::value;

template <class A, class B> struct Cons;
template <class A0, class ... A>
struct Cons<A0, tuple<A ...>>
{
    using type = tuple<A0, A ...>;
};
template <class A, class B>
using Cons_ = typename Cons<A, B>::type;

template <class A, class B> struct Append;
template <class ... A, class ... B>
struct Append<tuple<A ...>, tuple<B ...>>
{
    using type = tuple<A ..., B ...>;
};
template <class A, class B>
using Append_ = typename Append<A, B>::type;

template <class A, class B> struct Zip;

template <class ... A, class ... B>
struct Zip<tuple<A ...>, tuple<B ...>>
{
    using type = tuple<tuple<A, B> ...>;
};
template <class A, class B> using Zip_ = typename Zip<A, B>::type;

// The list is [a .. a+n).
template <int n, int a=0, int step=1>
struct Iota
{
    static_assert(n>=0, "bad size for Iota");
    using type = typename Cons<int_t<a>, typename Iota<n-1, a+step, step>::type>::type;
};
template <int a, int step>
struct Iota<0, a, step>
{
    using type = nil;
};
template <int n, int a=0, int step=1>
using Iota_ = typename Iota<n, a, step>::type;

template <int n, class T>
struct MakeList
{
    static_assert(n>=0, "bad size for MakeList");
    using type = typename Cons<T, typename MakeList<n-1, T>::type>::type;
};
template <class T>
struct MakeList<0, T>
{
    using type = nil;
};
template <int n, class T>
using MakeList_ = typename MakeList<n, T>::type;

// Subscript a list (of lists (of lists ...)).
// @param A     a list (of lists (of lists ...)).
// @param I...  the indices.
template <class A, int ... I>
struct Ref
{
    using type = A;
};
template <class A, int I0, int ... I>
struct Ref<A, I0, I ...>
{
    static_assert(I0>=0, "bad index for Ref");
    using type = typename Ref<typename std::tuple_element<I0, A>::type, I ...>::type;
};
template <class A, int ... I> using Ref_ = typename Ref<A, I ...>::type;

template <class A>
struct First
{
    using type = Ref_<A, 0>;
};
template <class A> using First_ = typename First<A>::type;

template <class A>
struct Last
{
    using type = Ref_<A, len<A>-1>;
};
template <class A> using Last_ = typename Last<A>::type;

template <bool c, class A, class B> using If = std::conditional<c, A, B>;
template <bool c, class A, class B> using If_ = typename If<c, A, B>::type;

template <bool a>
struct When
{
    constexpr static bool value = a;
    using type = int_t<value>;
};
template <bool a>
struct Unless
{
    constexpr static bool value = !a;
    using type = int_t<value>;
};

// Return the index of a type in a type list, or -1 if not found.
template <class A, class Val, int i=0>
struct Index
{
    constexpr static int value = -1;
};
template <class ... A, class Val, int i>
struct Index<tuple<Val, A ...>, Val, i>
{
    constexpr static int value = i;
};
template <class A0, class ... A, class Val, int i>
struct Index<tuple<A0, A ...>, Val, i>
{
    constexpr static int value = Index<tuple<A ...>, Val, i+1>::value;
};

// Index (& type) of the 1st item for which Pred<> is true, or -1 (& nil).
template <class A, template <class> class Pred, int i=0, class Enable=void>
struct IndexIf
{
    constexpr static int value = -1;
    using type = nil;
};
template <class A0, class ... A, template <class> class Pred, int i>
struct IndexIf<tuple<A0, A ...>, Pred, i, std::enable_if_t<Pred<A0>::value> >
{
    using type = A0;
    constexpr static int value = i;
};
template <class A0, class ... A, template <class> class Pred, int i>
struct IndexIf<tuple<A0, A ...>, Pred, i, std::enable_if_t<!(Pred<A0>::value)> >
{
    using next = IndexIf<tuple<A ...>, Pred, i+1>;
    using type = typename next::type;
    constexpr static int value = next::value;
};

// Index (& type) of pairwise winner. A variant of Fold.
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
    using next = IndexOf<pick_i, tuple<mp::If_<i==0, T0, T1>, Ti ...>, k+1, i==0 ? sel : k>;
    using type = typename next::type;
    constexpr static int value = next::value;
};

// Return the first tail of A headed by Val, like find-tail.
template <class A, class Val> struct FindTail;
template <class Val>
struct FindTail<nil, Val>
{
    using type = nil;
};
template <class ... A, class Val>
struct FindTail<tuple<Val, A ...>, Val>
{
    using type = tuple<Val, A ...>;
};
template <class A0, class ... A, class Val>
struct FindTail<tuple<A0, A ...>, Val>
{
    using type = typename FindTail<tuple<A ...>, Val>::type;
};

// Reverse list. See TSPL^3, p. 137.
template <class A, class New=nil>
struct Reverse
{
    using type = New;
};
template <class A0, class ... A, class New>
struct Reverse<tuple<A0, A ...>, New>
{
    using type = typename Reverse<tuple<A ...>, typename Cons<A0, New>::type>::type;
};
template <class A, class New=nil>
using Reverse_ = typename Reverse<A, New>::type;

// Drop1 is needed to avoid ambiguity in the declarations of Drop, Take.
template <class A> struct Drop1;
template <class A0, class ... A>
struct Drop1<tuple<A0, A ...>>
{
    using type = tuple<A ...>;
};
template <class A>
using Drop1_ = typename Drop1<A>::type;

template <class A, int n>
struct Drop
{
    static_assert(n>=0, "bad drop size");
    using type = typename Drop<typename Drop1<A>::type, n-1>::type;
};
template <class A>
struct Drop<A, 0>
{
    using type = A;
};
template <class A, int n>
using Drop_ = typename Drop<A, n>::type;

template <class A, int n>
struct Take
{
    static_assert(n>=0, "bad take size");
    using type = typename Cons<First_<A>,
                               typename Take<typename Drop1<A>::type, n-1>::type>::type;
};
template <class A>
struct Take<A, 0>
{
    using type = nil;
};
template <class A, int n>
using Take_ = typename Take<A, n>::type;

// As apply, or as ApplyV<Valuer<>>
template <template <class ... A> class F, class L>
struct Apply
{
    static_assert(len<L> >= 0, "arg must be a tuple");
};
template <template <class ... A> class F, class ... L>
struct Apply<F, tuple<L ...>>
{
    using type = typename F<L ...>::type;
};
template <template <class ... A> class F, class L>
using Apply_ = typename Apply<F, L>::type;

// As apply, or as Apply<Typer<>>
template <template <class ... A> class F, class L>
struct ApplyV
{
    static_assert(len<L> >= 0, "arg must be a tuple"); // ??
};
template <template <class ... A> class F, class ... L>
struct ApplyV<F, tuple<L ...>>
{
    static int const value = F<L ...>::value;
};

// As map.
template <template <class ... A> class F, class ... L>
struct Map
{
    using type = typename Cons<typename F<First_<L> ...>::type,
                               typename Map<F, typename Drop1<L>::type ...>::type>::type;
};
template <template <class ... A> class F, class ... L>
struct Map<F, nil, L ...>
{
    using type = nil;
};
template <template <class ... A> class F>
struct Map<F>
{
    using type = nil;
};
template <template <class ... A> class F, class ... L>
using Map_ = typename Map<F, L ...>::type;

template <class A, class B> struct Filter
{
    using type = mp::Append_<mp::If_<mp::First_<A>::value, mp::Take_<B, 1>, mp::nil>,
                             typename Filter<mp::Drop1_<A>, mp::Drop1_<B> >::type>;
};
template <class B> struct Filter<mp::nil, B>
{
    using type = B;
};
template <class A, class B>
using Filter_ = typename Filter<A, B>::type;

// As SRFI-1 fold (= fold-left).
template <template <class ... A> class F, class Def, class ... L>
struct Fold
{
    using def = If_<std::is_same<void, Def>::value, F<>, Def>;
    using type = typename Fold<F,
                               typename F<def, First_<L> ...>::type,
                               typename Drop1<L>::type ...>::type;
};
template <template <class ... A> class F, class Def, class ... L>
struct Fold<F, Def, nil, L ...>
{
    using type = If_<std::is_same<void, Def>::value, F<>, Def>;
};
template <template <class ... A> class F, class Def>
struct Fold<F, Def>
{
    using type = If_<std::is_same<void, Def>::value, F<>, Def>;
};
template <template <class ... A> class F, class Def, class ... L>
using Fold_ = typename Fold<F, Def, L ...>::type;

// Pass F(A) to templates expecting F(A ...). @todo Wait for C++ to be fixed.
template <template <class A> class F>
struct Coerce1
{
    template <class ... B> struct type_; // @bug ---breaks composability
    template <class B0, class ... B>
    struct type_<B0, B ...>
    {
        using type = typename F<B0>::type;
    };
};

// Pass F(A, B) to templates expecting F(A ...). @todo Wait for C++ to be fixed.
template <template <class A, class B> class F>
struct Coerce2
{
    template <class ... B> struct type_; // @bug ---breaks composability
    template <class B0, class B1, class ... B>
    struct type_<B0, B1, B ...>
    {
        using type = typename F<B0, B1>::type;
    };
};

// Pass F(A ...) -> type to/from templates expecting F(A ...) -> value.
template <template <class ... A> class F>
struct Valuer
{
    template <class ... B>
    struct type
    {
        constexpr static int value = F<B ...>::type::value;
    };
};

template <template <class ... A> class F>
struct Typer
{
    template <class ... B>
    struct type_ // @bug ---breaks composability
    {
        using type = int_t<F<B ...>::value>;
    };
};

// Operations on int_t arguments.
template <class ... A> struct Sum
{
    constexpr static int value = (A::value + ... + 0);
    using type = int_t<value>;
};

template <class ... A> struct Max
{
    constexpr static int value = std::numeric_limits<int>::min();
    using type = int_t<value>;
};
template <class A0, class ... A>
struct Max<A0, A ...>
{
    constexpr static int value = std::max(A0::value, Max<A ...>::type::value);
    using type = int_t<value>;
};

template <class ... A> struct Min
{
    constexpr static int value = std::numeric_limits<int>::max();
    using type = int_t<value>;
};
template <class A0, class ... A>
struct Min<A0, A ...>
{
    constexpr static int value = std::min(A0::value, Min<A ...>::type::value);
    using type = int_t<value>;
};

template <class ... A> struct Prod
{
    constexpr static int value = (A::value * ... * 1);
    using type = int_t<value>;
};

template <class ... A> struct And
{
    constexpr static bool value = (A::value && ...);
    using type = bool_t<value>;
};

template <class ... A> struct Or
{
    constexpr static bool value = (A::value || ...);
    using type = bool_t<value>;
};

// Remove from the second list the elements of the first list. None may have repeated elements, but they may be unsorted.
template <class S, class T, class SS=S>
struct ComplementList;
// end of T.
template <class S, class SS>
struct ComplementList<S, nil, SS>
{
    using type = nil;
};
// end search on S, did not find.
template <class T0, class ... T, class SS>
struct ComplementList<nil, tuple<T0, T ...>, SS>
{
    using type = typename Cons<T0, typename ComplementList<SS, tuple<T ...>>::type>::type;
};
// end search on S, found.
template <class F, class ... S, class ... T, class SS>
struct ComplementList<tuple<F, S ...>, tuple<F, T ...>, SS>
{
    using type = typename ComplementList<SS, tuple<T ...>>::type;
};
// keep searching on S.
template <class S0, class ... S, class T0, class ... T, class SS>
struct ComplementList<tuple<S0, S ...>, tuple<T0, T ...>, SS>
{
    using type = typename ComplementList<tuple<S ...>, tuple<T0, T ...>, SS>::type;
};

template <class S, class T, class SS=S>
using ComplementList_ = typename ComplementList<S, T, SS>::type;

// Like ComplementList, but assume that both lists are sorted.
template <class S, class T>
struct ComplementSList
{
    using type = nil;
};
template <class T>
struct ComplementSList<nil, T>
{
    using type = T;
};
template <class F, class ... S, class ... T>
struct ComplementSList<tuple<F, S ...>, tuple<F, T ...>>
{
    using type = typename ComplementSList<tuple<S ...>, tuple<T ...>>::type;
};
template <class S0, class ... S, class T0, class ... T>
struct ComplementSList<tuple<S0, S ...>, tuple<T0, T ...>>
{
    static_assert(T0::value<=S0::value, "bad lists for ComplementSList<>");
    using type = typename Cons<T0, typename ComplementSList<tuple<S0, S ...>, tuple<T ...>>::type>::type;
};

// Variant of ComplementList where the second argument is [0 .. end-1].
template <class S, int end>
struct Complement
{
    using type = typename ComplementSList<S, typename Iota<end>::type>::type;
};
template <class S, int end>
using Complement_ = typename Complement<S, end>::type;

// Prepend an element to each of a list of lists.
template <class c, class A> struct MapCons;
template <class c, class ... A>
struct MapCons<c, tuple<A ...>>
{
    using type = tuple<typename Cons<c, A>::type ...>;
};

// Prepend a list to each list in a list of lists.
template <class c, class A> struct MapPrepend;
template <class c, class ... A>
struct MapPrepend<c, tuple<A ...>>
{
    using type = tuple<typename Append<c, A>::type ...>;
};

// Form all possible lists by prepending an element of A to an element of B.
template <class A, class B> struct ProductAppend
{
    using type = nil;
};
template <class A0, class ... A, class B>
struct ProductAppend<tuple<A0, A ...>, B>
{
    using type = typename Append<typename MapPrepend<A0, B>::type,
                                 typename ProductAppend<tuple<A ...>, B>::type
                                 >::type;
};

// Compute the K-combinations of the N elements of A.
//
// @param A  type list.
// @param K  take combinations of length K.
template <class A, int K, int N=len<A>>
struct Combinations
{
    static_assert(N>=0 && K>=0, "N and K must be positive in Combinations");
    static_assert(K<=N, "K cannot be > N in Combinations of N elements by K");
    using Rest = typename Drop1<A>::type;
    using type = typename Append
// with the first element and the (N-1 K-1) combinations on the rest.
        <typename MapCons<First_<A>,
                          typename Combinations<Rest, K-1, N-1>::type>::type,
// combinations on the rest, which are another (N-1 K).
         typename Combinations<Rest, K, N-1>::type
        >::type;
};
// In this case, return a list with one element: the null list.
template <class A, int N>
struct Combinations<A, 0, N>
{
    using type = tuple<nil>;
};
// In this case, return a list with one element: the whole list.
template <class A, int N>
struct Combinations<A, N, N>
{
    using type = tuple<A>;
};
// Special case for 0 over 0, to resolve ambiguity of 0/N and N/N when N=0.
template <>
struct Combinations<nil, 0>
{
    using type = tuple<nil>;
};
template <class A, int k, int N=len<A>>
using Combinations_ = typename Combinations<A, k, N>::type;

// Sign of permutations.
template <class C, class R> struct PermutationSign;

template <int w, class C, class R>
struct PermutationSignIfFound
{
    constexpr static int value
        = ((w & 1) ? -1 : +1)
          * PermutationSign<typename Append<typename Take<C, w>::type,
                                            typename Drop<C, w+1>::type>::type,
                            typename Drop1<R>::type>::value;
};
template <class C, class R>
struct PermutationSignIfFound<-1, C, R>
{
    constexpr static int value = 0;
};
template <>
struct PermutationSign<nil, nil>
{
    constexpr static int value = 1;
};
template <class C>
struct PermutationSign<C, nil>
{
    constexpr static int value = 0;
};
template <class R>
struct PermutationSign<nil, R>
{
    constexpr static int value = 0;
};
template <class C, class Org>
struct PermutationSign
{
    constexpr static int value
        = PermutationSignIfFound<Index<C, First_<Org>>::value,
                                 C, Org>::value;
};

template <int ... I>
using int_list = tuple<int_t<I> ...>;

// increment the w-th element of an int_t<> list.
template <class L, int w>
struct Inc
{
    using type = typename
        Append<typename Take<L, w>::type,
               typename Cons<int_t<Ref_<L, w>::value+1>,
                             typename Drop<L, w+1>::type>::type>::type;
};

template <class A> struct InvertIndex;
template <class ... A> struct InvertIndex<std::tuple<A ...> >
{
    using AT = std::tuple<A ...>;
    template <class T> struct IndexA
    {
        using type = int_t<Index<AT, T>::value>;
    };
    constexpr static int N = Apply_<Max, AT>::value;
    using type = Map_<IndexA, Iota_<(N>=0 ? N+1 : 0)> >;
};
template <class A> using InvertIndex_ = typename InvertIndex<A>::type;

template <class A> struct index_tuple;
template <class ... A>
struct index_tuple<tuple<A ...>>
{
    using type = typename Iota<sizeof...(A)>::type;
};
template <class A>
using index_tuple_ = typename index_tuple<A>::type;


// Used in tests.
template <class A, int ... I>
struct check_idx
{
    constexpr static bool value = false;
};

template <>
struct check_idx<nil>
{
    constexpr static bool value = true;
};

template <class A0, int I0, class ... A, int ... I>
struct check_idx<std::tuple<A0, A ...>, I0, I ...>
{
    constexpr static bool value = (A0::value==I0)
                              && check_idx<std::tuple<A ...>, I ...>::value;
};

} // namespace mp

#endif // RA_TUPLELIST_H 561
