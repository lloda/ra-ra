// -*- mode: c++; coding: utf-8 -*-
/// @file reshape.cc
/// @brief Tests for reshape().

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <memory>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, ra::TestRecorder;

namespace ra {

std::ostream & operator<<(std::ostream & o, ra::Dim const d)
{
    o << "{" << d.size << ", " << d.stride << "}";
    return o;
}

} // namespace ra

int main()
{
    TestRecorder tr(std::cout);
    tr.section("reshape");
    {
        ra::Big<int, 3> aa({2, 3, 3}, ra::_0*3+ra::_1);
        auto a = aa(ra::all, ra::all, 0);
        tr.info("ravel_free").test_eq(ra::iota(6), ravel_free(a));
        tr.test_eq(ra::scalar(a.p), ra::scalar(ravel_free(a).p));
// select.
        tr.info("reshape select").test_eq(ra::Big<int, 1> {0, 1, 2}, reshape(a, ra::Small<int, 1> {3}));
        tr.test_eq(ra::scalar(a.p), ra::scalar(reshape(a, ra::Small<int, 1> {3}).p));
// tile.
        auto tilea = reshape(a, ra::Small<int, 3> {2, 2, 3});
        tr.info("reshape select").test_eq(ra::Big<int, 3>({2, 2, 3}, {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5}), tilea);
        tr.info("some tile-reshapes are free (I)").test_eq(0, tilea.stride(0));
        tr.info("some tile-reshapes are free (II)").test_eq(ra::scalar(a.data()), ra::scalar(tilea.data()));
// reshape with free ravel
        tr.info("reshape w/free ravel I").test_eq(ra::Big<int, 2>({3, 2}, {0, 1, 2, 3, 4, 5}), reshape(a, ra::Small<int, 2> {3, 2}));
        tr.test_eq(ra::scalar(a.p), ra::scalar(reshape(a, ra::Small<int, 2> {3, 2}).p));
        tr.info("reshape w/free ravel II").test_eq(ra::Big<int, 3>({2, 1, 2}, {0, 1, 2, 3}), reshape(a, ra::Small<int, 3> {2, 1, 2}));
        tr.test_eq(ra::scalar(a.p), ra::scalar(reshape(a, ra::Small<int, 3> {2, 1, 2}).p));
        tr.info("reshape w/free ravel III").test_eq(ra::Big<int, 2>({3, 2}, {0, 1, 2, 3, 4, 5}), reshape(a, ra::Small<int, 2> {-1, 2}));
        tr.test_eq(ra::scalar(a.p), ra::scalar(reshape(a, ra::Small<int, 2> {-1, 2}).p));
        tr.info("reshape w/free ravel IV").test_eq(ra::Big<int, 2>({2, 3}, {0, 1, 2, 3, 4, 5}), reshape(a, ra::Small<int, 2> {2, -1}));
        tr.test_eq(ra::scalar(a.p), ra::scalar(reshape(a, ra::Small<int, 2> {2, -1}).p));
        tr.info("reshape w/free ravel V").test_eq(ra::Big<int, 3>({2, 1, 3}, {0, 1, 2, 3, 4, 5}), reshape(a, ra::Small<int, 3> {2, -1, 3}));
        tr.test_eq(ra::scalar(a.p), ra::scalar(reshape(a, ra::Small<int, 3> {2, -1, 3}).p));
    }
    tr.section("reshape from var rank to fixed rank");
    {
        ra::Big<int> a({2, 3}, ra::_0*3+ra::_1);
        auto b = reshape(a, ra::Small<int, 1> {3});
        tr.info("reshape select").test_eq(ra::Big<int, 1> {0, 1, 2}, b);
        tr.test_eq(ra::scalar(a.p), ra::scalar(b.p));
        tr.info("reshape can fix rank").test_eq(1, rank_s(b));
    }
    tr.section("reshape from var rank to fixed rank using the initializer_list shim");
    {
        ra::Big<int> a({2, 3}, ra::_0*3+ra::_1);
        auto b = reshape(a, {3, 2});
        tr.info("reshape").test_eq(ra::Big<int, 2> {{0, 1}, {2, 3}, {4, 5}}, b);
        tr.test_eq(ra::scalar(a.p), ra::scalar(b.p));
        tr.info("reshape can return fixed rank (2)").test_eq(2, rank_s(b));
        auto c = reshape(a, {3l, 2l}); // check deduction works regardless
        tr.info("reshape").test_eq(ra::Big<int, 2> {{0, 1}, {2, 3}, {4, 5}}, c);
        tr.test_eq(ra::scalar(a.p), ra::scalar(c.p));
        tr.info("reshape can return fixed rank (3)").test_eq(2, rank_s(c));
    }
    tr.section("reshape from var rank to var rank");
    {
        ra::Big<int> a({2, 3}, ra::_0*3+ra::_1);
        auto b = reshape(a, ra::Big<int, 1> {3});
        tr.info("reshape select").test_eq(ra::Big<int, 1> {0, 1, 2}, b);
        tr.test_eq(ra::scalar(a.p), ra::scalar(b.p));
        tr.info("reshape can return var rank (1)").test_eq(ra::RANK_ANY, rank_s(b));
    }
    tr.section("reshape to fixed rank to var rank");
    {
// FIXME warning w/ gcc 6.3 in bootstrap.hh inside() [ra32]. Apparent root is in decl of b in reshape().
        ra::Big<int, 2> a({2, 3}, ra::_0*3+ra::_1);
        auto b = reshape(a, ra::Big<int, 1> {3});
        tr.info("reshape select").test_eq(ra::Big<int, 1> {0, 1, 2}, b);
        tr.test_eq(ra::scalar(a.p), ra::scalar(b.p));
        tr.info("reshape can return var rank").test_eq(ra::RANK_ANY, rank_s(b));
    }
    tr.section("conversion from var rank to fixed rank");
    {
        ra::Big<int> a({2, 3}, ra::_0*3+ra::_1);
        ra::View<int, 2> b = a;
        tr.info("fixing rank").test_eq(ra::_0*3+ra::_1, b);
        tr.info("fixing rank is view").test(a.data()==b.data());
    }
    return tr.summary();
}
