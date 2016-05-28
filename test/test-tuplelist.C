
// (c) Daniel Llorens - 2010

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-tuplelist.C
/// @brief test type list library based on tuples.

#include <iostream>
#include "ra/tuple-dynamic.H"
#include "ra/tuple-list.H"
#include "ra/test.H"

using std::tuple;
using std::tuple_size;
using std::tuple_element;
using std::is_same;
using namespace mp;
using std::cout; using std::endl;

template <class A>
struct Inc1
{
    typedef int_t<A::value+1> type;
};

template <class A, class B>
struct Sum2
{
    typedef int_t<A::value+B::value> type;
};

template <class ... A> struct SumV;
template <class A0, class ... A>
struct SumV<A0, A ...>
{
    static int const value = A0::value + SumV<A ...>::value;
};
template <>
struct SumV<>
{
    static int const value = 0;
};

template <class A>
struct SamePP
{
    template <class B>
    struct type
    {
        static bool const value = std::is_same<A, B>::value;
    };
};

struct True
{
    static bool const value = true;
};

int main()
{
    TestRecorder tr(std::cout);
// Basic constexpr stulff.
    {
        static_assert(mp::fact(0)==1, "bad fact(0)");
        static_assert(mp::fact(1)==1, "bad fact(1)");
        static_assert(mp::fact(2)==2, "bad fact(2)");
        static_assert(mp::fact(3)==6, "bad fact(3)");
    }
// Booleans.
    {
        static_assert(True::value && !mp::Not<True>::value, "bad Not");
    }
// Apply, Typer, Valuer.
    {
        using B = int_list<5, 6, 7, 1>;
        static_assert(Apply<Typer<SumV>::type_, B>::type::value==19, "bad");
        static_assert(ApplyV<SumV, B>::value==19, "bad");
        static_assert(ApplyV<Valuer<Sum>::type, B>::value==19, "bad");
    }
// Map & Coerce.
    {
        using A = int_list<5, 6, 3>;
        using B = int_list<2, 3, -1>;
        static_assert(check_idx<Map<Sum, A>::type, 5, 6, 3>::value, "");
        static_assert(check_idx<Map<Sum, A, B>::type, 7, 9, 2>::value, "");
        static_assert(check_idx<Map<Coerce1<Inc1>::type_, A>::type,
                                6, 7, 4>::value, "");
        static_assert(check_idx<Map<Coerce2<Sum2>::type_, A, B>::type,
                                7, 9, 2>::value, "");
    }
// Fold.
    {
        using A = int_list<5, 6, 3>;
        using B = int_list<2, 3, -1>;
        static_assert(Fold<Sum, int_t<1>, A>::type::value==15, "");
        static_assert(Fold<Sum, int_t<3>, B>::type::value==7, "");
        static_assert(Fold<Max, int_t<4>, A>::type::value==6, "");
        static_assert(Fold<Max, void, A>::type::value==6, "");
        static_assert(Fold<Max, int_t<9>, A>::type::value==9, "");
        static_assert(Fold<Min, int_t<4>, A>::type::value==3, "");
        static_assert(Fold<Min, void, A>::type::value==3, "");
        static_assert(Fold<Min, int_t<1>, A>::type::value==1, "");
        static_assert(Fold<Coerce2<Sum2>::type_, int_t<3>, B>::type::value==7, "");
    }
// product_test, product_testn
//     {
//         using A = int_list<1, 3, 3, 2>;
//         using B = int_list<5, 6, 7, 1>;
//         cout << prod_testn<Sum, A, B>::value << endl;
//         static_assert(prod_testn<Sum, A, B>::value==1620, "");
//     }
// Reductions.
    {
        using list_ = int_list<>;
        using list_1 = int_list<1>;
        using list_0 = int_list<0>;
        using list_10 = int_list<1, 0>;
        using list_01 = int_list<0, 1>;
        using list_11 = int_list<1, 1>;
        using list_00 = int_list<0, 0>;
        static_assert(Apply<And, list_>::type::value, "bad And");
        static_assert(Apply<And, list_1>::type::value, "bad And");
        static_assert(!Apply<And, list_0>::type::value, "bad And");
        static_assert(!Apply<And, list_10>::type::value, "bad And");
        static_assert(!Apply<And, list_01>::type::value, "bad And");
        static_assert(Apply<And, list_11>::type::value, "bad And");
        static_assert(!Apply<And, list_00>::type::value, "bad And");
        static_assert(!Apply<Or, list_>::type::value, "bad Or");
        static_assert(Apply<Or, list_1>::type::value, "bad Or");
        static_assert(!Apply<Or, list_0>::type::value, "bad Or");
        static_assert(Apply<Or, list_10>::type::value, "bad Or");
        static_assert(Apply<Or, list_01>::type::value, "bad Or");
        static_assert(Apply<Or, list_11>::type::value, "bad Or");
        static_assert(!Apply<Or, list_00>::type::value, "bad Or");
        static_assert(Apply<Sum, list_>::type::value==0, "bad Sum");
        static_assert(Apply<Sum, int_list<2, 4>>::type::value==6, "bad Sum");
        static_assert(Apply<Prod, list_>::type::value==1, "bad Prod");
        static_assert(Apply<Prod, int_list<2, 3>>::type::value==6, "bad Prod");
    }
// Append.
    using A = int_list<0, 2, 3>;
    using B = int_list<5, 6, 7>;
    using C = int_list<9, 8>;
    static_assert(is_same<int_list<>, nil>::value, "");
    static_assert(is_same<C, tuple<int_t<9>, int_t<8>>>::value, "");
    using O = nil;
    using A_B = Append<A, B>::type;
    using A_C = Append<A, C>::type;
    using C_B = Append<C, B>::type;
    static_assert(check_idx<A_B, 0, 2, 3, 5, 6, 7>::value, "bad AB");
    static_assert(check_idx<A_C, 0, 2, 3, 9, 8>::value, "bad AB");
    static_assert(check_idx<C_B, 9, 8, 5, 6, 7>::value, "bad AB");
    static_assert(is_same<A, Append<A, O>::type>::value, "bad A+empty");
    static_assert(is_same<A, Append<O, A>::type>::value, "bad empty+A");
// Iota.
    static_assert(check_idx<Iota<4, 0>::type, 0, 1, 2, 3>::value, "0a");
    static_assert(check_idx<Iota<4, 3>::type, 3, 4, 5, 6>::value, "0b");
    static_assert(check_idx<Iota<0, 3>::type>::value, "0c");
    static_assert(check_idx<Iota<1, 3>::type, 3>::value, "0d");
    static_assert(check_idx<Iota<3, -2>::type, -2, -1, 0>::value, "0e");
    static_assert(check_idx<Iota<4, 3, -1>::type, 3, 2, 1, 0>::value, "0a");
// MakeList.
    static_assert(check_idx<MakeList<2, int_t<9>>::type, 9, 9>::value, "1a");
    static_assert(check_idx<MakeList<0, int_t<9>>::type>::value, "1b");
// Ref.
    static_assert(Ref<tuple<A, B, C>, 0, 0>::type::value==0, "3a");
    static_assert(Ref<tuple<A, B, C>, 0, 1>::type::value==2, "3b");
    static_assert(Ref<tuple<A, B, C>, 0, 2>::type::value==3, "3c");
    static_assert(Ref<tuple<A, B, C>, 1, 0>::type::value==5, "3d");
    static_assert(Ref<tuple<A, B, C>, 1, 1>::type::value==6, "3e");
    static_assert(Ref<tuple<A, B, C>, 1, 2>::type::value==7, "3f");
    static_assert(Ref<tuple<A, B, C>, 2, 0>::type::value==9, "3g");
    static_assert(Ref<tuple<A, B, C>, 2, 1>::type::value==8, "3h");
    static_assert(First<B>::type::value==5 && First<C>::type::value==9, "3i");
    static_assert(Last<B>::type::value==7 && Last<C>::type::value==8, "3j");
// Useful default.
    static_assert(tuple_size<Ref<nil>::type>::value==0, "3i");
// 3-indices.
    using S2AB = tuple<A, B>;
    using S2BC = tuple<B, C>;
    using S3 = tuple<S2AB, S2BC>;
// in S2AB.
    static_assert(Ref<S3, 0, 0, 0>::type::value==0, "3j");
    static_assert(Ref<S3, 0, 0, 1>::type::value==2, "3k");
    static_assert(Ref<S3, 0, 0, 2>::type::value==3, "3l");
    static_assert(Ref<S3, 0, 1, 0>::type::value==5, "3m");
    static_assert(Ref<S3, 0, 1, 1>::type::value==6, "3n");
    static_assert(Ref<S3, 0, 1, 2>::type::value==7, "3o");
// in S2BC.
    static_assert(Ref<S3, 1, 0, 0>::type::value==5, "3p");
    static_assert(Ref<S3, 1, 0, 1>::type::value==6, "3q");
    static_assert(Ref<S3, 1, 0, 2>::type::value==7, "3r");
    static_assert(Ref<S3, 1, 1, 0>::type::value==9, "3s");
    static_assert(Ref<S3, 1, 1, 1>::type::value==8, "3t");
// Index.
    static_assert(Index<A, int_t<0>>::value==0, "4a");
    static_assert(Index<A, int_t<2>>::value==1, "4b");
    static_assert(Index<A, int_t<3>>::value==2, "4c");
    static_assert(Index<A, int_t<4>>::value==-1, "4d");
    static_assert(Index<S3, S2BC>::value==1, "4e");
// IndexIf.
    static_assert(IndexIf<A, SamePP<int_t<0>>::type>::value==0, "5a");
    static_assert(IndexIf<A, SamePP<int_t<2>>::type>::value==1, "5b");
    static_assert(IndexIf<A, SamePP<int_t<3>>::type>::value==2, "5c");
    static_assert(IndexIf<A, SamePP<int_t<9>>::type>::value==-1, "5d");
// Find.
    static_assert(is_same<FindTail<A, int_t<0>>::type, A>::value, "4a");
    static_assert(check_idx<FindTail<A, int_t<2>>::type, 2, 3>::value, "4b");
    static_assert(check_idx<FindTail<A, int_t<3>>::type, 3>::value, "4c");
    static_assert(NilP<FindTail<A, int_t<4>>::type>::value, "4d");
    static_assert(is_same<FindTail<S3, S2BC>::type, tuple<S2BC>>::value, "4e");
// Reverse.
    static_assert(check_idx<Reverse<A_B>::type, 7, 6, 5, 3, 2, 0>::value, "5a");
    static_assert(check_idx<Reverse<O>::type>::value, "5b");
    static_assert(is_same<Reverse<mp::nil>::type, mp::nil>::value, "bad Reverse");
// Drop & Take.
    static_assert(check_idx<Drop<A, 0>::type, 0, 2, 3>::value, "bad 6a");
    static_assert(check_idx<Drop<A, 1>::type, 2, 3>::value, "bad 6b");
    static_assert(check_idx<Drop<A, 2>::type, 3>::value, "bad 6c");
    static_assert(check_idx<Drop<A, 3>::type>::value, "bad 6d");
    static_assert(check_idx<Take<A, 0>::type>::value, "bad 6e");
    static_assert(check_idx<Take<A, 1>::type, 0>::value, "bad 6f");
    static_assert(check_idx<Take<A, 2>::type, 0, 2>::value, "bad 6g");
    static_assert(check_idx<Take<A, 3>::type, 0, 2, 3>::value, "bad 6h");
// Complement.
    {
        using case1 = int_list<1>;
        static_assert(check_idx<Complement<case1, 0>::type>::value, "");
        static_assert(check_idx<Complement<case1, 1>::type, 0>::value, "");
        static_assert(check_idx<Complement<case1, 2>::type, 0>::value, "");
        static_assert(check_idx<Complement<case1, 3>::type, 0, 2>::value, "");
        using list3 = Iota<3>::type;
        static_assert(check_idx<Complement<list3, 3>::type>::value, "");
        using c36 = Complement<list3, 6>::type;
        static_assert(check_idx<c36, 3, 4, 5>::value, "");
        static_assert(check_idx<Complement<c36, 6>::type, 0, 1, 2>::value, "");
        using case0 = tuple<int_t<0>>;
        static_assert(check_idx<Complement<case0, 0>::type>::value, "");
        static_assert(check_idx<Complement<case0, 1>::type>::value, "");
        static_assert(check_idx<Complement<case0, 2>::type, 1>::value, "");
    }
// ComplementSList && ComplementList.
    {
#define _ ,
#define CHECK_COMPLEMENT_SLIST( A, B, C ) \
static_assert(check_idx<ComplementSList<int_list A , B >::type \
                        C >::value, "a");
        CHECK_COMPLEMENT_SLIST( <1>, int_list<>, )
        CHECK_COMPLEMENT_SLIST( <1>, int_list<0>, _ 0)
        CHECK_COMPLEMENT_SLIST( <1>, int_list<0 _ 1>, _ 0)
        CHECK_COMPLEMENT_SLIST( <1>, int_list<0 _ 1 _ 2>, _ 0 _ 2)
        using l2 = Iota<2>::type;
        CHECK_COMPLEMENT_SLIST( <0>,     l2,  _ 1 )
        CHECK_COMPLEMENT_SLIST( <1>,     l2,  _ 0 )
        CHECK_COMPLEMENT_SLIST( <>,      l2,  _ 0 _ 1 )
        using l3 = Iota<3>::type;
        CHECK_COMPLEMENT_SLIST( <0 _ 1>, l3,  _ 2 )
        CHECK_COMPLEMENT_SLIST( <0 _ 2>, l3,  _ 1 )
        CHECK_COMPLEMENT_SLIST( <1 _ 2>, l3,  _ 0 )
        CHECK_COMPLEMENT_SLIST( <0>,     l3,  _ 1 _ 2 )
        CHECK_COMPLEMENT_SLIST( <1>,     l3,  _ 0 _ 2 )
        CHECK_COMPLEMENT_SLIST( <2>,     l3,  _ 0 _ 1 )
        CHECK_COMPLEMENT_SLIST( <>,      l3,  _ 0 _ 1 _ 2 )
        CHECK_COMPLEMENT_SLIST( <>,      nil, )
#undef CHECK_COMPLEMENT_SLIST
#undef _
#define _ ,
#define CHECK_COMPLEMENT_LIST( A, B, C ) \
static_assert(check_idx<ComplementList<int_list A , B >::type \
                        C >::value, "a");
        using l2 = Iota<2>::type;
        CHECK_COMPLEMENT_LIST( <0>,     l2,  _ 1 )
        CHECK_COMPLEMENT_LIST( <1>,     l2,  _ 0 )
        CHECK_COMPLEMENT_LIST( <>,      l2,  _ 0 _ 1 )
        using l3 = Iota<3>::type;
        CHECK_COMPLEMENT_LIST( <0 _ 1>, l3,  _ 2 )
        CHECK_COMPLEMENT_LIST( <0 _ 2>, l3,  _ 1 )
        CHECK_COMPLEMENT_LIST( <1 _ 2>, l3,  _ 0 )
        CHECK_COMPLEMENT_LIST( <0>,     l3,  _ 1 _ 2 )
        CHECK_COMPLEMENT_LIST( <1>,     l3,  _ 0 _ 2 )
        CHECK_COMPLEMENT_LIST( <2>,     l3,  _ 0 _ 1 )
        CHECK_COMPLEMENT_LIST( <>,      l3,  _ 0 _ 1 _ 2 )
        CHECK_COMPLEMENT_LIST( <>,      nil, )
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
        using a = Iota<2>::type;
        using b = Iota<2, 1>::type;
        using mc = MapCons<int_t<9>, tuple<a, b>>::type;
        static_assert(check_idx<Ref<mc, 0>::type, 9, 0, 1>::value, "a");
        static_assert(check_idx<Ref<mc, 1>::type, 9, 1, 2>::value, "b");
    }
// Combinations.
    {
        static_assert(n_over_p<0, 0>::value==1, "");
        static_assert(Len<Combinations<nil, 0>::type>::value==1, "");
        using l3 = Iota<3>::type;
        using c31 = Combinations<l3, 1>::type;
        using c32 = Combinations<l3, 2>::type;
        static_assert(Len<c31>::value==3, "a");
        static_assert(check_idx<Ref<c31, 0>::type, 0>::value, "a");
        static_assert(check_idx<Ref<c31, 1>::type, 1>::value, "b");
        static_assert(check_idx<Ref<c31, 2>::type, 2>::value, "c");
        static_assert(Len<c32>::value==3, "b");
        static_assert(check_idx<Ref<c32, 0>::type, 0, 1>::value, "d");
        static_assert(check_idx<Ref<c32, 1>::type, 0, 2>::value, "e");
        static_assert(check_idx<Ref<c32, 2>::type, 1, 2>::value, "f");
        using l4 = Iota<4>::type;
        using c40 = Combinations<l4, 0>::type;
        using c41 = Combinations<l4, 1>::type;
        using c42 = Combinations<l4, 2>::type;
        using c43 = Combinations<l4, 3>::type;
        using c44 = Combinations<l4, 4>::type;
        static_assert(Len<c40>::value==1, "a");
        static_assert(check_idx<Ref<c40, 0>::type>::value, "a");
        static_assert(Len<c41>::value==4, "b");
        static_assert(check_idx<Ref<c41, 0>::type, 0>::value, "b");
        static_assert(check_idx<Ref<c41, 1>::type, 1>::value, "b");
        static_assert(check_idx<Ref<c41, 2>::type, 2>::value, "b");
        static_assert(check_idx<Ref<c41, 3>::type, 3>::value, "b");
        static_assert(Len<c42>::value==6, "c");
        static_assert(check_idx<Ref<c42, 0>::type, 0, 1>::value, "c");
        static_assert(check_idx<Ref<c42, 1>::type, 0, 2>::value, "c");
        static_assert(check_idx<Ref<c42, 2>::type, 0, 3>::value, "c");
        static_assert(check_idx<Ref<c42, 3>::type, 1, 2>::value, "c");
        static_assert(check_idx<Ref<c42, 4>::type, 1, 3>::value, "c");
        static_assert(check_idx<Ref<c42, 5>::type, 2, 3>::value, "c");
        static_assert(Len<c43>::value==4, "d");
        static_assert(check_idx<Ref<c43, 0>::type, 0, 1, 2>::value, "d");
        static_assert(check_idx<Ref<c43, 1>::type, 0, 1, 3>::value, "d");
        static_assert(check_idx<Ref<c43, 2>::type, 0, 2, 3>::value, "d");
        static_assert(check_idx<Ref<c43, 3>::type, 1, 2, 3>::value, "d");
        static_assert(Len<c44>::value==1, "e");
        static_assert(check_idx<Ref<c44, 0>::type, 0, 1, 2, 3>::value, "e");
    }
// MapPrepend & ProductAppend.
    {
        typedef Iota<3>::type la;
        typedef Combinations<la, 1>::type ca;
        typedef Iota<3>::type lb;
        typedef Combinations<lb, 1>::type cb;
        typedef MapPrepend<nil, cb>::type test0;
        static_assert(std::is_same<test0, cb>::value, "");
        typedef MapPrepend<la, cb>::type test1;
        static_assert(Len<test1>::value==int(Len<cb>::value), "");
        static_assert(check_idx<Ref<test1, 0>::type, 0, 1, 2, 0>::value, "");
        static_assert(check_idx<Ref<test1, 1>::type, 0, 1, 2, 1>::value, "");
        static_assert(check_idx<Ref<test1, 2>::type, 0, 1, 2, 2>::value, "");

        typedef ProductAppend<ca, cb>::type test2;
        static_assert(Len<test2>::value==9, "");
        static_assert(check_idx<Ref<test2, 0>::type, 0, 0>::value, "");
        static_assert(check_idx<Ref<test2, 1>::type, 0, 1>::value, "");
        static_assert(check_idx<Ref<test2, 2>::type, 0, 2>::value, "");
        static_assert(check_idx<Ref<test2, 3>::type, 1, 0>::value, "");
        static_assert(check_idx<Ref<test2, 4>::type, 1, 1>::value, "");
        static_assert(check_idx<Ref<test2, 5>::type, 1, 2>::value, "");
        static_assert(check_idx<Ref<test2, 6>::type, 2, 0>::value, "");
        static_assert(check_idx<Ref<test2, 7>::type, 2, 1>::value, "");
        static_assert(check_idx<Ref<test2, 8>::type, 2, 2>::value, "");
    }
// PermutationSign.
    {
#define _ ,
#define CHECK_PERM_SIGN( A, B, C ) \
static_assert(PermutationSign<int_list A , \
                              int_list B >::value == C, "");
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
// Inc.
    {
        using l = int_list<7, 8, 2>;
        static_assert(check_idx<l, 7, 8, 2>::value, "bad");
        static_assert(check_idx<Inc<l, 0>::type, 8, 8, 2>::value, "bad");
        static_assert(check_idx<Inc<l, 1>::type, 7, 9, 2>::value, "bad");
        static_assert(check_idx<Inc<l, 2>::type, 7, 8, 3>::value, "bad");
    }
// Prod & Sum
    {
        using l = int_list<3, 5, 7>;
        static_assert(Apply<Prod, l>::type::value==105, "bad");
        static_assert(Apply<Sum, l>::type::value==15, "bad");
    }
// tuple-dynamic.
    {
        using l = int_list<3, 4, 5>;
        tr.test_eq(3, mp::on_tuple<l>::ref(0));
        tr.test_eq(4, mp::on_tuple<l>::ref(1));
        tr.test_eq(5, mp::on_tuple<l>::ref(2));
        // assert(mp::on_tuple<l>::ref(3)==-1); // @TODO Check that this fails at runtime.
    }
    return tr.summary();
};
