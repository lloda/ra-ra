
// (c) Daniel Llorens - 2013-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-0.C
/// @brief Checks for ra:: arrays, iterators.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"
#include "ra/mpdebug.H"

using std::cout; using std::endl; using std::flush;
template <int i> using TI = ra::TensorIndex<i, int>;

template <class A>
void CheckArrayOutput(TestRecorder & tr, A const & a, real * begin)
{
    std::ostringstream o;
    o << a;
    cout << "a: " << o.str() << endl;
    std::istringstream i(o.str());
    std::istream_iterator<real> iend;
    std::istream_iterator<real> ibegin(i);
    tr.test(std::equal(ibegin, iend, begin));
}

template <class A>
void CheckArrayIO(TestRecorder & tr, A const & a, real * begin)
{
    std::ostringstream o;
    o << a;
    cout << "a: " << o.str() << endl;
    {
        std::istringstream i(o.str());
        std::istream_iterator<real> iend;
        std::istream_iterator<real> ibegin(i);
        tr.test(std::equal(ibegin, iend, begin));
    }
    {
        std::istringstream i(o.str());
        A b(ra::init_not);
        tr.test(bool(i));
        i >> b;
        auto as = ra::ra_traits<A>::shape(a);
        auto bs = ra::ra_traits<A>::shape(b);
        cout << "o: " << o.str() << endl;
        cout << "as: " << ra::vector(as) << ", bs: " << ra::vector(bs) << endl;
        cout << "a: " << a << ", b: " << b << endl;
        tr.test(std::equal(as.begin(), as.end(), bs.begin()));
        tr.test(std::equal(a.begin(), a.begin(), b.begin()));
    }
}

template <int i> using UU = decltype(std::declval<ra::Unique<real, i>>().iter());
using SS = decltype(ra::scalar(1));

template <class A>
void CheckReverse(TestRecorder & tr, A && a)
{
    std::iota(a.begin(), a.end(), 1);
    cout << "a: " << a << endl;
    auto b0 = reverse(a, 0);
    cout << "b: " << b0 << endl;
    real check0[24] = { 17, 18, 19, 20,   21, 22, 23, 24,
                        9, 10, 11, 12,  13, 14, 15, 16,
                        1, 2, 3, 4,     5, 6, 7, 8 };
    tr.test(std::equal(check0, check0+24, b0.begin()));

    auto b1 = reverse(a, 1);
    cout << "b: " << b1 << endl;
    real check1[24] = { 5, 6, 7, 8,      1, 2, 3, 4,
                        13, 14, 15, 16,  9, 10, 11, 12,
                        21, 22, 23, 24,  17, 18, 19, 20 };
    tr.test(std::equal(check1, check1+24, b1.begin()));

    auto b2 = reverse(a, 2);
    cout << "b: " << b2 << endl;
    real check2[24] = { 4, 3, 2, 1,      8, 7, 6, 5,
                        12, 11, 10, 9,   16, 15, 14, 13,
                        20, 19, 18, 17,  24, 23, 22, 21 };
    tr.test(std::equal(check2, check2+24, b2.begin()));
}

template <class A>
void CheckTranspose1(TestRecorder & tr, A && a)
{
    std::iota(a.begin(), a.end(), 1);
    cout << "a: " << a << endl;
    auto b = transpose(a, ra::Small<int, 2>{1, 0});
    cout << "b: " << b << endl;
    real check[6] = {1, 3, 5, 2, 4, 6};
    tr.test(std::equal(b.begin(), b.end(), check));
}

int main()
{
    TestRecorder tr(std::cout);
    section("internal fields");
    {
        {
            real aa[10];
            aa[0] = 99;
            ra::Raw<real, 1> a { {{10, 1}}, aa };
            cout << "a1.p[0]: " << a.p[0] << endl;
        }
        {
            real aa[6] = { 1, 2, 3, 4, 5, 6 };
            aa[0] = 99;
// @BUG strange that {{3,2},{2,1}} doesn't work...
            ra::Raw<real, 2> a { { ra::Dim {3, 2}, ra::Dim {2, 1} }, aa };
            cout << "a2.p[0]: " << a.p[0] << endl;
        }
        {
            real aa[20];
            aa[19] = 77;
            ra::Raw<real> a = { {{10, 2}, {2, 1}}, aa };
            tr.test_eq(10, a.dim[0].size);
            tr.test_eq(2, a.dim[1].size);
            cout << "a.p(3, 4): " << a.p[19] << endl;
            tr.test_eq(77, a.p[19]);
        }
        {
            auto pp = std::unique_ptr<real []>(new real[10]);
            pp[9] = 77;
            real * p = pp.get();
            ra::Unique<real> a(ra::init_not);
            a.store = std::move(pp);
            a.p = p;
            a.dim = {{5, 2}, {2, 1}};
            tr.test_eq(5, a.dim[0].size);
            tr.test_eq(2, a.dim[1].size);
            cout << "a.p(3, 4): " << a.p[9] << endl;
            tr.test_eq(77, a.p[9]);
        }
        {
            auto pp = std::shared_ptr<real>(new real[10]);
            pp.get()[9] = 88;
            real * p = pp.get();
            ra::Shared<real> a(ra::init_not);
            a.store = pp;
            a.p = p;
            a.dim = {{5, 2}, {2, 1}};
            tr.test_eq(5, a.dim[0].size);
            tr.test_eq(2, a.dim[1].size);
            cout << "a.p(3, 4): " << a.p[9] << endl;
            tr.test_eq(88, a.p[9]);
        }
        {
            auto pp = std::vector<real>(10);
            pp[9] = 99;
            real * p = pp.data();
            ra::Owned<real> a(ra::init_not);
            a.store = pp;
            a.p = p;
            a.dim = {{5, 2}, {2, 1}};
            tr.test_eq(5, a.dim[0].size);
            tr.test_eq(2, a.dim[1].size);
            cout << "a.p(3, 4): " << a.p[9] << endl;
            tr.test_eq(99, a.p[9]);
        }
    }
    section("rank 0 -> scalar with Small");
    {
        auto rank0test0 = [](real & a) { a *= 2; };
        auto rank0test1 = [](real const & a) { return a*2; };
        ra::Small<real> a { 33 };
        static_assert(sizeof(a)==sizeof(real), "bad assumption");
        rank0test0(a);
        tr.test_eq(66, a);
        real b = rank0test1(a);
        tr.test_eq(66, a);
        tr.test_eq(132, b);
    }
    section("(170) rank 0 -> scalar with Raw");
    {
        auto rank0test0 = [](real & a) { a *= 2; };
        auto rank0test1 = [](real const & a) { return a*2; };
        real x { 99 };
        ra::Raw<real, 0> a { {}, &x };
        tr.test_eq(1, a.size());

// ra::Raw<T, 0> contains a pointer to T plus the dope vector of type Small<Dim, 0>. But after I put the data of Small in Small itself instead of in SmallBase, sizeof(Small<T, 0>) is no longer 0. That was specific of gcc, so better not to depend on it anyway...
        cout << "a()" << a() << endl;
        cout << "sizeof(a())" << sizeof(a()) << endl;
        cout << "sizeof(real *)" << sizeof(real *) << endl;
        // static_assert(sizeof(a())==sizeof(real *), "bad assumption");

        rank0test0(a);
        tr.test_eq(198, a);
        real b = rank0test1(a);
        tr.test_eq(198, a);
        tr.test_eq(396, b);
    }
    section("ra traits");
    {
        {
            using real2x3 = ra::Small<real, 2, 3>;
            real2x3 r { 1, 2, 3, 4, 5, 6 };
            cout << "r rank: " << ra::ra_traits<real2x3>::rank(r) << endl;
            tr.test_eq(6, ra::ra_traits<real2x3>::size(r));
        }
        {
            real pool[6] = { 1, 2, 3, 4, 5, 6 };
            ra::Raw<real> r { {{3, 2}, {2, 1}}, pool };
            cout << "r rank: " << ra::ra_traits<ra::Raw<real>>::rank(r) << endl;
            tr.test_eq(6, ra::ra_traits<ra::Raw<real>>::size(r));
        }
        {
            real pool[6] = { 1, 2, 3, 4, 5, 6 };
            ra::Raw<real, 2> r { { ra::Dim {3, 2}, ra::Dim {2, 1}}, pool };
            cout << "r rank: " << ra::ra_traits<ra::Raw<real, 2>>::rank(r) << endl;
            tr.test_eq(6, ra::ra_traits<ra::Raw<real, 2>>::size(r));
        }
    }
    section("iterator");
    {
        real chk[6] = { 0, 0, 0, 0, 0, 0 };
        real pool[6] = { 1, 2, 3, 4, 5, 6 };
        ra::Raw<real> r { {{3, 2}, {2, 1}}, pool };
        ra::ra_iterator<ra::Raw<real>> it(r.dim, r.p);
        cout << "as iterator: " << ra::print_iterator(it) << endl;
        cout << "Raw<real> it.c.p: " << it.c.p << endl;
        std::copy(r.begin(), r.end(), chk);
        tr.test(std::equal(pool, pool+6, r.begin()));
    }
    {
        real chk[6] = { 0, 0, 0, 0, 0, 0 };
        real pool[6] = { 1, 2, 3, 4, 5, 6 };
        ra::Raw<real, 1> r { { ra::Dim {6, 1}}, pool };
        ra::ra_iterator<ra::Raw<real, 1>> it(r.dim, r.p);
        cout << "Raw<real, 1> it.c.p: " << it.c.p << endl;
        std::copy(r.begin(), r.end(), chk);
        tr.test(std::equal(pool, pool+6, r.begin()));
    }
// STL-type iterators.
    {
        real rpool[6] = { 1, 2, 3, 4, 5, 6 };
        ra::Raw<real, 1> r { {ra::Dim {6, 1}}, rpool };

        real spool[6] = { 0, 0, 0, 0, 0, 0 };
        ra::Raw<real> s { {{3, 1}, {2, 3}}, spool };

        std::copy(r.begin(), r.end(), s.begin());
        std::copy(spool, spool+6, std::ostream_iterator<real>(cout, " "));

        real cpool[6] = { 1, 3, 5, 2, 4, 6 };
        tr.test(std::equal(cpool, cpool+6, spool));
        cout << endl;
    }
    section("storage types");
    {
        real pool[6] = { 1, 2, 3, 4, 5, 6 };

        ra::Shared<real> s({3, 2}, pool, pool+6);
        cout << "s rank: " << s.rank() << endl;
        tr.test(std::equal(pool, pool+6, s.begin()));

        ra::Unique<real> u({3, 2}, pool, pool+6);
        cout << "u rank: " << u.rank() << endl;
        tr.test(std::equal(pool, pool+6, u.begin()));

        ra::Owned<real> o({3, 2}, pool, pool+6);
        cout << "o rank: " << o.rank() << endl;
        tr.test(std::equal(pool, pool+6, o.begin()));
    }
    section("driver selection");
    {
        static_assert(TI<0>::rank_s()==1, "bad TI rank");
        static_assert(ra::pick_driver<UU<0>, UU<1> >::value==1, "bad driver 1a");
        static_assert(ra::pick_driver<TI<1>, UU<2> >::value==1, "bad driver 1b");

// these two depend on TI<w>::rank_s() being w+1, which I haven't settled on.
        static_assert(TI<1>::rank_s()==2, "bad TI rank");
        static_assert(ra::pick_driver<TI<0>, TI<1> >::value==1, "bad driver 1c");

        static_assert(UU<0>::size_s()==1, "bad size_s 0");
        static_assert(SS::size_s()==1, "bad size_s 1");
        static_assert(ra::pick_driver<UU<0>, SS>::value==0, "bad size_s 2");
// static size/rank identical; prefer the first.
        static_assert(ra::largest_rank<UU<0>, SS>::value==0, "bad match 1a");
        static_assert(ra::largest_rank<SS, UU<0>>::value==0, "bad match 1b");
// prefer the larger rank.
        static_assert(ra::largest_rank<SS, UU<1>>::value==1, "bad match 2a");
        static_assert(ra::largest_rank<UU<1>, SS>::value==0, "bad match 2b");
// never choose TensorIndex.
        static_assert(ra::pick_driver<UU<2>, TI<0>>::value==0, "bad match 3a");
        static_assert(ra::pick_driver<TI<0>, UU<2>>::value==1, "bad match 3b");
// static size/rank identical; prefer the first.
        static_assert(ra::pick_driver<UU<2>, UU<2>>::value==0, "bad match 4");
        static_assert(ra::largest_rank<UU<2>, TI<0>, UU<2>>::value==0, "bad match 5a");
// dynamic rank counts as +inf.
        static_assert(ra::gt_rank(ra::RANK_ANY, 2), "bad match 6a");
        static_assert(ra::gt_rank(UU<ra::RANK_ANY>::rank_s(), UU<2>::rank_s()), "bad match 6b");
        static_assert(!ra::gt_rank(UU<2>::rank_s(), UU<ra::RANK_ANY>::rank_s()), "bad match 6c");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, UU<2>>::value==0, "bad match 6d");
        static_assert(ra::pick_driver<UU<2>, UU<ra::RANK_ANY>>::value==1, "bad match 6e");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, UU<ra::RANK_ANY>>::value==0, "bad match 6f");
        static_assert(ra::pick_driver<TI<0>, UU<ra::RANK_ANY>>::value==1, "bad match 6g");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, TI<0>>::value==0, "bad match 6h");
        static_assert(ra::largest_rank<TI<0>, UU<ra::RANK_ANY>>::value==1, "bad match 6i");
        static_assert(ra::largest_rank<UU<ra::RANK_ANY>, TI<0>>::value==0, "bad match 6j");
        static_assert(ra::largest_rank<UU<2>, UU<ra::RANK_ANY>, TI<0>>::value==1, "bad match 6k");
        static_assert(ra::largest_rank<UU<1>, UU<2>, UU<3>>::value==2, "bad match 6l");
        static_assert(ra::largest_rank<UU<2>, TI<0>, UU<ra::RANK_ANY>>::value==2, "bad match 6m");
// more cases with +2 candidates.
        static_assert(ra::largest_rank<UU<3>, UU<1>, TI<0>>::value==0, "bad match 7b");
        static_assert(ra::largest_rank<TI<0>, UU<3>, UU<1>>::value==1, "bad match 7c");
        static_assert(ra::largest_rank<UU<1>, TI<0>, UU<3>>::value==2, "bad match 7d");
        static_assert(ra::largest_rank<UU<1>, TI<0>, UU<3>>::value==2, "bad match 7e");
    }
    section("copy between arrays, construct from iterator pair");
    {
        // copy from Fortran order to C order.
        real rpool[6] = { 1, 2, 3, 4, 5, 6 };
        real check[6] = { 1, 4, 2, 5, 3, 6 };

        ra::Raw<real> r { {{3, 1}, {2, 3}}, rpool };
        std::copy(r.begin(), r.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
        tr.test(std::equal(check, check+6, r.begin()));

        ra::Unique<real> u({3, 2}, r.begin(), r.end());
        std::copy(u.begin(), u.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
        tr.test(std::equal(check, check+6, u.begin()));

        // @TODO Have strides in Small.
        ra::Small<real, 3, 2> s { 1, 4, 2, 5, 3, 6 };
        std::copy(s.begin(), s.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
        tr.test(std::equal(check, check+6, s.begin()));

        // construct Small from iterators.
        ra::Small<real, 3, 2> z(r.begin(), r.end());
        std::copy(z.begin(), z.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
        tr.test(std::equal(check, check+6, z.begin()));
    }
// In this case, the Raw + shape provides the driver.
    section("construct Raw from shape + driverless xpr");
    {
        static_assert(ra::has_tensorindex<std::decay_t<decltype(ra::_0)>>::value,
                      "bad has_tensorindex test 0");
        static_assert(ra::has_tensorindex<TI<0>>::value,
                      "bad has_tensorindex test 1");

        ra::Unique<int, 2> a({3, 2}, ra::unspecified);
        auto dyn = ra::expr([](int & a, int b) { a = b; }, a.iter(), ra::_0);
        static_assert(ra::has_tensorindex<std::decay_t<decltype(dyn)>>::value,
                      "bad has_tensorindex test 2");

        auto dyn2 = ra::expr([](int & dest, int const & src) { dest = src; }, a.iter(), ra::_0);
        static_assert(ra::has_tensorindex<std::decay_t<decltype(dyn2)>>::value,
                      "bad has_tensorindex test 3");

        {
            int checkb[6] = { 0, 0, 1, 1, 2, 2 };
            ra::Unique<int, 2> b({3, 2}, ra::_0);
            cout << b << endl;
            tr.test(std::equal(checkb, checkb+6, b.begin()));
        }
// This requires the driverless xpr dyn(scalar, tensorindex) to be constructible.
        {
            int checkb[6] = { 3, 3, 4, 4, 5, 5 };
            ra::Unique<int, 2> b({3, 2}, ra::expr([](int a, int b) { return a+b; }, ra::scalar(3), ra::_0));
            cout << b << "\n" << endl;
            tr.test(std::equal(checkb, checkb+6, b.begin()));
        }
        {
            int checkb[6] = { 0, -1, 1, 0, 2, 1 };
            ra::Unique<int, 2> b({3, 2}, ra::expr([](int a, int b) { return a-b; }, ra::_0, ra::_1));
            cout << b << "\n" << endl;
            tr.test(std::equal(checkb, checkb+6, b.begin()));
        }
// @TODO Check this is an error (chosen driver is TensorIndex<2>, that can't drive).
        // {
        //     ra::Unique<int, 2> b({3, 2}, ra::expr([](int a, int b) { return a-b; }, ra::_2, ra::_1));
        //     cout << b << endl;
        // }
// @TODO Could this be made to work?
        // {
        //     ra::Unique<int> b({3, 2}, ra::expr([](int a, int b) { return a-b; }, ra::_2, ra::_1));
        //     cout << b << endl;
        // }
    }
    section("construct Raw from shape + xpr");
    {
        real checka[6] = { 9, 9, 9, 9, 9, 9 };
        ra::Unique<real, 2> a({3, 2}, ra::scalar(9));
        cout << a << endl;
        tr.test(std::equal(checka, checka+6, a.begin()));
        real checkb[6] = { 11, 11, 22, 22, 33, 33 };
        ra::Unique<real, 2> b({3, 2}, ra::Small<real, 3> { 11, 22, 33 });
        cout << b << endl;
        tr.test(std::equal(checkb, checkb+6, b.begin()));
    }
    section("construct Unique from Unique");
    {
        real check[6] = { 2, 3, 1, 4, 8, 9 };
        ra::Unique<real, 2> a({3, 2}, { 2, 3, 1, 4, 8, 9 });
        // ra::Unique<real, 2> b(a); // error; need temp
        ra::Unique<real, 2> c(ra::Unique<real, 2>({3, 2}, { 2, 3, 1, 4, 8, 9 })); // ok; from actual temp
        cout << "c: " << c << endl;
        tr.test(std::equal(check, check+6, c.begin()));
        ra::Unique<real, 2> d(std::move(a)); // ok; from fake temp
        cout << "d: " << d << endl;
        tr.test(std::equal(check, check+6, d.begin()));
    }
    section("construct from xpr having its own shape");
    {
        ra::Unique<real, 0> a(ra::scalar(33));
        ra::Unique<real> b(ra::scalar(44));
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
// b.rank() is runtime, so b()==44. and the whole assert argument become array xprs when ra-operators.H is included.
        tr.test_eq(0, b.rank());
        tr.test_eq(1, b.size());
        tr.test_eq(44, b());
        b = 55.;
        cout << "b: " << b << endl;
// see above for b.rank().
        tr.test_eq(0, b.rank());
        tr.test_eq(1, b.size());
        tr.test_eq(55., b());
    }
    section("rank 0 -> scalar with storage type");
    {
        auto rank0test0 = [](real & a) { a *= 2; };
        auto rank0test1 = [](real const & a) { return a*2; };
        ra::Unique<real, 0> a(ra::scalar(33));
        tr.test_eq(1, a.size());

// See note in (170).
        // static_assert(sizeof(a())==sizeof(real *), "bad assumption");

        rank0test0(a);
        tr.test_eq(66, a);
        real b = rank0test1(a);
        tr.test_eq(66, a);
        tr.test_eq(132, b);
    }
    section("rank 0 -> scalar with storage type, explicit size");
    {
        auto rank0test0 = [](real & a) { a *= 2; };
        auto rank0test1 = [](real const & a) { return a*2; };
        ra::Unique<real, 0> a({}, ra::scalar(33));
        tr.test_eq(1, a.size());

// See note in (170).
        // static_assert(sizeof(a())==sizeof(real *), "bad assumption");
        rank0test0(a);
        tr.test_eq(66, a);
        real b = rank0test1(a);
        tr.test_eq(66, a);
        tr.test_eq(132, b);
    }
    section("constructors from data in initializer_list");
    {
        real checka[6] = { 2, 3, 1, 4, 8, 9 };
        {
            ra::Unique<real, 2> a({2, 3}, { 2, 3, 1, 4, 8, 9 });
            tr.test_eq(2, a.dim[0].size);
            tr.test_eq(3, a.dim[1].size);
            tr.test(std::equal(a.begin(), a.end(), checka));
        }
        {
            ra::Unique<real> a { 2, 3, 1, 4, 8, 9 };
            tr.test_eq(6, a.size());
            tr.test_eq(1, a.rank());
            tr.test(std::equal(a.begin(), a.end(), checka));
            ra::Unique<real> b ({ 2, 3, 1, 4, 8, 9 });
            tr.test_eq(6, b.size());
            tr.test_eq(1, b.rank());
            tr.test(std::equal(b.begin(), b.end(), checka));
        }
        {
            ra::Unique<real, 1> a { 2, 3, 1, 4, 8, 9 };
            tr.test_eq(6, a.size());
            tr.test_eq(1, a.rank());
            tr.test(std::equal(a.begin(), a.end(), checka));
            ra::Unique<real, 1> b ({ 2, 3, 1, 4, 8, 9 });
            tr.test_eq(6, b.size());
            tr.test_eq(1, b.rank());
            tr.test(std::equal(b.begin(), b.end(), checka));
        }
    }
    section("transpose of 0 rank");
    {
        ra::Unique<real, 0> a({}, 99);
        auto b = transpose(a, ra::Small<int, 0> {});
        tr.test_eq(0, b.rank());
        tr.test_eq(99, b());
    }
    section("row-major assignment from initializer_list, rank 2");
    {
        ra::Unique<real, 2> a({3, 2}, ra::unspecified);
        a = { 2, 3, 1, 4, 8, 9 };
        tr.test_eq(2, a(0, 0));
        tr.test_eq(3, a(0, 1));
        tr.test_eq(1, a(1, 0));
        tr.test_eq(4, a(1, 1));
        tr.test_eq(8, a(2, 0));
        tr.test_eq(9, a(2, 1));

        auto b = transpose(a, {1, 0});
        b = { 2, 3, 1, 4, 8, 9 };
        tr.test_eq(2, b(0, 0));
        tr.test_eq(3, b(0, 1));
        tr.test_eq(1, b(0, 2));
        tr.test_eq(4, b(1, 0));
        tr.test_eq(8, b(1, 1));
        tr.test_eq(9, b(1, 2));

        tr.test_eq(2, a(0, 0));
        tr.test_eq(4, a(0, 1));
        tr.test_eq(3, a(1, 0));
        tr.test_eq(8, a(1, 1));
        tr.test_eq(1, a(2, 0));
        tr.test_eq(9, a(2, 1));

        auto c = transpose(a, {1, 0});
        tr.test(a.data()==c.data()); // pointers are not ra::scalars. Dunno if this deserves fixing.
        tr.test_eq(a.size(0), c.size(1));
        tr.test_eq(a.size(1), c.size(0));
        tr.test_eq(b, c);
    }
    section("row-major assignment from initializer_list, rank 1");
    {
        ra::Owned<real, 1> a({5}, ra::unspecified);
        a = { 2, 3, 1, 4, 8 };
        tr.test_eq(2, a(0));
        tr.test_eq(3, a(1));
        tr.test_eq(1, a(2));
        tr.test_eq(4, a(3));
        tr.test_eq(8, a(4));
    }
    section("subscripts");
    {
        section("Raw fixed rank == 0");
        {
            real x = 99;
            ra::Raw<real, 0> y(ra::Small<int, 0>{}, &x);
            tr.test_eq(99, y());
            tr.test_eq(99, y);
            real u = 77.;
            ra::Raw<real, 0> v(ra::Small<int, 0>{}, &u);
            y = v;
            tr.test_eq(77, u);
            tr.test_eq(77, v);
            tr.test_eq(77, x);
            tr.test_eq(77, y);
        }
        section("Raw fixed rank > 0");
        {
            real rpool[6] = { 1, 2, 3, 4, 5, 6 };
            ra::Raw<real, 2> r { {ra::Dim {3, 1}, ra::Dim {2, 3}}, rpool };
            cout << "org" << endl;
            std::copy(r.begin(), r.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;

            real rcheck0[6] = { 1, 4, 2, 5, 3, 6 };
            auto r0 = r();
            cout << "r0" << endl;
            std::copy(r0.begin(), r0.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            tr.test(std::equal(r0.begin(), r0.end(), rcheck0));
            ra::Small<int, 0> i0 {};
            tr.info("ra::Small<int, 0> rank").test_eq(1, i0.rank());
            auto r0a = r.at(ra::Small<int, 0> {});
            tr.test(std::equal(r0a.begin(), r0a.end(), rcheck0));

            real rcheck1[2] = { 2, 5 };
            auto r1 = r(1);
            cout << "r1" << endl;
            std::copy(r1.begin(), r1.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            tr.test(std::equal(r1.begin(), r1.end(), rcheck1));
            auto r1a = r.at(ra::Small<int, 1> {1});
            tr.test(std::equal(r1a.begin(), r1a.end(), rcheck1));

            real rcheck2[2] = { 5 };
            auto r2 = r(1, 1);
            cout << "r2" << endl;

            // Does r(1, 1) return rank 0, or a scalar directly?
            // std::copy(r2.begin(), r2.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            // tr.test(std::equal(r2.begin(), r2.end(), rcheck2));
            cout << r2 << endl;
            tr.test_eq(5, r2);

            auto r2a = r.at(ra::Small<int, 2> {1, 1});
            tr.test(std::equal(r2a.begin(), r2a.end(), rcheck2));
        }
        // @TODO Subscript a rank>1 array, multiple selectors, mixed beatable & unbeatable selectors.
        section("Raw fixed rank, unbeatable subscripts");
        {
            ra::Unique<real, 1> a = {1, 2, 3, 4};
            ra::Unique<int, 1> i = {3, 1, 2};
            cout << a(i) << endl;
            ra::Unique<real, 1> ai = a(i);
            tr.test_eq(i.size(), ai.size());
            tr.test_eq(a[i[0]], ai[0]);
            tr.test_eq(a[i[1]], ai[1]);
            tr.test_eq(a[i[2]], ai[2]);
            a(i) = ra::Unique<real, 1> {7, 8, 9};
            cout << a << endl;
            tr.test_eq(4, a.size());
            tr.test_eq(1, a[0]);
            tr.test_eq(a[i[0]], 7);
            tr.test_eq(a[i[1]], 8);
            tr.test_eq(a[i[2]], 9);
        }
        section("Raw var rank");
        {
            real rpool[6] = { 1, 2, 3, 4, 5, 6 };
            ra::Raw<real> r { {ra::Dim {3, 1}, ra::Dim {2, 3}}, rpool };
            tr.test_eq(2, r.rank());
            cout << "org" << endl;
            std::copy(r.begin(), r.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;

            real rcheck0[6] = { 1, 4, 2, 5, 3, 6 };
            auto r0 = r();
            auto r0a = r.at(ra::Small<int, 0> {});
            tr.test_eq(2, r0a.rank());
            tr.test_eq(2, r0.rank());
            cout << "r0" << endl;
            std::copy(r0.begin(), r0.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            tr.test(std::equal(r0.begin(), r0.end(), rcheck0));
            tr.test(std::equal(r0a.begin(), r0a.end(), rcheck0));

            real rcheck1[2] = { 2, 5 };
            auto r1 = r(1);
            auto r1a = r.at(ra::Small<int, 1> {1});
            tr.test_eq(1, r1a.rank());
            tr.test_eq(1, r1.rank());
            cout << "r1" << endl;
            std::copy(r1.begin(), r1.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            tr.test(std::equal(r1.begin(), r1.end(), rcheck1));
            tr.test(std::equal(r1a.begin(), r1a.end(), rcheck1));

            real rcheck2[2] = { 5 };
            auto r2 = r(1, 1);
            auto r2a = r.at(ra::Small<int, 2> {1, 1});
            tr.test_eq(0, r2a.rank());
            cout << "r2" << endl;
            std::copy(r2.begin(), r2.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            tr.test(std::equal(r2.begin(), r2.end(), rcheck2));
            tr.test(std::equal(r2a.begin(), r2a.end(), rcheck2));
        }
// @TODO Make sure that this is real & = 99, etc. and not Raw<real, 0> = 99, etc.
        section("assign to rank-0 result of subscript");
        {
            real check[6] = {99, 88, 77, 66, 55, 44};
            ra::Unique<real> a({2, 3}, 11.);
            a(0, 0) = 99; a(0, 1) = 88; a(0, 2) = 77;
            a(1, 0) = 66; a(1, 1) = 55; a(1, 2) = 44;
            std::copy(a.begin(), a.end(), std::ostream_iterator<real>(cout, " ")); cout << endl;
            tr.test(std::equal(check, check+6, a.begin()));
        }
    }
    section("construct from shape");
    {
        ra::Unique<real> a(std::vector<ra::dim_t> {3, 2, 4}, ra::unspecified);
        std::iota(a.begin(), a.end(), 0);
        auto sa = ra::ra_traits<ra::Unique<real>>::shape(a);
        tr.test_eq(3, sa[0]);
        tr.test_eq(2, sa[1]);
        tr.test_eq(4, sa[2]);
        real check[24];
        std::iota(check, check+24, 0);
        tr.test(std::equal(check, check+24, a.begin()));
    }
    section("Var rank Raw from fixed rank Raw");
    {
        ra::Unique<real, 3> a({3, 2, 4}, ra::unspecified);
        ra::Raw<real> b(a);
        tr.test(a.data()==b.data()); // pointers are not ra::scalars. Dunno if this deserves fixing.
        tr.test_eq(a.rank(), b.rank());
        tr.test_eq(a.size(0), b.size(0));
        tr.test_eq(a.size(1), b.size(1));
        tr.test_eq(a.size(2), b.size(2));
        tr.test(every(a==b));
    }
    section("I/O");
    {
        section("1");
        {
            ra::Small<real, 3, 2> s { 1, 4, 2, 5, 3, 6 };
            real check[6] = { 1, 4, 2, 5, 3, 6 };
            CheckArrayIO(tr, s, check);
        }
        section("2");
        {
            ra::Small<real, 3> s { 1, 4, 2 };
            real check[3] = { 1, 4, 2 };
            CheckArrayIO(tr, s, check);
        }
        section("3");
        {
            ra::Small<real> s { 77 };
            real check[1] = { 77 };
            CheckArrayIO(tr, s, check);
        }
        section("4. Raw<> can't allocate, so have no istream >>. Check output only.");
        {
            real rpool[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
            ra::Raw<real, 3> r { {ra::Dim {2, 4}, ra::Dim {2, 2}, ra::Dim {2, 1}}, rpool };
            real check[11] = { 2, 2, 2, 1, 2, 3, 4, 5, 6, 7, 8 };
            CheckArrayOutput(tr, r, check);
        }
        section("5");
        {
            real rpool[6] = { 1, 2, 3, 4, 5, 6 };
            ra::Raw<real, 2> r { {ra::Dim {3, 1}, ra::Dim {2, 3}}, rpool };
            real check[8] = { 3, 2, 1, 4, 2, 5, 3, 6 };
            CheckArrayOutput(tr, r, check);
        }
        section("6");
        {
            real rpool[3] = { 1, 2, 3 };
            ra::Raw<real, 1> r { {ra::Dim {3, 1}}, rpool };
            real check[4] = { 3, 1, 2, 3 };
            CheckArrayOutput(tr, r, check);
        }
        section("7");
        {
            real rpool[1] = { 88 };
            ra::Raw<real, 0> r { {}, rpool };
            real check[1] = { 88 };
            CheckArrayOutput(tr, r, check);
            tr.test_eq(1, r.size());
// See note in (170).
            // static_assert(sizeof(r)==sizeof(real *), "bad assumption");
            tr.test_eq(88, r);
        }
        section("8");
        {
            real rpool[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
            ra::Raw<real> a { {ra::Dim {2, 4}, ra::Dim {2, 2}, ra::Dim {2, 1}}, rpool };
            real check[12] = { 3, 2, 2, 2, 1, 2, 3, 4, 5, 6, 7, 8 };
            CheckArrayOutput(tr, a, check);
// default strides.
            ra::Raw<real> b { {2, 2, 2}, rpool };
            CheckArrayOutput(tr, b, check);
        }
        section("9");
        {
            ra::Unique<real, 3> a(std::vector<ra::dim_t> {3, 2, 4}, ra::unspecified);
            std::iota(a.begin(), a.end(), 0);
            real check[3+24] = { 3, 2, 4 };
            std::iota(check+3, check+3+24, 0);
            CheckArrayIO(tr, a, check);
        }
        section("10");
        {
            ra::Unique<real> a(std::vector<ra::dim_t> {3, 2, 4}, ra::unspecified);
            std::iota(a.begin(), a.end(), 0);
            real check[4+24] = { 3, 3, 2, 4 };
            std::iota(check+4, check+4+24, 0);
            CheckArrayIO(tr, a, check);
        }
    }
    section("ply - xpr types - Scalar");
    {
        {
            auto s = ra::scalar(7);
            cout << "s: " << s.s << endl;
        }
        {
            auto s = ra::scalar(ra::Small<int, 2> {11, 12});
            cout << "s: " << s.s << endl;
        }
        {
            ra::Unique<real> a(std::vector<ra::dim_t> {3, 2, 4}, ra::unspecified);
            std::iota(a.begin(), a.end(), 0);
            auto s = ra::scalar(a);
            cout << "s: " << s.s << endl;
        }
    }
    section("ra::iota");
    {
        static_assert(ra::is_array_iterator<decltype(ra::iota(10))>::value, "bad type pred for iota");
        section("straight cases");
        {
            ra::Owned<int, 1> a = ra::iota(4, 1);
            assert(a[0]==1 && a[1]==2 && a[2]==3 && a[3]==4);
        }
        section("work with operators");
        {
            tr.test(every(ra::iota(4)==ra::Owned<int, 1> {0, 1, 2, 3}));
            tr.test(every(ra::iota(4, 1)==ra::Owned<int, 1> {1, 2, 3, 4}));
            tr.test(every(ra::iota(4, 1, 2)==ra::Owned<int, 1> {1, 3, 5, 7}));
        }
 // @TODO actually whether unroll is avoided depends on ply_either, have a way to require it.
// Cf [trc-01] in test-ra-compatibility.C.
        section("[tr0-01] frame-matching, forbidding unroll");
        {
            ra::Owned<int, 3> b ({3, 4, 2}, ra::unspecified);
            transpose(b, {0, 2, 1}) = ra::iota(3, 1);
            cout << b << endl;
            tr.test(every(b(0)==1));
            tr.test(every(b(1)==2));
            tr.test(every(b(2)==3));
        }
    }
    section("reverse array types");
    {
        CheckReverse(tr, ra::Unique<real>({ 3, 2, 4 }, ra::unspecified));
        CheckReverse(tr, ra::Unique<real, 3>({ 3, 2, 4 }, ra::unspecified));
    }
    section("transpose");
    {
        CheckTranspose1(tr, ra::Unique<real>({ 3, 2 }, ra::unspecified));
        CheckTranspose1(tr, ra::Unique<real, 2>({ 3, 2 }, ra::unspecified));
// can do these only with var-rank. @TODO Static-axes version of transpose.
        {
            ra::Unique<real> a({3, 2}, ra::unspecified);
            std::iota(a.begin(), a.end(), 1);
            auto b = transpose(a, ra::Small<int, 2> { 0, 0 });
            cout << "b: " << b << endl;
            tr.test_eq(1, b.rank());
            tr.test_eq(2, b.size());
            tr.test_eq(1, b[0]);
            tr.test_eq(4, b[1]);
        }
        {
            ra::Unique<real> a({2, 3}, ra::unspecified);
            std::iota(a.begin(), a.end(), 1);
            auto b = transpose(a, ra::Small<int, 2> { 0, 0 });
            cout << "b: " << b << endl;
            tr.test_eq(1, b.rank());
            tr.test_eq(2, b.size());
            tr.test_eq(1, b[0]);
            tr.test_eq(5, b[1]);
        }
    }
    return tr.summary();
}
