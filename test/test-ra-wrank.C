
// (c) Daniel Llorens - 2013-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-wrank.C
/// @brief Checks for ra:: arrays, especially cell rank > 0 operations.

#include <iostream>
#include <sstream>
#include <iterator>
#include <numeric>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-wrank.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;
using std::tuple;

template <class V, class A, class B>
void framematch_demo(V && v, A && a, B && b)
{
    using FM = ra::Framematch<std::decay_t<V>, tuple<decltype(a.iter()), decltype(b.iter())>>;
    cout << "width of fm: " << mp::Len<typename FM::R>::value << ", depth: " << FM::depth << endl;
    cout << "FM::R: "; mp::print_int_list<typename FM::R>::f(cout) << endl;
    cout << "FM::framedrivers: "; mp::print_int_list<typename FM::framedrivers>::f(cout) << endl;
    cout << "FM::axisdrivers: "; mp::print_int_list<typename FM::axisdrivers>::f(cout) << endl;
    cout << "FM::axisaxes: "; mp::print_int_list<typename FM::axisaxes>::f(cout) << endl;
    cout << "FM::argindices: "; mp::print_int_list<typename FM::argindices>::f(cout) << endl;
    cout << endl;
}

template <class V, class A, class B>
void nested_wrank_demo(V && v, A && a, B && b)
{
    std::iota(a.begin(), a.end(), 10);
    std::iota(b.begin(), b.end(), 1);
    {
        using FM = ra::Framematch<V, tuple<decltype(a.iter()), decltype(b.iter())>>;
        cout << "width of fm: " << mp::Len<typename FM::R>::value << ", depth: " << FM::depth << endl;
        mp::print_int_list<typename FM::R>::f(cout) << endl;
        auto af0 = ra::applyframes<mp::Ref_<typename FM::R, 0>, FM::depth>::f(a.iter());
        auto af1 = ra::applyframes<mp::Ref_<typename FM::R, 1>, FM::depth>::f(b.iter());
        cout << sizeof(af0) << endl;
        cout << sizeof(af1) << endl;
        {
            auto ryn = ra::ryn_<FM>(FM::op(v), af0, af1);
            cout << sizeof(ryn) << endl;
            cout << "ryn rank: " << ryn.rank() << endl;
            for (int k=0; k<ryn.rank(); ++k) {
                cout << ryn.size(k) << ": " << ryn.driver(k) << ", " << endl;
            }

            // cout << mp::show_type<decltype(ra::ryn_<FM>(FM::op(v), af0, af1))>::value << endl;
            cout << "\nusing (ryn_ &):\n";
            ra::ply_ravel(ryn);
            cout << endl;
            cout << "\nusing (ryn_ &&):\n";
            ra::ply_ravel(ra::ryn_<FM>(FM::op(v), af0, af1));
        }
        {
            // cout << mp::show_type<decltype(ra::ryn(v, a.iter(), b.iter()))>::value << endl;
            auto ryn = ra::ryn(v, a.iter(), b.iter());
            cout << "ryn.shape(): " << rawp(ryn.shape()) << endl;
#define TEST(plier)                                                     \
            cout << "\n\nusing " STRINGIZE(plier) " (ryn &):\n";        \
            ra::plier(ryn);                                             \
            cout << "\n\nusing " STRINGIZE(plier) " ply (ryn &&):\n";   \
            ra::plier(ra::ryn(v, a.iter(), b.iter()));
            TEST(ply_ravel);
            TEST(ply_index);
        }
        cout << "\n\n" << endl;
    }
}

int main()
{
    TestRecorder tr;

    auto plus2real = [](real a, real b) { return a + b; };
    section("declaring verbs");
    {
        auto v = ra::verb<0, 1>::make(plus2real);
        cout << mp::Ref_<decltype(v)::R, 0>::value << endl;
        cout << mp::Ref_<decltype(v)::R, 1>::value << endl;
        auto vv = ra::wrank<1, 1>::make(v);
        cout << mp::Ref_<decltype(vv)::R, 0>::value << endl;
        cout << mp::Ref_<decltype(vv)::R, 1>::value << endl;
    }
    section("using Framematch");
    {
        ra::Unique<real, 2> a({3, 2}, ra::default_init);
        ra::Unique<real, 2> b({3, 2}, ra::default_init);
        std::iota(a.begin(), a.end(), 10);
        std::iota(b.begin(), b.end(), 1);
        {
            framematch_demo(ra::verb<0, 0>::make(plus2real), a, b);
            framematch_demo(ra::verb<0, 1>::make(plus2real), a, b);
            framematch_demo(ra::verb<1, 0>::make(plus2real), a, b);
            framematch_demo(ra::verb<1, 1>::make(plus2real), a, b);
        }
        auto plus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        {
            auto v = ra::verb<0, 2>::make(plus2real_print);
            using FM = ra::Framematch<decltype(v), tuple<decltype(a.iter()), decltype(b.iter())>>;
            cout << "width of fm: " << mp::Len<FM::R>::value << ", depth: " << FM::depth << endl;
            mp::print_int_list<FM::R>::f(cout) << endl;
            auto af0 = ra::applyframes<mp::Ref_<FM::R, 0>, FM::depth>::f(a.iter());
            auto af1 = ra::applyframes<mp::Ref_<FM::R, 1>, FM::depth>::f(b.iter());
            cout << sizeof(af0) << endl;
            cout << sizeof(af1) << endl;
            auto ryn = ra::ryn_<FM>(FM::op(v), af0, af1);
            cout << sizeof(ryn) << "\n" << endl;
            cout << "ryn rank: " << ryn.rank() << endl;
            for (int k=0; k<ryn.rank(); ++k) {
                cout << ryn.size(k) << ": " << ryn.driver(k) << ", " << endl;
            }
            ra::ply_ravel(ryn);
        }
    }
    section("wrank tests 0-1");
    {
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::verb<0, 1>::make(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::default_init),
                          ra::Unique<real, 1>({4}, ra::default_init));
        nested_wrank_demo(ra::verb<0, 1>::make(ra::verb<0, 0>::make(minus2real_print)),
                          ra::Unique<real, 1>({3}, ra::default_init),
                          ra::Unique<real, 1>({3}, ra::default_init));
    }
    section("wrank tests 1-0");
    {
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::verb<1, 0>::make(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::default_init),
                          ra::Unique<real, 1>({4}, ra::default_init));
        nested_wrank_demo(ra::verb<1, 0>::make(ra::verb<0, 0>::make(minus2real_print)),
                          ra::Unique<real, 1>({3}, ra::default_init),
                          ra::Unique<real, 1>({4}, ra::default_init));
    }
    section("wrank tests 0-0 (nop), case 1 - exact match");
    {
// This uses the applyframes specialization for 'do nothing' (@TODO if there's one).
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::verb<0, 0>::make(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::default_init),
                          ra::Unique<real, 1>({3}, ra::default_init));
    }
    section("wrank tests 0-0 (nop), case 2 - non-exact frame match");
    {
// This uses the applyframes specialization for 'do nothing' (@TODO if there's one).
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::verb<0, 0>::make(minus2real_print),
                          ra::Unique<real, 2>({3, 4}, ra::default_init),
                          ra::Unique<real, 1>({3}, ra::default_init));
        nested_wrank_demo(ra::verb<0, 0>::make(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::default_init),
                          ra::Unique<real, 2>({3, 4}, ra::default_init));
    }
    section("wrank tests 1-1-0, init array with outer product");
    {
        auto minus2real = [](real & c, real a, real b) { c = a-b; };
        ra::Unique<real, 1> a({3}, ra::default_init);
        ra::Unique<real, 1> b({4}, ra::default_init);
        std::iota(a.begin(), a.end(), 10);
        std::iota(b.begin(), b.end(), 1);
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
        ra::Unique<real, 2> c({3, 4}, ra::default_init);
        ra::ply_either(ra::ryn(ra::verb<1, 0, 1>::make(minus2real), c.iter(), a.iter(), b.iter()));
        cout << "c: " << c << endl;
        real checkc34[3*4] = { /* 10-[1 2 3 4] */ 9, 8, 7, 6,
                               /* 11-[1 2 3 4] */ 10, 9, 8, 7,
                               /* 12-[1 2 3 4] */ 11, 10, 9, 8 };
        tr.test(std::equal(checkc34, checkc34+3*4, c.begin()));
        ra::Unique<real, 2> d34(ra::ryn(ra::verb<0, 1>::make(std::minus<real>()), a.iter(), b.iter()));
        cout << "d34: " << d34 << endl;
        tr.test(std::equal(checkc34, checkc34+3*4, d34.begin()));
        real checkc43[3*4] = { /* [10 11 12]-1 */ 9, 10, 11,
                               /* [10 11 12]-2 */ 8, 9, 10,
                               /* [10 11 12]-3 */ 7, 8, 9,
                               /* [10 11 12]-4 */ 6, 7, 8 };
        ra::Unique<real, 2> d43(ra::ryn(ra::verb<1, 0>::make(std::minus<real>()), a.iter(), b.iter()));
        cout << "d43: " << d43 << endl;
        tr.test(d43.size(0)==4 && d43.size(1)==3);
        tr.test(std::equal(checkc43, checkc43+3*4, d43.begin()));
    }
    section("recipe for unbeatable subscripts in _from_ operator");
    {
        ra::Unique<int, 1> a({3}, ra::default_init);
        ra::Unique<int, 1> b({4}, ra::default_init);
        std::iota(a.begin(), a.end(), 10);
        std::iota(b.begin(), b.end(), 1);
        ra::Unique<real, 2> c({100, 100}, ra::default_init);
        std::iota(c.begin(), c.end(), 0);

        real checkd[3*4] = { 1001, 1002, 1003, 1004,  1101, 1102, 1103, 1104,  1201, 1202, 1203, 1204 };
// default auto is value, so need to speficy.
#define EXPR ra::ryn(ra::verb<0, 1>::make([&c](int a, int b) -> decltype(auto) { return c(a, b); } ), \
                     a.iter(), b.iter())
        std::ostringstream os;
        os << EXPR << endl;
        ra::Unique<real, 2> cc(ra::init_not);
        std::istringstream is(os.str());
        is >> cc;
        cout << "cc: " << cc << endl;
        tr.test(std::equal(checkd, checkd+3*4, cc.begin()));
        ra::Unique<real, 2> d(EXPR);
        cout << "d: " << d << endl;
        tr.test(std::equal(checkd, checkd+3*4, d.begin()));
// Using expr as lvalue.
        EXPR = 7.;
        cout << EXPR << endl;
// expr-way @BUG use of test_equal fails (??)
        assert(every(c==where(ra::_0>=10 && ra::_0<=12 && ra::_1>=1 && ra::_1<=4, 7, ra::_0*100+ra::_1)));
// looping...
        bool valid = true;
        for (int i=0; i<c.size(0); ++i) {
            for (int j=0; j<c.size(1); ++j) {
                valid = valid && (i>=10 && i<=12 && j>=1 && j<=4 ? 7 : i*100+j == c(i, j));
            }
        }
        tr.test(valid);
    }
    section("rank conjunction / empty");
    {
    }
    section("static rank() in ra::Ryn");
    {
        ra::Unique<real, 3> a({2, 2, 2}, 1.);
        ra::Unique<real, 3> b({2, 2, 2}, 2.);
        real y = 0;
        auto e = ra::ryn(ra::verb<0, 0>::make([&y](real const a, real const b) { y += a*b; }), a.iter(), b.iter());
        static_assert(3==e.rank(), "bad rank in static rank expr");
        ra::ply_ravel(ra::ryn(ra::verb<0, 0>::make([&y](real const a, real const b) { y += a*b; }), a.iter(), b.iter()));
        tr.test_equal(16, y);
    }
    section("outer product variants");
    {
        ra::Owned<real, 2> a({2, 3}, ra::_0 - ra::_1);
        ra::Owned<real, 2> b({3, 2}, ra::_1 - 2*ra::_0);
        ra::Owned<real, 2> c1 = mm_mul(a, b);
        cout << "matrix a * b: \n" << c1 << endl;
// matrix product as outer product + reduction (no reductions yet, so manually).
        {
            ra::Owned<real, 3> d = ra::ryn(ra::wrank<1, 2>::make(ra::wrank<0, 1>::make(ra::times())), start(a), start(b));
            cout << "d(i,k,j) = a(i,k)*b(k,j): \n" << d << endl;
            ra::Owned<real, 2> c2({d.size(0), d.size(2)}, 0.);
            for (int k=0; k<d.size(1); ++k) {
                c2 += d(ra::all, k, ra::all);
            }
            tr.test_equal(c1, c2);
        }
// do the k-reduction by plying with wrank.
        {
            ra::Owned<real, 2> c2({a.size(0), b.size(1)}, 0.);
            ra::ply_either(ra::ryn(ra::wrank<1, 1, 2>::make(ra::wrank<1, 0, 1>::make([](auto & c, auto && a, auto && b) { c += a*b; })),
                                   start(c2), start(a), start(b)));
            cout << "sum_k a(i,k)*b(k,j): \n" << c2 << endl;
            tr.test_equal(c1, c2);
        }
    }
    return tr.summary();
}
