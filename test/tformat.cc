// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Sandbox for std::format. Cf io.cc.

// (c) Daniel Llorens - 2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// The binary cannot be called format bc gcc14 will try to include it (!!)

#include "ra/test.hh"
#include <print>

using ra::TestRecorder;

using int3 = ra::Small<int, 3>;
using int2 = ra::Small<int, 2>;
using double2 = ra::Small<double, 2>;

struct Yes
{
    int x = 1;
};

template <>
struct std::formatter<Yes>
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
    format(Yes const & q, Ctx & ctx) const
    {
        return std::format_to(ctx.out(), "Yes{}", q.x);
    }
};

struct No
{
    int y = 1;
};

std::ostream & operator<<(std::ostream & o, No const & no) { return o << "No" << no.y; }

int main()
{
    TestRecorder tr(std::cout);
    static_assert(std::formattable<Yes, char>);
    static_assert(!std::formattable<No, char>);
    std::print(stdout, "a number {:015.9}\n", std::numbers::pi_v<double>);
    std::print(stdout, "a Yes {}\n", Yes { 3 });

    std::print(stdout, "a small of Yes {:}\n", ra::Small<Yes, 2> {Yes{1}, Yes{2}});
    std::print(stdout, "a small of No {:}\n", ra::Small<No, 2> {No{1}, No{2}});
    std::print(stdout, "a small of complex {:}\n", ra::Small<std::complex<double>, 2> {{1, -1}, {-1, 1}});
    if constexpr (std::formattable<std::complex<double>, char>) {
        std::print(stdout, "a complex {}\n", std::complex {1, -1});
    } else {
        std::print(stdout, "complex aren't currently formattable!\n");
    }
    tr.test_seq("Yes1 Yes2", std::format("{}", ra::Small<Yes, 2> {Yes{1}, Yes{2}}));
    tr.test_seq("No1 No2", std::format("{}", ra::Small<No, 2> {No{1}, No{2}}));
    tr.test_seq("(1,-1) (-1,1)", std::format("{}", ra::Small<std::complex<double>, 2> {{1, -1}, {-1, 1}}));

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
    std::print(stdout, "a big\n{}\n", ra::fmt(ra::cstyle, ra::Big<int, 2>({3, 4}, ra::_0 + ra::_1)));
    return tr.summary();
}
