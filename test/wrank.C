// -*- mode: c++; coding: utf-8 -*-
/// @file wrank.C
/// @brief Checks for ra:: arrays, especially cell rank > 0 operations.

// (c) Daniel Llorens - 2013-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <sstream>
#include <iterator>
#include <numeric>
#include <atomic>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/wrank.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "test/old.H"

using std::cout, std::endl, std::flush, std::tuple, ra::dim_t;
using real = double;

// Find the driver for given axis. This pattern is used in Ryn to find the size-giving argument for each axis.
template <int iarg, class T>
std::enable_if_t<(iarg==mp::len<std::decay_t<T>>), int>
constexpr driver(T && t, int k)
{
    assert(0 && "there was no driver"); abort();
}

template <int iarg, class T>
std::enable_if_t<(iarg<mp::len<std::decay_t<T>>), int>
constexpr driver(T && t, int k)
{
    dim_t s = std::get<iarg>(t).size(k);
    return s>=0 ? iarg : driver<iarg+1>(t, k);
}

template <class FM, class Enable=void> struct DebugFrameMatch
{
    constexpr static bool terminal = true;
    using R = typename FM::R;
    constexpr static int depth = FM::depth;
    using framedrivers = mp::int_list<FM::driver>;
    using axisdrivers = mp::makelist<mp::ref<typename FM::live, FM::driver>::value, mp::int_t<FM::driver>>;
    using axisaxes = mp::iota<mp::ref<typename FM::live, FM::driver>::value, mp::len<mp::ref<typename FM::R_, FM::driver>>>;
    using argindices = mp::zip<axisdrivers, axisaxes>;
};

template <class FM> struct DebugFrameMatch<FM, std::enable_if_t<mp::exists<typename FM::FM> > >
{
    using FMC = typename FM::FM;
    using DFMC = DebugFrameMatch<FMC>;

    constexpr static bool terminal = false;
    using R = typename FM::R;
    constexpr static int depth = FM::depth;
    using framedrivers = mp::cons<mp::int_t<FM::driver>, typename DFMC::framedrivers>;
    using axisdrivers = mp::append<mp::makelist<mp::ref<typename FM::live, FM::driver>::value, mp::int_t<FM::driver>>,
                                   typename DFMC::axisdrivers>;
    using axisaxes = mp::append<mp::iota<mp::ref<typename FM::live, FM::driver>::value, mp::len<mp::ref<typename FM::R_, FM::driver>>>,
                                 typename DFMC::axisaxes>;
    using argindices = mp::zip<axisdrivers, axisaxes>;
};

template <class V, class A, class B>
void framematch_demo(V && v, A && a, B && b)
{
    using FM = ra::Framematch<std::decay_t<V>, tuple<decltype(a.iter()), decltype(b.iter())>>;
    using DFM = DebugFrameMatch<FM>;
    cout << "FM is terminal: " << DFM::terminal << endl;
    cout << "width of fm: " << mp::len<typename DFM::R> << ", depth: " << DFM::depth << endl;
    cout << "FM::R: " << mp::print_int_list<typename DFM::R> {} << endl;
    cout << "FM::framedrivers: " << mp::print_int_list<typename DFM::framedrivers> {} << endl;
    cout << "FM::axisdrivers: " << mp::print_int_list<typename DFM::axisdrivers> {} << endl;
    cout << "FM::axisaxes: " << mp::print_int_list<typename DFM::axisaxes> {} << endl;
    cout << "FM::argindices: " << mp::print_int_list<typename DFM::argindices> {} << endl;
    cout << endl;
}

template <class V, class A, class B>
void nested_wrank_demo(V && v, A && a, B && b)
{
    std::iota(a.begin(), a.end(), 10);
    std::iota(b.begin(), b.end(), 1);
    {
        using FM = ra::Framematch<V, tuple<decltype(a.iter()), decltype(b.iter())>>;
        cout << "width of fm: " << mp::len<typename FM::R> << ", depth: " << FM::depth << endl;
        cout << mp::print_int_list<typename FM::R> {} << endl;
        auto af0 = ra::applyframes<mp::ref<typename FM::R, 0>, FM::depth>(a.iter());
        auto af1 = ra::applyframes<mp::ref<typename FM::R, 1>, FM::depth>(b.iter());
        cout << sizeof(af0) << endl;
        cout << sizeof(af1) << endl;
        {
            auto ryn = ra::ryn<FM>(FM::op(v), af0, af1);
            cout << sizeof(ryn) << endl;
            cout << "ryn rank: " << ryn.rank() << endl;
            for (int k=0; k<ryn.rank(); ++k) {
                cout << ryn.size(k) << ": " << driver<0>(ryn.t, k) << endl;
            }

            // cout << mp::show_type<decltype(ra::ryn<FM>(FM::op(v), af0, af1))>::value << endl;
            cout << "\nusing (ryn &):\n";
            ra::ply_ravel(ryn);
            cout << endl;
            cout << "\nusing (ryn &&):\n";
            ra::ply_ravel(ra::ryn<FM>(FM::op(v), af0, af1));
        }
        {
            // cout << mp::show_type<decltype(ra::expr(v, a.iter(), b.iter()))>::value << endl;
            auto ryn = ra::expr(v, a.iter(), b.iter());
            cout << "ryn.shape(): " << ra::noshape << ryn.shape() << endl;
#define TEST(plier)                                                     \
            cout << "\n\nusing " STRINGIZE(plier) " (ryn &):\n";        \
            ra::plier(ryn);                                             \
            cout << "\n\nusing " STRINGIZE(plier) " ply (ryn &&):\n";   \
            ra::plier(ra::expr(v, a.iter(), b.iter()));
            TEST(ply_ravel);
            TEST(ply_index);
            TEST(plyf);
            TEST(plyf_index);
        }
        cout << "\n\n" << endl;
    }
}

int main()
{
    TestRecorder tr;

    auto plus2real = [](real a, real b) { return a + b; };
    tr.section("declaring verbs");
    {
        auto v = ra::wrank<0, 1>(plus2real);
        cout << mp::ref<decltype(v)::R, 0>::value << endl;
        cout << mp::ref<decltype(v)::R, 1>::value << endl;
        auto vv = ra::wrank<1, 1>(v);
        cout << mp::ref<decltype(vv)::R, 0>::value << endl;
        cout << mp::ref<decltype(vv)::R, 1>::value << endl;
    }
    tr.section("using Framematch");
    {
        ra::Unique<real, 2> a({3, 2}, ra::none);
        ra::Unique<real, 2> b({3, 2}, ra::none);
        std::iota(a.begin(), a.end(), 10);
        std::iota(b.begin(), b.end(), 1);
        {
            framematch_demo(plus2real, a, b);
            framematch_demo(ra::wrank<0, 0>(plus2real), a, b);
            framematch_demo(ra::wrank<0, 1>(plus2real), a, b);
            framematch_demo(ra::wrank<1, 0>(plus2real), a, b);
            framematch_demo(ra::wrank<1, 1>(plus2real), a, b);
        }
        auto plus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        {
            auto v = ra::wrank<0, 2>(plus2real_print);
            using FM = ra::Framematch<decltype(v), tuple<decltype(a.iter()), decltype(b.iter())>>;
            cout << "width of fm: " << mp::len<FM::R> << ", depth: " << FM::depth << endl;
            cout << mp::print_int_list<FM::R> {} << endl;
            auto af0 = ra::applyframes<mp::ref<FM::R, 0>, FM::depth>(a.iter());
            auto af1 = ra::applyframes<mp::ref<FM::R, 1>, FM::depth>(b.iter());
            cout << sizeof(af0) << endl;
            cout << sizeof(af1) << endl;
            auto ryn = ra::ryn<FM>(FM::op(v), af0, af1);
            cout << sizeof(ryn) << "\n" << endl;
            cout << "ryn rank: " << ryn.rank() << endl;
            for (int k=0; k<ryn.rank(); ++k) {
                cout << ryn.size(k) << ": " << driver<0>(ryn.t, k) << endl;
            }
            ra::ply_ravel(ryn);
        }
    }
    tr.section("wrank tests 0-1");
    {
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::wrank<0, 1>(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::none),
                          ra::Unique<real, 1>({4}, ra::none));
        nested_wrank_demo(ra::wrank<0, 1>(ra::wrank<0, 0>(minus2real_print)),
                          ra::Unique<real, 1>({3}, ra::none),
                          ra::Unique<real, 1>({3}, ra::none));
    }
    tr.section("wrank tests 1-0");
    {
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::wrank<1, 0>(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::none),
                          ra::Unique<real, 1>({4}, ra::none));
        nested_wrank_demo(ra::wrank<1, 0>(ra::wrank<0, 0>(minus2real_print)),
                          ra::Unique<real, 1>({3}, ra::none),
                          ra::Unique<real, 1>({4}, ra::none));
    }
    tr.section("wrank tests 0-0 (nop), case 1 - exact match");
    {
// This uses the applyframes specialization for 'do nothing' (TODO if there's one).
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::wrank<0, 0>(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::none),
                          ra::Unique<real, 1>({3}, ra::none));
    }
    tr.section("wrank tests 0-0 (nop), case 2 - non-exact frame match");
    {
// This uses the applyframes specialization for 'do nothing' (TODO if there's one).
        auto minus2real_print = [](real a, real b) { cout << (a - b) << " "; };
        nested_wrank_demo(ra::wrank<0, 0>(minus2real_print),
                          ra::Unique<real, 2>({3, 4}, ra::none),
                          ra::Unique<real, 1>({3}, ra::none));
        nested_wrank_demo(ra::wrank<0, 0>(minus2real_print),
                          ra::Unique<real, 1>({3}, ra::none),
                          ra::Unique<real, 2>({3, 4}, ra::none));
    }
    tr.section("wrank tests 1-1-0, init array with outer product");
    {
        auto minus2real = [](real & c, real a, real b) { c = a-b; };
        ra::Unique<real, 1> a({3}, ra::none);
        ra::Unique<real, 1> b({4}, ra::none);
        std::iota(a.begin(), a.end(), 10);
        std::iota(b.begin(), b.end(), 1);
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
        ra::Unique<real, 2> c({3, 4}, ra::none);
        ra::ply(ra::expr(ra::wrank<1, 0, 1>(minus2real), c.iter(), a.iter(), b.iter()));
        cout << "c: " << c << endl;
        real checkc34[3*4] = { /* 10-[1 2 3 4] */ 9, 8, 7, 6,
                               /* 11-[1 2 3 4] */ 10, 9, 8, 7,
                               /* 12-[1 2 3 4] */ 11, 10, 9, 8 };
        tr.test(std::equal(checkc34, checkc34+3*4, c.begin()));
        ra::Unique<real, 2> d34(ra::expr(ra::wrank<0, 1>(std::minus<real>()), a.iter(), b.iter()));
        cout << "d34: " << d34 << endl;
        tr.test(std::equal(checkc34, checkc34+3*4, d34.begin()));
        real checkc43[3*4] = { /* [10 11 12]-1 */ 9, 10, 11,
                               /* [10 11 12]-2 */ 8, 9, 10,
                               /* [10 11 12]-3 */ 7, 8, 9,
                               /* [10 11 12]-4 */ 6, 7, 8 };
        ra::Unique<real, 2> d43(ra::expr(ra::wrank<1, 0>(std::minus<real>()), a.iter(), b.iter()));
        cout << "d43: " << d43 << endl;
        tr.test(d43.size(0)==4 && d43.size(1)==3);
        tr.test(std::equal(checkc43, checkc43+3*4, d43.begin()));
    }
    tr.section("recipe for unbeatable subscripts in _from_ operator");
    {
        ra::Unique<int, 1> a({3}, ra::none);
        ra::Unique<int, 1> b({4}, ra::none);
        std::iota(a.begin(), a.end(), 10);
        std::iota(b.begin(), b.end(), 1);
        ra::Unique<real, 2> c({100, 100}, ra::none);
        std::iota(c.begin(), c.end(), 0);

        real checkd[3*4] = { 1001, 1002, 1003, 1004,  1101, 1102, 1103, 1104,  1201, 1202, 1203, 1204 };
// default auto is value, so need to speficy.
#define EXPR ra::expr(ra::wrank<0, 1>([&c](int a, int b) -> decltype(auto) { return c(a, b); } ), \
                      a.iter(), b.iter())
        std::ostringstream os;
        os << EXPR << endl;
        ra::Unique<real, 2> cc {};
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
// expr-way BUG use of test_eq fails (??)
        assert(every(c==where(ra::_0>=10 && ra::_0<=12 && ra::_1>=1 && ra::_1<=4, 7, ra::_0*100+ra::_1)));
// looping...
        bool valid = true;
        for (int i=0; i<c.size(0); ++i) {
            for (int j=0; j<c.size(1); ++j) {
                valid = valid && ((i>=10 && i<=12 && j>=1 && j<=4 ? 7 : i*100+j) == c(i, j));
            }
        }
        tr.test(valid);
    }
    tr.section("rank conjunction / empty");
    {
    }
    tr.section("static rank() in ra::Ryn");
    {
        ra::Unique<real, 3> a({2, 2, 2}, 1.);
        ra::Unique<real, 3> b({2, 2, 2}, 2.);
        real y = 0;
        auto e = ra::expr(ra::wrank<0, 0>([&y](real const a, real const b) { y += a*b; }), a.iter(), b.iter());
        static_assert(3==e.rank(), "bad rank in static rank expr");
        ra::ply_ravel(ra::expr(ra::wrank<0, 0>([&y](real const a, real const b) { y += a*b; }), a.iter(), b.iter()));
        tr.test_eq(16, y);
    }
    tr.section("outer product variants");
    {
        ra::Big<real, 2> a({2, 3}, ra::_0 - ra::_1);
        ra::Big<real, 2> b({3, 2}, ra::_1 - 2*ra::_0);
        ra::Big<real, 2> c1 = gemm(a, b);
        cout << "matrix a * b: \n" << c1 << endl;
// matrix product as outer product + reduction (no reductions yet, so manually).
        {
            ra::Big<real, 3> d = ra::expr(ra::wrank<1, 2>(ra::wrank<0, 1>(ra::times())), start(a), start(b));
            cout << "d(i,k,j) = a(i,k)*b(k,j): \n" << d << endl;
            ra::Big<real, 2> c2({d.size(0), d.size(2)}, 0.);
            for (int k=0; k<d.size(1); ++k) {
                c2 += d(ra::all, k, ra::all);
            }
            tr.test_eq(c1, c2);
        }
// do the k-reduction by plying with wrank.
        {
            ra::Big<real, 2> c2({a.size(0), b.size(1)}, 0.);
            ra::ply(ra::expr(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto & c, auto && a, auto && b) { c += a*b; })),
                             start(c2), start(a), start(b)));
            cout << "sum_k a(i,k)*b(k,j): \n" << c2 << endl;
            tr.test_eq(c1, c2);
        }
    }
    tr.section("stencil test for ApplyFrames::keep_stride. Reduced from test/bench-stencil2.C");
    {
        int nx = 4;
        int ny = 4;
        int ts = 4; // must be even b/c of swap

        auto I = ra::iota(nx-2, 1);
        auto J = ra::iota(ny-2, 1);

        constexpr ra::Small<real, 3, 3> mask = { 0, 1, 0,
                                                 1, -4, 1,
                                                 0, 1, 0 };

        real value = 1;

        auto f_raw = [&](ra::View<real, 2> & A, ra::View<real, 2> & Anext, ra::View<real, 4> & Astencil)
            {
                for (int t=0; t<ts; ++t) {
                    for (int i=1; i+1<nx; ++i) {
                        for (int j=1; j+1<ny; ++j) {
                            Anext(i, j) = -4*A(i, j)
                            + A(i+1, j) + A(i, j+1)
                            + A(i-1, j) + A(i, j-1);
                        }
                    }
                    std::swap(A.p, Anext.p);
                }
            };
        auto f_sumprod = [&](ra::View<real, 2> & A, ra::View<real, 2> & Anext, ra::View<real, 4> & Astencil)
            {
                for (int t=0; t!=ts; ++t) {
                    Astencil.p = A.data();
                    Anext(I, J) = 0; // TODO miss notation for sum-of-axes without preparing destination...
                    Anext(I, J) += map(ra::wrank<2, 2>(ra::times()), Astencil, mask);
                    std::swap(A.p, Anext.p);
                }
            };
        auto bench = [&](auto & A, auto & Anext, auto & Astencil, auto && ref, auto && tag, auto && f)
            {
                A = value;
                Anext = 0.;

                f(A, Anext, Astencil);

                tr.info(tag).test_rel_error(ref, A, 1e-11);
            };

        ra::Big<real, 2> Aref;
        ra::Big<real, 2> A({nx, ny}, 1.);
        ra::Big<real, 2> Anext({nx, ny}, 0.);
        auto Astencil = stencil(A, 1, 1);
        cout << "Astencil " << format_array(Astencil(0, 0, ra::dots<2>), "|", " ") << endl;
#define BENCH(ref, op) bench(A, Anext, Astencil, ref, STRINGIZE(op), op);
        BENCH(A, f_raw);
        Aref = ra::Big<real, 2>(A);
        BENCH(Aref, f_sumprod);
    }
    tr.section("Iota with dead axes");
    {
        ra::Big<int, 2> a = from([](auto && i, auto && j) { return i-j; }, ra::iota(3), ra::iota(3));
        tr.test_eq(ra::Big<int, 2>({3, 3}, {0, -1, -2,  1, 0, -1,  2, 1, 0}), a);
    }
    tr.section("Vector with dead axes");
    {
        std::vector<int> i = {0, 1, 2};
        ra::Big<int, 2> a = ra::from([](auto && i, auto && j) { return i-j; }, i, i);
        tr.test_eq(ra::Big<int, 2>({3, 3}, {0, -1, -2,  1, 0, -1,  2, 1, 0}), a);
    }
    tr.section("no arguments -> zero rank");
    {
        int x = ra::from([]() { return 3; });
        tr.test_eq(3, x);
    }
    tr.section("counting ops");
    {
        std::atomic<int> i { 0 };
        auto fi = [&i](auto && x) { ++i; return x; };
        std::atomic<int> j { 0 };
        auto fj = [&j](auto && x) { ++j; return x; };
        ra::Big<int, 2> a = from(ra::minus(), map(fi, ra::iota(7)), map(fj, ra::iota(9)));
        tr.test_eq(ra::_0-ra::_1, a);
        tr.info("FIXME").skip().test_eq(7, int(i));
        tr.info("FIXME").skip().test_eq(9, int(j));
    }
    return tr.summary();
}
