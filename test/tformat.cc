// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Sandbox for std::format. Cf io.cc.

// (c) Daniel Llorens - 2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// The binary cannot be called format bc gcc14 will try to include it (!!)

#include "ra/test.hh"

using ra::TestRecorder;

using int3 = ra::Small<int, 3>;
using int2 = ra::Small<int, 2>;
using double2 = ra::Small<double, 2>;

struct Q
{
    int x = 1;
};

template <>
struct std::formatter<Q>
{
    constexpr auto
    parse(std::format_parse_context const & ctx)
    {
        auto i = ctx.begin();
        assert(i == ctx.end() || *i == '}');
        return i;
    }
    template <class Ctx>
    auto
    format(Q const & q, Ctx & ctx) const
    {
        return std::format_to(ctx.out(), "Q{}", q.x);
    }
};

int main()
{
    std::print(stdout, "a number {:015.9}\n", std::numbers::pi_v<double>);
    std::print(stdout, "a Q {}\n", Q { 3 });
    std::print(stdout, "a complex {}\n", std::complex {1, -1});
    std::print(stdout, "a small {:}\n", ra::Small<ra::dim_t, 1> {1});
    std::print(stdout, "a small {:}\n", ra::Small<int, 3> {1, 2, 3});
    std::print(stdout, "a small {::}\n", ra::Small<int, 3> {1, 2, 3});
    std::print(stdout, "a small {::9}\n", ra::Small<int, 3> {1, 2, 3});
    std::print(stdout, "a small {::<9}\n", ra::Small<int, 3> {1, 2, 3});
    std::print(stdout, "a big\n{::7}\n", ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1));
    std::print(stdout, "a big\n{:s:7}\n", ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1));
    std::print(stdout, "a big\n{:p:5}\n", ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1));
    std::print(stdout, "a big\n{:pe:5}\n", ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1));
    std::print(stdout, "a big\n{::5}\n", ra::Big<int>({3, 4}, ra::_0 + ra::_1));
    std::print(stdout, "a big(small)\n{:cs:p:06.3f}\n", ra::Big<double2>({3, 4}, ra::_0 + ra::_1));
    std::print(stdout, "a big\n{}\n", ra::format_array(ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1), ra::cstyle));
    std::print(stdout, "a big\n{}\n", ra::format(ra::cstyle, ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1)));
    return 0;
}
