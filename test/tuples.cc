// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Test type list library based on tuples.

// (c) Daniel Llorens - 2010
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::tuple, std::tuple_element, std::is_same_v;
using std::cout, std::endl, ra::TestRecorder;
using ra::int_c, ra::mp::ref, ra::ilist_t;

template <class A>
struct Inc1
{
    using type = int_c<A::value+1>;
};

template <class A, class B>
struct Sum2
{
    using type = int_c<A::value+B::value>;
};

template <class ... A> struct SumV;
template <class A0, class ... A>
struct SumV<A0, A ...>
{
    constexpr static int value = A0::value + SumV<A ...>::value;
};
template <>
struct SumV<>
{
    constexpr static int value = 0;
};

template <class A>
struct SamePP
{
    template <class B>
    struct type
    {
        constexpr static bool value = is_same_v<A, B>;
    };
};

struct True
{
    constexpr static bool value = true;
};

// These used to be in base.hh, but then I thought it wasn't worth having them there. Some are tested below.
namespace ra::mp {

template <class ... A> using sum = int_c<(A::value + ... + 0)>;
template <class ... A> using prod = int_c<(A::value * ... * 1)>;
template <class ... A> using andb = ic_t<(A::value && ...)>;
template <class ... A> using orb = ic_t<(A::value || ...)>;

// increment L[w]
template <class L, int w> using inc = append<take<L, w>, cons<int_c<ref<L, w>::value+1>, drop<L, w+1>>>;

template <bool a> using when = ic_t<a>;
template <bool a> using unless = ic_t<(!a)>;

// like fold-left
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

template <class ... A> using max = int_c<[]() { int r=std::numeric_limits<int>::min(); ((r=std::max(r, A::value)), ...); return r; }()>;
template <class ... A> using min = int_c<[]() { int r=std::numeric_limits<int>::max(); ((r=std::min(r, A::value)), ...); return r; }()>;

template <class A> struct InvertIndex_;
template <class ... A> struct InvertIndex_<tuple<A ...>>
{
    using AT = tuple<A ...>;
    template <class T> using IndexA = int_c<index<AT, T>::value>;
    constexpr static int N = apply<max, AT>::value;
    using type = map<IndexA, iota<(N>=0 ? N+1 : 0)>>;
};
template <class A> using InvertIndex = typename InvertIndex_<A>::type;

} // namespace ra::mp

int main()
{
    TestRecorder tr(std::cout);
// Booleans.
    {
        static_assert(True::value, "bad True");
    }
// Map
    {
        using A = ilist_t<5, 6, 3>;
        using B = ilist_t<2, 3, -1>;
        static_assert(ra::mp::check_idx<ra::mp::map<ra::mp::sum, A>, 5, 6, 3>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::map<ra::mp::sum, A, B>, 7, 9, 2>::value, "");
    }
// fold.
    {
        using A = ilist_t<5, 6, 3>;
        using B = ilist_t<2, 3, -1>;
        static_assert(ra::mp::fold<ra::mp::sum, int_c<1>, A>::value==15, "");
        static_assert(ra::mp::fold<ra::mp::sum, int_c<3>, B>::value==7, "");
        static_assert(ra::mp::fold<ra::mp::max, int_c<4>, A>::value==6, "");
        static_assert(ra::mp::fold<ra::mp::max, void, A>::value==6, "");
        static_assert(ra::mp::fold<ra::mp::max, int_c<9>, A>::value==9, "");
        static_assert(ra::mp::fold<ra::mp::min, int_c<4>, A>::value==3, "");
        static_assert(ra::mp::fold<ra::mp::min, void, A>::value==3, "");
        static_assert(ra::mp::fold<ra::mp::min, int_c<1>, A>::value==1, "");
    }
// Reductions.
    {
        using list_ = ilist_t<>;
        using list_1 = ilist_t<1>;
        using list_0 = ilist_t<0>;
        using list_10 = ilist_t<1, 0>;
        using list_01 = ilist_t<0, 1>;
        using list_11 = ilist_t<1, 1>;
        using list_00 = ilist_t<0, 0>;
        static_assert(ra::mp::apply<ra::mp::andb, list_>::value, "bad And");
        static_assert(ra::mp::apply<ra::mp::andb, list_1>::value, "bad And");
        static_assert(!ra::mp::apply<ra::mp::andb, list_0>::value, "bad And");
        static_assert(!ra::mp::apply<ra::mp::andb, list_10>::value, "bad And");
        static_assert(!ra::mp::apply<ra::mp::andb, list_01>::value, "bad And");
        static_assert(ra::mp::apply<ra::mp::andb, list_11>::value, "bad And");
        static_assert(!ra::mp::apply<ra::mp::andb, list_00>::value, "bad And");
        static_assert(!ra::mp::apply<ra::mp::orb, list_>::value, "bad Or");
        static_assert(ra::mp::apply<ra::mp::orb, list_1>::value, "bad Or");
        static_assert(!ra::mp::apply<ra::mp::orb, list_0>::value, "bad Or");
        static_assert(ra::mp::apply<ra::mp::orb, list_10>::value, "bad Or");
        static_assert(ra::mp::apply<ra::mp::orb, list_01>::value, "bad Or");
        static_assert(ra::mp::apply<ra::mp::orb, list_11>::value, "bad Or");
        static_assert(!ra::mp::apply<ra::mp::orb, list_00>::value, "bad Or");
        static_assert(ra::mp::apply<ra::mp::sum, list_>::value==0, "bad Sum");
        static_assert(ra::mp::apply<ra::mp::sum, ilist_t<2, 4>>::value==6, "bad Sum");
        static_assert(ra::mp::apply<ra::mp::prod, list_>::value==1, "bad Prod");
        static_assert(ra::mp::apply<ra::mp::prod, ilist_t<2, 3>>::value==6, "bad Prod");
    }
// append.
    using A = ilist_t<0, 2, 3>;
    using B = ilist_t<5, 6, 7>;
    using C = ilist_t<9, 8>;
    static_assert(is_same_v<ilist_t<>, ra::mp::nil>, "");
    static_assert(is_same_v<C, tuple<int_c<9>, int_c<8>>>, "");
    using O = ra::mp::nil;
    using A_B = ra::mp::append<A, B>;
    using A_C = ra::mp::append<A, C>;
    using C_B = ra::mp::append<C, B>;
    static_assert(ra::mp::check_idx<A_B, 0, 2, 3, 5, 6, 7>::value, "bad AB");
    static_assert(ra::mp::check_idx<A_C, 0, 2, 3, 9, 8>::value, "bad AB");
    static_assert(ra::mp::check_idx<C_B, 9, 8, 5, 6, 7>::value, "bad AB");
    static_assert(is_same_v<A, ra::mp::append<A, O>>, "bad A+empty");
    static_assert(is_same_v<A, ra::mp::append<O, A>>, "bad empty+A");
// mp::iota.
    static_assert(ra::mp::check_idx<ra::mp::iota<4, 0>, 0, 1, 2, 3>::value, "0a");
    static_assert(ra::mp::check_idx<ra::mp::iota<4, 3>, 3, 4, 5, 6>::value, "0b");
    static_assert(ra::mp::check_idx<ra::mp::iota<0, 3>>::value, "0c");
    static_assert(ra::mp::check_idx<ra::mp::iota<1, 3>, 3>::value, "0d");
    static_assert(ra::mp::check_idx<ra::mp::iota<3, -2>, -2, -1, 0>::value, "0e");
    static_assert(ra::mp::check_idx<ra::mp::iota<4, 3, -1>, 3, 2, 1, 0>::value, "0a");
// makelist
    static_assert(ra::mp::check_idx<ra::mp::makelist<2, int_c<9>>, 9, 9>::value, "1a");
    static_assert(ra::mp::check_idx<ra::mp::makelist<0, int_c<9>>>::value, "1b");
// ref
    static_assert(ref<tuple<A, B, C>, 0, 0>::value==0, "3a");
    static_assert(ref<tuple<A, B, C>, 0, 1>::value==2, "3b");
    static_assert(ref<tuple<A, B, C>, 0, 2>::value==3, "3c");
    static_assert(ref<tuple<A, B, C>, 1, 0>::value==5, "3d");
    static_assert(ref<tuple<A, B, C>, 1, 1>::value==6, "3e");
    static_assert(ref<tuple<A, B, C>, 1, 2>::value==7, "3f");
    static_assert(ref<tuple<A, B, C>, 2, 0>::value==9, "3g");
    static_assert(ref<tuple<A, B, C>, 2, 1>::value==8, "3h");
    static_assert(ra::mp::first<B>::value==5 && ra::mp::first<C>::value==9, "3i");
    static_assert(ra::mp::last<B>::value==7 && ra::mp::last<C>::value==8, "3j");
// Useful default.
    static_assert(ra::mp::len<ref<ra::mp::nil>> ==0, "3i");
// 3-indices.
    using S2AB = tuple<A, B>;
    using S2BC = tuple<B, C>;
    using S3 = tuple<S2AB, S2BC>;
// in S2AB.
    static_assert(ref<S3, 0, 0, 0>::value==0, "3j");
    static_assert(ref<S3, 0, 0, 1>::value==2, "3k");
    static_assert(ref<S3, 0, 0, 2>::value==3, "3l");
    static_assert(ref<S3, 0, 1, 0>::value==5, "3m");
    static_assert(ref<S3, 0, 1, 1>::value==6, "3n");
    static_assert(ref<S3, 0, 1, 2>::value==7, "3o");
// in S2BC.
    static_assert(ref<S3, 1, 0, 0>::value==5, "3p");
    static_assert(ref<S3, 1, 0, 1>::value==6, "3q");
    static_assert(ref<S3, 1, 0, 2>::value==7, "3r");
    static_assert(ref<S3, 1, 1, 0>::value==9, "3s");
    static_assert(ref<S3, 1, 1, 1>::value==8, "3t");
// index.
    static_assert(ra::mp::index<A, int_c<0>>::value==0, "4a");
    static_assert(ra::mp::index<A, int_c<2>>::value==1, "4b");
    static_assert(ra::mp::index<A, int_c<3>>::value==2, "4c");
    static_assert(ra::mp::index<A, int_c<4>>::value==-1, "4d");
    static_assert(ra::mp::index<S3, S2BC>::value==1, "4e");
// InvertIndex
    {
        using II0 = ilist_t<4, 6, 7, 1>;
        using II1 = ra::mp::InvertIndex<II0>;
        static_assert(is_same_v<ilist_t<-1, 3, -1, -1, 0, -1, 1, 2>, II1>);
    }
    {
        using II0 = ilist_t<3>;
        using II1 = ra::mp::InvertIndex<II0>;
        static_assert(is_same_v<ilist_t<-1, -1, -1, 0>, II1>);
    }
    {
        using II0 = ilist_t<>;
        using II1 = ra::mp::InvertIndex<II0>;
        static_assert(is_same_v<ilist_t<>, II1>);
    }
// indexif.
    static_assert(ra::mp::indexif<A, SamePP<int_c<0>>::type>::value==0, "5a");
    static_assert(ra::mp::indexif<A, SamePP<int_c<2>>::type>::value==1, "5b");
    static_assert(ra::mp::indexif<A, SamePP<int_c<3>>::type>::value==2, "5c");
    static_assert(ra::mp::indexif<A, SamePP<int_c<9>>::type>::value==-1, "5d");
// findtail.
    static_assert(is_same_v<ra::mp::findtail<A, int_c<0>>, A>, "4a");
    static_assert(ra::mp::check_idx<ra::mp::findtail<A, int_c<2>>, 2, 3>::value, "4b");
    static_assert(ra::mp::check_idx<ra::mp::findtail<A, int_c<3>>, 3>::value, "4c");
    static_assert(std::is_same_v<ra::mp::nil, ra::mp::findtail<A, int_c<4>>>, "4d");
    static_assert(is_same_v<ra::mp::findtail<S3, S2BC>, tuple<S2BC>>, "4e");
// reverse.
    static_assert(ra::mp::check_idx<ra::mp::reverse<A_B>, 7, 6, 5, 3, 2, 0>::value, "5a");
    static_assert(ra::mp::check_idx<ra::mp::reverse<O>>::value, "5b");
    static_assert(is_same_v<ra::mp::reverse<ra::mp::nil>, ra::mp::nil>, "bad reverse");
// drop & take
    static_assert(ra::mp::check_idx<ra::mp::drop<A, 0>, 0, 2, 3>::value, "bad 6a");
    static_assert(ra::mp::check_idx<ra::mp::drop<A, 1>, 2, 3>::value, "bad 6b");
    static_assert(ra::mp::check_idx<ra::mp::drop<A, 2>, 3>::value, "bad 6c");
    static_assert(ra::mp::check_idx<ra::mp::drop<A, 3>>::value, "bad 6d");
    static_assert(ra::mp::check_idx<ra::mp::take<A, 0>>::value, "bad 6e");
    static_assert(ra::mp::check_idx<ra::mp::take<A, 1>, 0>::value, "bad 6f");
    static_assert(ra::mp::check_idx<ra::mp::take<A, 2>, 0, 2>::value, "bad 6g");
    static_assert(ra::mp::check_idx<ra::mp::take<A, 3>, 0, 2, 3>::value, "bad 6h");
// complement.
    {
        using case1 = ilist_t<1>;
        static_assert(ra::mp::check_idx<ra::mp::complement<case1, 0>>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::complement<case1, 1>, 0>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::complement<case1, 2>, 0>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::complement<case1, 3>, 0, 2>::value, "");
        using list3 = ra::mp::iota<3>;
        static_assert(ra::mp::check_idx<ra::mp::complement<list3, 3>>::value, "");
        using c36 = ra::mp::complement<list3, 6>;
        static_assert(ra::mp::check_idx<c36, 3, 4, 5>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::complement<c36, 6>, 0, 1, 2>::value, "");
        using case0 = tuple<int_c<0>>;
        static_assert(ra::mp::check_idx<ra::mp::complement<case0, 0>>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::complement<case0, 1>>::value, "");
        static_assert(ra::mp::check_idx<ra::mp::complement<case0, 2>, 1>::value, "");
    }
// complement_sorted_list && complement_list.
    {
#define _ ,
#define CHECK_COMPLEMENT_SLIST( A, B, C ) \
static_assert(ra::mp::check_idx<ra::mp::complement_sorted_list<ilist_t A , B > C >::value, "a");
        CHECK_COMPLEMENT_SLIST( <1>, ilist_t<>, )
        CHECK_COMPLEMENT_SLIST( <1>, ilist_t<0>, _ 0)
        CHECK_COMPLEMENT_SLIST( <1>, ilist_t<0 _ 1>, _ 0)
        CHECK_COMPLEMENT_SLIST( <1>, ilist_t<0 _ 1 _ 2>, _ 0 _ 2)
        using l2 = ra::mp::iota<2>;
        CHECK_COMPLEMENT_SLIST( <0>,     l2,  _ 1 )
        CHECK_COMPLEMENT_SLIST( <1>,     l2,  _ 0 )
        CHECK_COMPLEMENT_SLIST( <>,      l2,  _ 0 _ 1 )
        using l3 = ra::mp::iota<3>;
        CHECK_COMPLEMENT_SLIST( <0 _ 1>, l3,  _ 2 )
        CHECK_COMPLEMENT_SLIST( <0 _ 2>, l3,  _ 1 )
        CHECK_COMPLEMENT_SLIST( <1 _ 2>, l3,  _ 0 )
        CHECK_COMPLEMENT_SLIST( <0>,     l3,  _ 1 _ 2 )
        CHECK_COMPLEMENT_SLIST( <1>,     l3,  _ 0 _ 2 )
        CHECK_COMPLEMENT_SLIST( <2>,     l3,  _ 0 _ 1 )
        CHECK_COMPLEMENT_SLIST( <>,      l3,  _ 0 _ 1 _ 2 )
        CHECK_COMPLEMENT_SLIST( <>,      ra::mp::nil, )
#undef CHECK_COMPLEMENT_SLIST
#undef _
#define _ ,
#define CHECK_COMPLEMENT_LIST( A, B, C ) \
static_assert(ra::mp::check_idx<ra::mp::complement_list<ilist_t A , B > C >::value, "a");
        using l2 = ra::mp::iota<2>;
        CHECK_COMPLEMENT_LIST( <0>,     l2,  _ 1 )
        CHECK_COMPLEMENT_LIST( <1>,     l2,  _ 0 )
        CHECK_COMPLEMENT_LIST( <>,      l2,  _ 0 _ 1 )
        using l3 = ra::mp::iota<3>;
        CHECK_COMPLEMENT_LIST( <0 _ 1>, l3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <0 _ 2>, l3,  _ 1 )
        CHECK_COMPLEMENT_LIST( <1 _ 2>, l3,  _ 0 )
        CHECK_COMPLEMENT_LIST( <0>,     l3,  _ 1 _ 2 )
        CHECK_COMPLEMENT_LIST( <1>,     l3,  _ 0 _ 2 )
        CHECK_COMPLEMENT_LIST( <2>,     l3,  _ 0 _ 1 )
        CHECK_COMPLEMENT_LIST( <>,      l3,  _ 0 _ 1 _ 2 )
        CHECK_COMPLEMENT_LIST( <>,      ra::mp::nil, )
// this must also work on unserted lists.
        CHECK_COMPLEMENT_LIST( <1 _ 0>, l3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <2 _ 1>, l3,  _ 0 )
        CHECK_COMPLEMENT_LIST( <2 _ 0>, l3,  _ 1 )
        using x3 = ilist_t<2, 0, 1>;
        CHECK_COMPLEMENT_LIST( <1 _ 0>, x3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <2 _ 0>, x3,  _ 1 )
        CHECK_COMPLEMENT_LIST( <2 _ 1>, x3,  _ 0 )
#undef CHECK_COMPLEMENT_LIST
#undef _
    }
// mapcons.
    {
        using a = ra::mp::iota<2>;
        using b = ra::mp::iota<2, 1>;
        using mc = ra::mp::mapcons<int_c<9>, tuple<a, b>>;
        static_assert(ra::mp::check_idx<ref<mc, 0>, 9, 0, 1>::value, "a");
        static_assert(ra::mp::check_idx<ref<mc, 1>, 9, 1, 2>::value, "b");
    }
// Combinations.
    {
        static_assert(ra::mp::len<ra::mp::combs<ra::mp::nil, 0>> == 1, "");
        using l3 = ra::mp::iota<3>;
        using c31 = ra::mp::combs<l3, 1>;
        using c32 = ra::mp::combs<l3, 2>;
        static_assert(ra::mp::len<c31> == 3, "a");
        static_assert(ra::mp::check_idx<ref<c31, 0>, 0>::value, "a");
        static_assert(ra::mp::check_idx<ref<c31, 1>, 1>::value, "b");
        static_assert(ra::mp::check_idx<ref<c31, 2>, 2>::value, "c");
        static_assert(ra::mp::len<c32> == 3, "b");
        static_assert(ra::mp::check_idx<ref<c32, 0>, 0, 1>::value, "d");
        static_assert(ra::mp::check_idx<ref<c32, 1>, 0, 2>::value, "e");
        static_assert(ra::mp::check_idx<ref<c32, 2>, 1, 2>::value, "f");
        using l4 = ra::mp::iota<4>;
        using c40 = ra::mp::combs<l4, 0>;
        using c41 = ra::mp::combs<l4, 1>;
        using c42 = ra::mp::combs<l4, 2>;
        using c43 = ra::mp::combs<l4, 3>;
        using c44 = ra::mp::combs<l4, 4>;
        static_assert(ra::mp::len<c40> == 1, "a");
        static_assert(ra::mp::check_idx<ref<c40, 0>>::value, "a");
        static_assert(ra::mp::len<c41> == 4, "b");
        static_assert(ra::mp::check_idx<ref<c41, 0>, 0>::value, "b");
        static_assert(ra::mp::check_idx<ref<c41, 1>, 1>::value, "b");
        static_assert(ra::mp::check_idx<ref<c41, 2>, 2>::value, "b");
        static_assert(ra::mp::check_idx<ref<c41, 3>, 3>::value, "b");
        static_assert(ra::mp::len<c42> == 6, "c");
        static_assert(ra::mp::check_idx<ref<c42, 0>, 0, 1>::value, "c");
        static_assert(ra::mp::check_idx<ref<c42, 1>, 0, 2>::value, "c");
        static_assert(ra::mp::check_idx<ref<c42, 2>, 0, 3>::value, "c");
        static_assert(ra::mp::check_idx<ref<c42, 3>, 1, 2>::value, "c");
        static_assert(ra::mp::check_idx<ref<c42, 4>, 1, 3>::value, "c");
        static_assert(ra::mp::check_idx<ref<c42, 5>, 2, 3>::value, "c");
        static_assert(ra::mp::len<c43> == 4, "d");
        static_assert(ra::mp::check_idx<ref<c43, 0>, 0, 1, 2>::value, "d");
        static_assert(ra::mp::check_idx<ref<c43, 1>, 0, 1, 3>::value, "d");
        static_assert(ra::mp::check_idx<ref<c43, 2>, 0, 2, 3>::value, "d");
        static_assert(ra::mp::check_idx<ref<c43, 3>, 1, 2, 3>::value, "d");
        static_assert(ra::mp::len<c44> == 1, "e");
        static_assert(ra::mp::check_idx<ref<c44, 0>, 0, 1, 2, 3>::value, "e");
    }
// mapprepend & prodappend.
    {
        using la = ra::mp::iota<3>;
        using ca = ra::mp::combs<la, 1>;
        using lb = ra::mp::iota<3>;
        using cb = ra::mp::combs<lb, 1>;
        using test0 = ra::mp::mapprepend<ra::mp::nil, cb>;
        static_assert(is_same_v<test0, cb>, "");
        using test1 = ra::mp::mapprepend<la, cb>;
        static_assert(ra::mp::len<test1> == int(ra::mp::len<cb>), "");
        static_assert(ra::mp::check_idx<ref<test1, 0>, 0, 1, 2, 0>::value, "");
        static_assert(ra::mp::check_idx<ref<test1, 1>, 0, 1, 2, 1>::value, "");
        static_assert(ra::mp::check_idx<ref<test1, 2>, 0, 1, 2, 2>::value, "");

        using test2 = ra::mp::prodappend<ca, cb>;
        static_assert(ra::mp::len<test2> == 9, "");
        static_assert(ra::mp::check_idx<ref<test2, 0>, 0, 0>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 1>, 0, 1>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 2>, 0, 2>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 3>, 1, 0>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 4>, 1, 1>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 5>, 1, 2>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 6>, 2, 0>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 7>, 2, 1>::value, "");
        static_assert(ra::mp::check_idx<ref<test2, 8>, 2, 2>::value, "");
    }
// permsign.
    {
#define _ ,
#define CHECK_PERM_SIGN( A, B, C ) \
static_assert(ra::mp::permsign<ilist_t A , ilist_t B >::value == C, "");
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <0 _ 1 _ 2>, +1 );
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <0 _ 2 _ 1>, -1 );
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <1 _ 2 _ 0>, +1 );
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <2 _ 1 _ 0>, -1 );
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <1 _ 0 _ 2>, -1 );
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <2 _ 0 _ 1>, +1 );
// inverted.
        CHECK_PERM_SIGN( <0 _ 1 _ 2>, <0 _ 1 _ 2>, +1 );
        CHECK_PERM_SIGN( <0 _ 2 _ 1>, <0 _ 1 _ 2>, -1 );
        CHECK_PERM_SIGN( <1 _ 2 _ 0>, <0 _ 1 _ 2>, +1 );
        CHECK_PERM_SIGN( <2 _ 1 _ 0>, <0 _ 1 _ 2>, -1 );
        CHECK_PERM_SIGN( <1 _ 0 _ 2>, <0 _ 1 _ 2>, -1 );
        CHECK_PERM_SIGN( <2 _ 0 _ 1>, <0 _ 1 _ 2>, +1 );
// other cases.
        CHECK_PERM_SIGN( <0>, <0>, +1 );
        CHECK_PERM_SIGN( <>,  <>,  +1 );
        CHECK_PERM_SIGN( <0>, <1>,  0 );
        CHECK_PERM_SIGN( <0>, <>,   0 );
        CHECK_PERM_SIGN( <>,  <0>,  0 );
#undef CHECK_PERM_SIGN
#undef _
    }
// inc
    {
        using l = ilist_t<7, 8, 2>;
        static_assert(ra::mp::check_idx<l, 7, 8, 2>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::inc<l, 0>, 8, 8, 2>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::inc<l, 1>, 7, 9, 2>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::inc<l, 2>, 7, 8, 3>::value, "bad");
    }
// Prod & Sum
    {
        using l = ilist_t<3, 5, 7>;
        static_assert(ra::mp::apply<ra::mp::prod, l>::value==105, "bad");
        static_assert(ra::mp::apply<ra::mp::sum, l>::value==15, "bad");
    }
    return tr.summary();
};
