
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-9.C
/// @brief Against a bug in clang-3.7/8/9

#include <iostream>
#include <iterator>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"
#include "ra/wedge.H"

using std::cout; using std::endl; using std::flush; using std::tuple;
template <int rank=ra::RANK_ANY> using Ureal = ra::Unique<real, rank>;
template <int rank=ra::RANK_ANY> using Uint = ra::Unique<int, rank>;

template <class L>
void print_intlist()
{
    cout << mp::First_<L>::value << " " << flush;
}

template <>
void print_intlist<mp::nil>()
{
    cout << endl;
}

int main()
{
    TestRecorder tr(std::cout);
    section("@TODO clang issue that causes errors all the way down");
    {
        using thematch1 = mp::MatchPermutationP< mp::int_list<1, 0> >::type< mp::int_list<0, 1> >;
        cout << "A1... " << (thematch1::value) << endl;
        using thematch2 = mp::MatchPermutationP< mp::int_list<0, 1> >::type< mp::int_list<1, 0> >;
        cout << "A2... " << (thematch2::value) << endl;
        using index_if = mp::IndexIf<std::tuple< mp::int_list<1, 0> >, mp::MatchPermutationP< mp::int_list<0, 1> >::template type>;
        cout << "B... " << index_if::value << endl;
        // static_assert(index_if::value==0, "this fails in clang 3.8");
    }
//     {
//         constexpr int D = 3;
//         constexpr int O = 1;
//         using H = fun::Hodge<D, O>;

//         // using Va = ra::Small<double, 3>;
//         // using Vb = ra::Small<double, 3>;
//         constexpr int i = 1;

//         using Cai = mp::Ref_<H::Ca, i>;
//         static_assert(mp::Len<Cai>::value==O, "bad");
// // sort Cai, because Complement only accepts sorted combinations.
// // Ref<Cb, i> should be complementary to Cai, but I don't want to rely on that.
//         using SCai = mp::Ref_<H::LexOrCa, mp::FindCombination<Cai, H::LexOrCa>::where>;
//         using CompCai = typename mp::Complement<SCai, D>::type;
//         static_assert(mp::Len<CompCai>::value==D-O, "bad");
//         using fpw = mp::FindCombination<CompCai, H::Cb>;

//         cout << "CompCai: " << endl; print_intlist<CompCai>(); cout << endl;
//         cout << "Cb length: " << mp::Len<H::Cb>::value << endl;
//         cout << "Cb0: " << endl; print_intlist<mp::Ref_<H::Cb, 0> >(); cout << endl;
//         cout << "Cb1: " << endl; print_intlist<mp::Ref_<H::Cb, 1> >(); cout << endl;
//         cout << "Cb2: " << endl; print_intlist<mp::Ref_<H::Cb, 2> >(); cout << endl;
//         cout << "where: " << fpw::where << endl;

// // // for the sign see e.g. DoCarmo1991 I.Ex 10.
// //         using fps = mp::FindCombination<mp::Append_<Cai, mp::Ref_<H::Cb, fpw::where>>, H::Cr>;
// //         static_assert(fps::sign!=0, "bad");
// //         cout << fps::sign << endl;
//     }
    // section("hodge / hodgex");
    // {
    //     ra::Small<real, 3> a {1, 2, 3};
    //     ra::Small<real, 3> c;
    //     fun::hodgex<3, 1>(a, c);
    //     tr.test_eq(a, c);
    //     auto d = fun::hodge<3, 1>(a);
    //     tr.test_eq(a, d);
    // }
    return tr.summary();
}
