// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Sandbox for std::format. Cf io.cc.

// (c) Daniel Llorens - 2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// The binary cannot be called format bc gcc14 will try to include it (!!)

#include <print>
#include <cstdio>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

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

// FIXME a way to use the formatter without parsing, like ra::format_t { .option = value ... }.
// FIXME an ellipsis feature, e.g. max width/max length.
// FIXME formatting options for the shape (needed?)
template <class A> requires (ra::is_ra<A> || (ra::is_fov<A> && !std::is_convertible_v<A, std::string_view>))
struct std::formatter<A>
{
    ra::shape_t shape = ra::defaultshape;
    std::string open = "", close = "", sep0 = " ", sepn = "\n", rep = "\n";
    std::formatter<ra::value_t<A>> under;
    bool align = false;

// https://stackoverflow.com/a/75549049
    template <class Ctx>
    constexpr auto
    parse(Ctx & ctx)
    {
        auto i = ctx.begin();
        for (; i!=ctx.end() && ':'!=*i && '}'!=*i; ++i) {
            switch (*i) {
            case 'j': break;
            case 'c': shape=ra::noshape; open="{"; close="}"; sep0=", "; sepn=",\n"; rep=""; align=true; break;
            case 'l': shape=ra::noshape; open="("; close=")"; sep0=" "; sepn="\n"; rep=""; align=true; break;
            case 'p': shape=ra::noshape; open="["; close="]"; sep0=", "; sepn=",\n"; rep="\n"; align=true; break;
            case 'a': align=true; break;
            case 'e': align=false; break;
            case 's': shape=ra::withshape; break;
            case 'n': shape=ra::noshape; break;
            case 'd': shape=ra::defaultshape; break;
            default: throw std::format_error("Bad format for ra:: object.");
            }
        }
        if (i!=ctx.end() && ':'==*i) {
            ctx.advance_to(i+1);
            i = under.parse(ctx);
        }
        if (i!=ctx.end() && '}'!=*i) {
            throw std::format_error("Unexpected input while parsing format for ra:: object.");
        }
        return i;
    }
    template <class Ctx>
    constexpr auto
    format(A const & a_, Ctx & ctx) const
    {
        static_assert(!ra::has_len<A>, "len outside subscript context.");
        static_assert(ra::BAD!=ra::size_s<A>(), "Cannot print undefined size expr.");
        auto a = ra::start(a_); // [ra35]
        auto sha = ra::shape(a);
        assert(every(ra::start(sha)>=0));
        auto out = ctx.out();
// always print shape with defaultshape to avoid recursion on shape(shape(...)) = [1].
        if (shape==ra::withshape || (shape==ra::defaultshape && size_s(a)==ra::ANY)) {
            out = std::format_to(out, "{:d}\n", ra::start(sha));
        }
        ra::rank_t const rank = ra::rank(a);
        auto goin = [&](int k, auto & goin) -> void
        {
            if (k==rank) {
                ctx.advance_to(under.format(*a, ctx));
                out = ctx.out();
            } else {
                out = std::ranges::copy(open, out).out;
                for (int i=0; i<sha[k]; ++i) {
                    goin(k+1, goin);
                    if (i+1<sha[k]) {
                        a.adv(k, 1);
                        out = std::ranges::copy((k==rank-1 ? sep0 : sepn), out).out;
                        for (int i=0; i<std::max(0, rank-2-k); ++i) {
                            out = std::ranges::copy(rep, out).out;
                        }
                        if (align && k<rank-1) {
                            for (int i=0; i<(k+1)*ra::size(open); ++i) {
                                *out++ = ' ';
                            }
                        }
                    } else {
                        a.adv(k, 1-sha[k]);
                        break;
                    }
                }
                out = std::ranges::copy(close, out).out;
            }
        };
        goin(0, goin);
        return out;
    }
};

int main()
{
    std::print(stdout, "a number {:015.9}\n", std::numbers::pi_v<double>);
    std::print(stdout, "a Q {}\n", Q { 3 });
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
    return 0;
}
