// -*- mode: c++; coding: utf-8 -*-
/// @file old.cc
/// @brief Deprecated parts of test/ra-0.cc.

// (c) Daniel Llorens - 2013-2015, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

// FIXME Remove these tests as soon as there's an updated equivalent elsewhere.

namespace ra {

constexpr bool
gt_len(dim_t sa, dim_t sb)
{
    return sb==DIM_BAD
             ? 1
             : sa==DIM_BAD
               ? 0
               : sb==DIM_ANY
                 ? 1
                 : (sa!=DIM_ANY && sa>=sb);
}

template <class A, class B>
struct pick_driver
{
    constexpr static int ra = A::rank_s();
    constexpr static int rb = B::rank_s();

    constexpr static bool value_ =
// check by rank
        rb==RANK_BAD
          ? 1
          : rb==RANK_ANY
            ? ra==RANK_ANY
            : ra==RANK_BAD
                ? 0
                : ra==RANK_ANY
                  ? 1
                  : ra>rb
                    ? 1
                    : ra<rb
                      ? 0
// check by size
                      : gt_len(size_s<A>(), size_s<B>());

    constexpr static int value = value_ ? 0 : 1; // 0 if A wins over B, else 1
};

template <class ... P> constexpr int driver_index = mp::indexof<pick_driver, std::tuple<P ...>>;

} // namespace ra

using std::cout, std::endl, std::flush, ra::TestRecorder;
template <int i> using TI = ra::TensorIndex<i>;
template <int i> using UU = decltype(std::declval<ra::Unique<double, i>>().iter());
using SM1 = decltype(std::declval<ra::Small<double, 2>>().iter());
using SM2 = decltype(std::declval<ra::Small<double, 2, 2>>().iter());
using SM3 = decltype(std::declval<ra::Small<double, 2, 2, 2>>().iter());
using SS = decltype(ra::scalar(1));

int main()
{
    TestRecorder tr(std::cout);
// FIXME this section is deprecated, this mechanism isn't used anymore after v10. See test/frame-new.cc.
    tr.section("driver selection");
    {
        static_assert(TI<0>::rank_s()==1, "bad TI rank");
        static_assert(ra::pick_driver<UU<0>, UU<1>>::value==1, "bad driver 1a");
        static_assert(ra::pick_driver<TI<1>, UU<2>>::value==1, "bad driver 1b");

// these two depend on TI<w>::rank_s() being w+1, which I haven't settled on.
        static_assert(TI<1>::rank_s()==2, "bad TI rank");
        static_assert(ra::pick_driver<TI<0>, TI<1>>::value==1, "bad driver 1c");

        static_assert(ra::size_s<UU<0>>()==1, "bad size_s 0");
        static_assert(ra::size_s<SS>()==1, "bad size_s 1");
        static_assert(ra::pick_driver<UU<0>, SS>::value==0, "bad size_s 2");
// static size/rank identical; prefer the first.
        static_assert(ra::driver_index<UU<0>, SS> ==0, "bad match 1a");
        static_assert(ra::driver_index<SS, UU<0>> ==0, "bad match 1b");
// prefer the larger rank.
        static_assert(ra::driver_index<SS, UU<1>> ==1, "bad match 2a");
        static_assert(ra::driver_index<UU<1>, SS> ==0, "bad match 2b");
// never choose TensorIndex as driver.
        static_assert(ra::pick_driver<UU<2>, TI<0>>::value==0, "bad match 3a");
        static_assert(ra::pick_driver<TI<0>, UU<2>>::value==1, "bad match 3b");
// static size/rank identical; prefer the first.
        static_assert(ra::pick_driver<UU<2>, UU<2>>::value==0, "bad match 4");
        static_assert(ra::driver_index<UU<2>, TI<0>, UU<2>> ==0, "bad match 5a");
// dynamic rank counts as +inf.
        static_assert(ra::RANK_ANY==ra::choose_rank(ra::RANK_ANY, 2), "bad match 6a");
        static_assert(UU<ra::RANK_ANY>::rank_s()==ra::choose_rank(UU<ra::RANK_ANY>::rank_s(), UU<2>::rank_s()), "bad match 6b");
        static_assert(UU<ra::RANK_ANY>::rank_s()==ra::choose_rank(UU<2>::rank_s(), UU<ra::RANK_ANY>::rank_s()), "bad match 6c");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, UU<2>>::value==0, "bad match 6d");
        static_assert(ra::pick_driver<UU<2>, UU<ra::RANK_ANY>>::value==1, "bad match 6e");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, UU<ra::RANK_ANY>>::value==0, "bad match 6f");
        static_assert(ra::pick_driver<TI<0>, UU<ra::RANK_ANY>>::value==1, "bad match 6g");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, TI<0>>::value==0, "bad match 6h");
        static_assert(ra::driver_index<TI<0>, UU<ra::RANK_ANY>> ==1, "bad match 6i");
        static_assert(ra::driver_index<UU<ra::RANK_ANY>, TI<0>> ==0, "bad match 6j");
        static_assert(ra::driver_index<UU<2>, UU<ra::RANK_ANY>, TI<0>> ==1, "bad match 6k");
        static_assert(ra::driver_index<UU<1>, UU<2>, UU<3>> ==2, "bad match 6l");
        static_assert(ra::driver_index<UU<2>, TI<0>, UU<ra::RANK_ANY>> ==2, "bad match 6m");
// dynamic vs static size, both static rank
        static_assert(ra::pick_driver<UU<3>, SM3>::value==1, "static rank, dynamic vs static size");
// dynamic rank vs static size & rank
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, SM1 >::value==0, "bad match 7a");
        static_assert(ra::pick_driver<SM1, UU<ra::RANK_ANY>>::value==1, "bad match 7b");
        static_assert(ra::pick_driver<UU<ra::RANK_ANY>, SM2 >::value==0, "bad match 7c");
        static_assert(ra::pick_driver<SM2, UU<ra::RANK_ANY>>::value==1, "bad match 7d");
// more cases with +2 candidates.
        static_assert(ra::driver_index<UU<3>, UU<1>, TI<0>> ==0, "bad match 7b");
        static_assert(ra::driver_index<TI<0>, UU<3>, UU<1>> ==1, "bad match 7c");
        static_assert(ra::driver_index<UU<1>, TI<0>, UU<3>> ==2, "bad match 7d");
        static_assert(ra::driver_index<UU<1>, TI<0>, UU<3>> ==2, "bad match 7e");
    }
    return tr.summary();
}
