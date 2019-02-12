
// (c) Daniel Llorens - 2010

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file tuplelist.C
/// @brief test type list library based on tuples.

#include <iostream>
#include "ra/tuple-dynamic.H"
#include "ra/tuple-list.H"
#include "ra/mpdebug.H"
#include "ra/test.H"

using std::tuple, std::tuple_element, std::is_same_v;
using std::cout, std::endl;
using mp::int_t, mp::ref, mp::int_list;

template <class A>
struct Inc1
{
    using type = int_t<A::value+1>;
};

template <class A, class B>
struct Sum2
{
    using type = int_t<A::value+B::value>;
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

int main()
{
    TestRecorder tr(std::cout);
// Basic constexpr stuff.
    {
        static_assert(mp::fact(0)==1, "bad fact(0)");
        static_assert(mp::fact(1)==1, "bad fact(1)");
        static_assert(mp::fact(2)==2, "bad fact(2)");
        static_assert(mp::fact(3)==6, "bad fact(3)");
    }
// Booleans.
    {
        static_assert(True::value, "bad True");
    }
// Map
    {
        using A = int_list<5, 6, 3>;
        using B = int_list<2, 3, -1>;
        static_assert(mp::check_idx<mp::map<mp::sum, A>, 5, 6, 3>::value, "");
        static_assert(mp::check_idx<mp::map<mp::sum, A, B>, 7, 9, 2>::value, "");
    }
// fold.
    {
        using A = int_list<5, 6, 3>;
        using B = int_list<2, 3, -1>;
        static_assert(mp::fold<mp::sum, int_t<1>, A>::value==15, "");
        static_assert(mp::fold<mp::sum, int_t<3>, B>::value==7, "");
        static_assert(mp::fold<mp::max, int_t<4>, A>::value==6, "");
        static_assert(mp::fold<mp::max, void, A>::value==6, "");
        static_assert(mp::fold<mp::max, int_t<9>, A>::value==9, "");
        static_assert(mp::fold<mp::min, int_t<4>, A>::value==3, "");
        static_assert(mp::fold<mp::min, void, A>::value==3, "");
        static_assert(mp::fold<mp::min, int_t<1>, A>::value==1, "");
    }
// Reductions.
    {
        using list_ = int_list<>;
        using list_1 = int_list<1>;
        using list_0 = int_list<0>;
        using list_10 = int_list<1, 0>;
        using list_01 = int_list<0, 1>;
        using list_11 = int_list<1, 1>;
        using list_00 = int_list<0, 0>;
        static_assert(mp::apply<mp::andb, list_>::value, "bad And");
        static_assert(mp::apply<mp::andb, list_1>::value, "bad And");
        static_assert(!mp::apply<mp::andb, list_0>::value, "bad And");
        static_assert(!mp::apply<mp::andb, list_10>::value, "bad And");
        static_assert(!mp::apply<mp::andb, list_01>::value, "bad And");
        static_assert(mp::apply<mp::andb, list_11>::value, "bad And");
        static_assert(!mp::apply<mp::andb, list_00>::value, "bad And");
        static_assert(!mp::apply<mp::orb, list_>::value, "bad Or");
        static_assert(mp::apply<mp::orb, list_1>::value, "bad Or");
        static_assert(!mp::apply<mp::orb, list_0>::value, "bad Or");
        static_assert(mp::apply<mp::orb, list_10>::value, "bad Or");
        static_assert(mp::apply<mp::orb, list_01>::value, "bad Or");
        static_assert(mp::apply<mp::orb, list_11>::value, "bad Or");
        static_assert(!mp::apply<mp::orb, list_00>::value, "bad Or");
        static_assert(mp::apply<mp::sum, list_>::value==0, "bad Sum");
        static_assert(mp::apply<mp::sum, int_list<2, 4>>::value==6, "bad Sum");
        static_assert(mp::apply<mp::prod, list_>::value==1, "bad Prod");
        static_assert(mp::apply<mp::prod, int_list<2, 3>>::value==6, "bad Prod");
    }
// append.
    using A = int_list<0, 2, 3>;
    using B = int_list<5, 6, 7>;
    using C = int_list<9, 8>;
    static_assert(is_same_v<int_list<>, mp::nil>, "");
    static_assert(is_same_v<C, tuple<int_t<9>, int_t<8>>>, "");
    using O = mp::nil;
    using A_B = mp::append<A, B>;
    using A_C = mp::append<A, C>;
    using C_B = mp::append<C, B>;
    static_assert(mp::check_idx<A_B, 0, 2, 3, 5, 6, 7>::value, "bad AB");
    static_assert(mp::check_idx<A_C, 0, 2, 3, 9, 8>::value, "bad AB");
    static_assert(mp::check_idx<C_B, 9, 8, 5, 6, 7>::value, "bad AB");
    static_assert(is_same_v<A, mp::append<A, O>>, "bad A+empty");
    static_assert(is_same_v<A, mp::append<O, A>>, "bad empty+A");
// Iota.
    static_assert(mp::check_idx<mp::iota<4, 0>, 0, 1, 2, 3>::value, "0a");
    static_assert(mp::check_idx<mp::iota<4, 3>, 3, 4, 5, 6>::value, "0b");
    static_assert(mp::check_idx<mp::iota<0, 3>>::value, "0c");
    static_assert(mp::check_idx<mp::iota<1, 3>, 3>::value, "0d");
    static_assert(mp::check_idx<mp::iota<3, -2>, -2, -1, 0>::value, "0e");
    static_assert(mp::check_idx<mp::iota<4, 3, -1>, 3, 2, 1, 0>::value, "0a");
// makelist
    static_assert(mp::check_idx<mp::makelist<2, int_t<9>>, 9, 9>::value, "1a");
    static_assert(mp::check_idx<mp::makelist<0, int_t<9>>>::value, "1b");
// ref
    static_assert(ref<tuple<A, B, C>, 0, 0>::value==0, "3a");
    static_assert(ref<tuple<A, B, C>, 0, 1>::value==2, "3b");
    static_assert(ref<tuple<A, B, C>, 0, 2>::value==3, "3c");
    static_assert(ref<tuple<A, B, C>, 1, 0>::value==5, "3d");
    static_assert(ref<tuple<A, B, C>, 1, 1>::value==6, "3e");
    static_assert(ref<tuple<A, B, C>, 1, 2>::value==7, "3f");
    static_assert(ref<tuple<A, B, C>, 2, 0>::value==9, "3g");
    static_assert(ref<tuple<A, B, C>, 2, 1>::value==8, "3h");
    static_assert(mp::first<B>::value==5 && mp::first<C>::value==9, "3i");
    static_assert(mp::last<B>::value==7 && mp::last<C>::value==8, "3j");
// Useful default.
    static_assert(mp::len<ref<mp::nil>> ==0, "3i");
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
    static_assert(mp::index<A, int_t<0>>::value==0, "4a");
    static_assert(mp::index<A, int_t<2>>::value==1, "4b");
    static_assert(mp::index<A, int_t<3>>::value==2, "4c");
    static_assert(mp::index<A, int_t<4>>::value==-1, "4d");
    static_assert(mp::index<S3, S2BC>::value==1, "4e");
// InvertIndex
    {
        using II0 = int_list<4, 6, 7, 1>;
        using II1 = mp::InvertIndex_<II0>;
        static_assert(is_same_v<int_list<-1, 3, -1, -1, 0, -1, 1, 2>, II1>);
    }
    {
        using II0 = int_list<3>;
        using II1 = mp::InvertIndex_<II0>;
        static_assert(is_same_v<int_list<-1, -1, -1, 0>, II1>);
    }
    {
        using II0 = int_list<>;
        using II1 = mp::InvertIndex_<II0>;
        static_assert(is_same_v<int_list<>, II1>);
    }
// IndexIf.
    static_assert(mp::IndexIf<A, SamePP<int_t<0>>::type>::value==0, "5a");
    static_assert(mp::IndexIf<A, SamePP<int_t<2>>::type>::value==1, "5b");
    static_assert(mp::IndexIf<A, SamePP<int_t<3>>::type>::value==2, "5c");
    static_assert(mp::IndexIf<A, SamePP<int_t<9>>::type>::value==-1, "5d");
// findtail.
    static_assert(is_same_v<mp::findtail<A, int_t<0>>, A>, "4a");
    static_assert(mp::check_idx<mp::findtail<A, int_t<2>>, 2, 3>::value, "4b");
    static_assert(mp::check_idx<mp::findtail<A, int_t<3>>, 3>::value, "4c");
    static_assert(mp::nilp<mp::findtail<A, int_t<4>>>, "4d");
    static_assert(is_same_v<mp::findtail<S3, S2BC>, tuple<S2BC>>, "4e");
// reverse.
    static_assert(mp::check_idx<mp::reverse<A_B>, 7, 6, 5, 3, 2, 0>::value, "5a");
    static_assert(mp::check_idx<mp::reverse<O>>::value, "5b");
    static_assert(is_same_v<mp::reverse<mp::nil>, mp::nil>, "bad reverse");
// drop & take
    static_assert(mp::check_idx<mp::drop<A, 0>, 0, 2, 3>::value, "bad 6a");
    static_assert(mp::check_idx<mp::drop<A, 1>, 2, 3>::value, "bad 6b");
    static_assert(mp::check_idx<mp::drop<A, 2>, 3>::value, "bad 6c");
    static_assert(mp::check_idx<mp::drop<A, 3>>::value, "bad 6d");
    static_assert(mp::check_idx<mp::take<A, 0>>::value, "bad 6e");
    static_assert(mp::check_idx<mp::take<A, 1>, 0>::value, "bad 6f");
    static_assert(mp::check_idx<mp::take<A, 2>, 0, 2>::value, "bad 6g");
    static_assert(mp::check_idx<mp::take<A, 3>, 0, 2, 3>::value, "bad 6h");
// complement.
    {
        using case1 = int_list<1>;
        static_assert(mp::check_idx<mp::Complement_<case1, 0>>::value, "");
        static_assert(mp::check_idx<mp::Complement_<case1, 1>, 0>::value, "");
        static_assert(mp::check_idx<mp::Complement_<case1, 2>, 0>::value, "");
        static_assert(mp::check_idx<mp::Complement_<case1, 3>, 0, 2>::value, "");
        using list3 = mp::iota<3>;
        static_assert(mp::check_idx<mp::Complement_<list3, 3>>::value, "");
        using c36 = mp::Complement_<list3, 6>;
        static_assert(mp::check_idx<c36, 3, 4, 5>::value, "");
        static_assert(mp::check_idx<mp::Complement_<c36, 6>, 0, 1, 2>::value, "");
        using case0 = tuple<int_t<0>>;
        static_assert(mp::check_idx<mp::Complement_<case0, 0>>::value, "");
        static_assert(mp::check_idx<mp::Complement_<case0, 1>>::value, "");
        static_assert(mp::check_idx<mp::Complement_<case0, 2>, 1>::value, "");
    }
// ComplementSList && ComplementList.
    {
#define _ ,
#define CHECK_COMPLEMENT_SLIST( A, B, C ) \
static_assert(mp::check_idx<mp::ComplementSList_<int_list A , B > C >::value, "a");
        CHECK_COMPLEMENT_SLIST( <1>, int_list<>, )
        CHECK_COMPLEMENT_SLIST( <1>, int_list<0>, _ 0)
        CHECK_COMPLEMENT_SLIST( <1>, int_list<0 _ 1>, _ 0)
        CHECK_COMPLEMENT_SLIST( <1>, int_list<0 _ 1 _ 2>, _ 0 _ 2)
        using l2 = mp::iota<2>;
        CHECK_COMPLEMENT_SLIST( <0>,     l2,  _ 1 )
        CHECK_COMPLEMENT_SLIST( <1>,     l2,  _ 0 )
        CHECK_COMPLEMENT_SLIST( <>,      l2,  _ 0 _ 1 )
        using l3 = mp::iota<3>;
        CHECK_COMPLEMENT_SLIST( <0 _ 1>, l3,  _ 2 )
        CHECK_COMPLEMENT_SLIST( <0 _ 2>, l3,  _ 1 )
        CHECK_COMPLEMENT_SLIST( <1 _ 2>, l3,  _ 0 )
        CHECK_COMPLEMENT_SLIST( <0>,     l3,  _ 1 _ 2 )
        CHECK_COMPLEMENT_SLIST( <1>,     l3,  _ 0 _ 2 )
        CHECK_COMPLEMENT_SLIST( <2>,     l3,  _ 0 _ 1 )
        CHECK_COMPLEMENT_SLIST( <>,      l3,  _ 0 _ 1 _ 2 )
        CHECK_COMPLEMENT_SLIST( <>,      mp::nil, )
#undef CHECK_COMPLEMENT_SLIST
#undef _
#define _ ,
#define CHECK_COMPLEMENT_LIST( A, B, C ) \
static_assert(mp::check_idx<mp::ComplementList_<int_list A , B > C >::value, "a");
        using l2 = mp::iota<2>;
        CHECK_COMPLEMENT_LIST( <0>,     l2,  _ 1 )
        CHECK_COMPLEMENT_LIST( <1>,     l2,  _ 0 )
        CHECK_COMPLEMENT_LIST( <>,      l2,  _ 0 _ 1 )
        using l3 = mp::iota<3>;
        CHECK_COMPLEMENT_LIST( <0 _ 1>, l3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <0 _ 2>, l3,  _ 1 )
        CHECK_COMPLEMENT_LIST( <1 _ 2>, l3,  _ 0 )
        CHECK_COMPLEMENT_LIST( <0>,     l3,  _ 1 _ 2 )
        CHECK_COMPLEMENT_LIST( <1>,     l3,  _ 0 _ 2 )
        CHECK_COMPLEMENT_LIST( <2>,     l3,  _ 0 _ 1 )
        CHECK_COMPLEMENT_LIST( <>,      l3,  _ 0 _ 1 _ 2 )
        CHECK_COMPLEMENT_LIST( <>,      mp::nil, )
// this must also work on unserted lists.
        CHECK_COMPLEMENT_LIST( <1 _ 0>, l3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <2 _ 1>, l3,  _ 0 )
        CHECK_COMPLEMENT_LIST( <2 _ 0>, l3,  _ 1 )
        using x3 = int_list<2, 0, 1>;
        CHECK_COMPLEMENT_LIST( <1 _ 0>, x3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <2 _ 0>, x3,  _ 1 )
        CHECK_COMPLEMENT_LIST( <2 _ 1>, x3,  _ 0 )
#undef CHECK_COMPLEMENT_LIST
#undef _
    }
// MapCons.
    {
        using a = mp::iota<2>;
        using b = mp::iota<2, 1>;
        using mc = mp::MapCons<int_t<9>, tuple<a, b>>::type;
        static_assert(mp::check_idx<ref<mc, 0>, 9, 0, 1>::value, "a");
        static_assert(mp::check_idx<ref<mc, 1>, 9, 1, 2>::value, "b");
    }
// Combinations.
    {
        static_assert(mp::n_over_p(0, 0)==1, "");
        static_assert(mp::len<mp::combinations<mp::nil, 0>> == 1, "");
        using l3 = mp::iota<3>;
        using c31 = mp::combinations<l3, 1>;
        using c32 = mp::combinations<l3, 2>;
        static_assert(mp::len<c31> == 3, "a");
        static_assert(mp::check_idx<ref<c31, 0>, 0>::value, "a");
        static_assert(mp::check_idx<ref<c31, 1>, 1>::value, "b");
        static_assert(mp::check_idx<ref<c31, 2>, 2>::value, "c");
        static_assert(mp::len<c32> == 3, "b");
        static_assert(mp::check_idx<ref<c32, 0>, 0, 1>::value, "d");
        static_assert(mp::check_idx<ref<c32, 1>, 0, 2>::value, "e");
        static_assert(mp::check_idx<ref<c32, 2>, 1, 2>::value, "f");
        using l4 = mp::iota<4>;
        using c40 = mp::combinations<l4, 0>;
        using c41 = mp::combinations<l4, 1>;
        using c42 = mp::combinations<l4, 2>;
        using c43 = mp::combinations<l4, 3>;
        using c44 = mp::combinations<l4, 4>;
        static_assert(mp::len<c40> == 1, "a");
        static_assert(mp::check_idx<ref<c40, 0>>::value, "a");
        static_assert(mp::len<c41> == 4, "b");
        static_assert(mp::check_idx<ref<c41, 0>, 0>::value, "b");
        static_assert(mp::check_idx<ref<c41, 1>, 1>::value, "b");
        static_assert(mp::check_idx<ref<c41, 2>, 2>::value, "b");
        static_assert(mp::check_idx<ref<c41, 3>, 3>::value, "b");
        static_assert(mp::len<c42> == 6, "c");
        static_assert(mp::check_idx<ref<c42, 0>, 0, 1>::value, "c");
        static_assert(mp::check_idx<ref<c42, 1>, 0, 2>::value, "c");
        static_assert(mp::check_idx<ref<c42, 2>, 0, 3>::value, "c");
        static_assert(mp::check_idx<ref<c42, 3>, 1, 2>::value, "c");
        static_assert(mp::check_idx<ref<c42, 4>, 1, 3>::value, "c");
        static_assert(mp::check_idx<ref<c42, 5>, 2, 3>::value, "c");
        static_assert(mp::len<c43> == 4, "d");
        static_assert(mp::check_idx<ref<c43, 0>, 0, 1, 2>::value, "d");
        static_assert(mp::check_idx<ref<c43, 1>, 0, 1, 3>::value, "d");
        static_assert(mp::check_idx<ref<c43, 2>, 0, 2, 3>::value, "d");
        static_assert(mp::check_idx<ref<c43, 3>, 1, 2, 3>::value, "d");
        static_assert(mp::len<c44> == 1, "e");
        static_assert(mp::check_idx<ref<c44, 0>, 0, 1, 2, 3>::value, "e");
    }
// MapPrepend & ProductAppend.
    {
        using la = mp::iota<3>;
        using ca = mp::combinations<la, 1>;
        using lb = mp::iota<3>;
        using cb = mp::combinations<lb, 1>;
        using test0 = mp::MapPrepend<mp::nil, cb>::type;
        static_assert(is_same_v<test0, cb>, "");
        using test1 = mp::MapPrepend<la, cb>::type;
        static_assert(mp::len<test1> == int(mp::len<cb>), "");
        static_assert(mp::check_idx<ref<test1, 0>, 0, 1, 2, 0>::value, "");
        static_assert(mp::check_idx<ref<test1, 1>, 0, 1, 2, 1>::value, "");
        static_assert(mp::check_idx<ref<test1, 2>, 0, 1, 2, 2>::value, "");

        using test2 = mp::ProductAppend<ca, cb>::type;
        static_assert(mp::len<test2> == 9, "");
        static_assert(mp::check_idx<ref<test2, 0>, 0, 0>::value, "");
        static_assert(mp::check_idx<ref<test2, 1>, 0, 1>::value, "");
        static_assert(mp::check_idx<ref<test2, 2>, 0, 2>::value, "");
        static_assert(mp::check_idx<ref<test2, 3>, 1, 0>::value, "");
        static_assert(mp::check_idx<ref<test2, 4>, 1, 1>::value, "");
        static_assert(mp::check_idx<ref<test2, 5>, 1, 2>::value, "");
        static_assert(mp::check_idx<ref<test2, 6>, 2, 0>::value, "");
        static_assert(mp::check_idx<ref<test2, 7>, 2, 1>::value, "");
        static_assert(mp::check_idx<ref<test2, 8>, 2, 2>::value, "");
    }
// PermutationSign.
    {
#define _ ,
#define CHECK_PERM_SIGN( A, B, C ) \
static_assert(mp::PermutationSign<int_list A , int_list B >::value == C, "");
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
        using l = int_list<7, 8, 2>;
        static_assert(mp::check_idx<l, 7, 8, 2>::value, "bad");
        static_assert(mp::check_idx<mp::inc<l, 0>, 8, 8, 2>::value, "bad");
        static_assert(mp::check_idx<mp::inc<l, 1>, 7, 9, 2>::value, "bad");
        static_assert(mp::check_idx<mp::inc<l, 2>, 7, 8, 3>::value, "bad");
    }
// Prod & Sum
    {
        using l = int_list<3, 5, 7>;
        static_assert(mp::apply<mp::prod, l>::value==105, "bad");
        static_assert(mp::apply<mp::sum, l>::value==15, "bad");
    }
// tuple-dynamic
    {
        using l = int_list<3, 4, 5>;
        tr.test_eq(3, mp::on_tuple<l>::ref(0));
        tr.test_eq(4, mp::on_tuple<l>::ref(1));
        tr.test_eq(5, mp::on_tuple<l>::ref(2));
        // assert(mp::on_tuple<l>::ref(3)==-1); // TODO Check that this fails at runtime.
    }
    return tr.summary();
};
