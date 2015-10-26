
// (c) Daniel Llorens - 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// @TODO Check that Iota+scalar, etc. is also a Iota. Currently, it's an Expr,
// because the ops below are overriden by the generic ops in ra-operators.H.

#include "ra/ra-operators.H"
#include "ra/test.H"

using std::cout; using std::endl;

namespace ra
{
template <class E, int a=0> decltype(auto) optimize(E && e) { return std::forward<E>(e); }

// @TODO need something to handle the & variants...
#define ITEM(i) std::get<(i)>(e.t)
#define IS_IOTA(I) std::is_same<std::decay_t<I>, Iota<typename I::T> >

// --------------
// plus
// --------------

template <class I, class J, enableif_<mp::And<IS_IOTA(I), ra::is_zero_or_scalar<J> >, int> =0>
auto optimize(Expr<ra::plus, std::tuple<I, J> > && e)
{
    return Iota<decltype(ITEM(0).org_+ITEM(1))> { ITEM(0).size_, ITEM(0).org_+ITEM(1), ITEM(0).stride_ };
}

template <class I, class J, enableif_<mp::And<ra::is_zero_or_scalar<I>, IS_IOTA(J)>, int> =0>
auto optimize(Expr<ra::plus, std::tuple<I, J> > && e)
{
    return Iota<decltype(ITEM(0)+ITEM(1).org_)> { ITEM(1).size_, ITEM(0)+ITEM(1).org_, ITEM(1).stride_ };
}

template <class I, class J, enableif_<mp::And<IS_IOTA(I), IS_IOTA(J)>, int> =0>
auto optimize(Expr<ra::plus, std::tuple<I, J> > && e)
{
    assert(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
    return Iota<decltype(ITEM(0).org_+ITEM(1).org_)> { ITEM(0).size_, ITEM(0).org_+ITEM(1).org_, ITEM(0).stride_+ITEM(1).stride_ };
}

// --------------
// minus
// --------------

template <class I, class J, enableif_<mp::And<IS_IOTA(I), ra::is_zero_or_scalar<J> >, int> =0>
auto optimize(Expr<ra::minus, std::tuple<I, J> > && e)
{
    return Iota<decltype(ITEM(0).org_-ITEM(1))> { ITEM(0).size_, ITEM(0).org_-ITEM(1), ITEM(0).stride_ };
}

template <class I, class J, enableif_<mp::And<ra::is_zero_or_scalar<I>, IS_IOTA(J)>, int> =0>
auto optimize(Expr<ra::minus, std::tuple<I, J> > && e)
{
    return Iota<decltype(ITEM(0)-ITEM(1).org_)> { ITEM(1).size_, ITEM(0)-ITEM(1).org_, -ITEM(1).stride_ };
}

template <class I, class J, enableif_<mp::And<IS_IOTA(I), IS_IOTA(J)>, int> =0>
auto optimize(Expr<ra::minus, std::tuple<I, J> > && e)
{
    assert(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
    return Iota<decltype(ITEM(0).org_-ITEM(1).org_)> { ITEM(0).size_, ITEM(0).org_-ITEM(1).org_, ITEM(0).stride_-ITEM(1).stride_ };
}

// --------------
// times
// --------------

template <class I, class J, enableif_<mp::And<IS_IOTA(I), ra::is_zero_or_scalar<J> >, int> =0>
auto optimize(Expr<ra::times, std::tuple<I, J> > && e)
{
    return Iota<decltype(ITEM(0).org_*ITEM(1))> { ITEM(0).size_, ITEM(0).org_*ITEM(1), ITEM(0).stride_*ITEM(1) };
}

template <class I, class J, enableif_<mp::And<ra::is_zero_or_scalar<I>, IS_IOTA(J)>, int> =0>
auto optimize(Expr<ra::times, std::tuple<I, J> > && e)
{
    return Iota<decltype(ITEM(0)*ITEM(1).org_)> { ITEM(1).size_, ITEM(0)*ITEM(1).org_, ITEM(0)*ITEM(1).stride_ };
}

#undef IS_IOTA
#undef ITEM
}

int main()
{
    TestRecorder tr(std::cout);
    auto i = ra::jvec(5);
    section("optimize is nop by default");
    {
        auto l = optimize(i*i);
        tr.test_equal(ra::vector({0, 1, 4, 9, 16}), l);
    }
    section("operations with Iota, plus");
    {
        auto j = i+1;
        auto k1 = optimize(i+1);
        auto k2 = optimize(1+i);
        auto k3 = optimize(ra::jvec(5)+1);
        auto k4 = optimize(1+ra::jvec(5));
// it's actually a Iota
        tr.test_equal(1, k1.org_);
        tr.test_equal(1, k2.org_);
        tr.test_equal(1, k3.org_);
        tr.test_equal(1, k4.org_);
        tr.test_equal(ra::vector({1, 2, 3, 4, 5}), j);
        tr.test_equal(ra::vector({1, 2, 3, 4, 5}), k1);
        tr.test_equal(ra::vector({1, 2, 3, 4, 5}), k2);
        tr.test_equal(ra::vector({1, 2, 3, 4, 5}), k3);
        tr.test_equal(ra::vector({1, 2, 3, 4, 5}), k4);
    }
    section("operations with Iota, times");
    {
        auto j = i*2;
        auto k1 = optimize(i*2);
        auto k2 = optimize(2*i);
        auto k3 = optimize(ra::jvec(5)*2);
        auto k4 = optimize(2*ra::jvec(5));
// it's actually a Iota
        tr.test_equal(0, k1.org_);
        tr.test_equal(0, k2.org_);
        tr.test_equal(0, k3.org_);
        tr.test_equal(0, k4.org_);
        tr.test_equal(ra::vector({0, 2, 4, 6, 8}), j);
        tr.test_equal(ra::vector({0, 2, 4, 6, 8}), k1);
        tr.test_equal(ra::vector({0, 2, 4, 6, 8}), k2);
        tr.test_equal(ra::vector({0, 2, 4, 6, 8}), k3);
        tr.test_equal(ra::vector({0, 2, 4, 6, 8}), k4);
    }
    return tr.summary();
}
