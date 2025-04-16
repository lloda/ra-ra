// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression traversal.

// (c) Daniel Llorens - 2013-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Make traversal order a parameter, some operations (e.g. output, ravel) require specific orders.
// TODO Better traversal. Tiling, etc. (see eval.cc in Blitz++). Unit step case?
// TODO std::execution::xxx-policy TODO Validate output argument strides.

#pragma once
#include "expr.hh"
#include <print>

namespace ra {

template <class A>
constexpr decltype(auto)
VALUE(A && a)
{
    if constexpr (is_scalar<A>) { return RA_FWD(a); } // [ra8]
    else if constexpr (is_iterator<A>) { return *a; } // no need to start()
    else { return *(ra::start(RA_FWD(a))); }
}

template <class A> using value_t = std::remove_volatile_t<std::remove_reference_t<decltype(VALUE(std::declval<A>()))>>;
template <class A> using ncvalue_t = std::remove_const_t<value_t<A>>;


// ---------------------
// replace Len in expr tree.
// ---------------------

template <>
struct WLen<Len>
{
    constexpr static decltype(auto)
    f(auto ln, auto && e)
    {
        return Scalar<decltype(ln)>(ln);
    }
};

template <class Op, IteratorConcept ... P, int ... I> requires (has_len<P> || ...)
struct WLen<Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>>
{
    constexpr static decltype(auto)
    f(auto ln, auto && e)
    {
        return expr(RA_FWD(e).op, wlen(ln, std::get<I>(RA_FWD(e).t)) ...);
    }
};

template <IteratorConcept ... P, int ... I> requires (has_len<P> || ...)
struct WLen<Pick<std::tuple<P ...>, mp::int_list<I ...>>>
{
    constexpr static decltype(auto)
    f(auto ln, auto && e)
    {
        return pick(wlen(ln, std::get<I>(RA_FWD(e).t)) ...);
    }
};

// final iota/ptr types must be either is_constant or is_scalar.

template <int w, class I, class N, class S> requires (has_len<I> || has_len<N> || has_len<S>)
struct WLen<Iota<w, I, N, S>>
{
    constexpr static decltype(auto)
    f(auto ln, auto && e)
    {
        return iota<w>(VALUE(wlen(ln, RA_FWD(e).n)), VALUE(wlen(ln, RA_FWD(e).i)), VALUE(wlen(ln, RA_FWD(e).s)));
    }
};

template <class I, class N, class S> requires (has_len<N> || has_len<S>)
struct WLen<Ptr<I, N, S>>
{
    constexpr static decltype(auto)
    f(auto ln, auto && e)
    {
        return ptr(RA_FWD(e).i, VALUE(wlen(ln, RA_FWD(e).n)), VALUE(wlen(ln, RA_FWD(e).s)));
    }
};


// --------------
// ply, run time order/rank.
// --------------

struct Nop {};

// step() must give 0 for k>=their own rank, to allow frame matching.
template <IteratorConcept A, class Early = Nop>
constexpr auto
ply_ravel(A && a, Early && early = Nop {})
{
    rank_t rank = ra::rank(a);
// must avoid 0-length vlas [ra40].
    if (0>=rank) {
        if (0>rank) [[unlikely]] { std::abort(); }
        if constexpr (requires {early.def;}) {
            return (*a).value_or(early.def);
        } else {
            *a;
            return;
        }
    }
// inside first. FIXME better heuristic - but first need a way to force row-major
    rank_t order[rank];
    for (rank_t i=0; i<rank; ++i) {
        order[i] = rank-1-i;
    }
    dim_t sha[rank], ind[rank] = {};
// find outermost compact dim.
    rank_t * ocd = order;
    dim_t ss = a.len(*ocd);
#pragma GCC diagnostic push // gcc 12.2 and 13.2 with RA_DO_CHECK=0 and -fno-sanitize=all
#pragma GCC diagnostic warning "-Warray-bounds"
    for (--rank, ++ocd; rank>0 && a.keep(ss, order[0], *ocd); --rank, ++ocd) {
        ss *= a.len(*ocd);
    }
    for (int k=0; k<rank; ++k) {
// ss takes care of the raveled dimensions ss.
        if (0>=(sha[k]=a.len(ocd[k]))) {
            if (0>sha[k]) [[unlikely]] { std::abort(); }
            if constexpr (requires {early.def;}) {
                return early.def;
            } else {
                return;
            }
        }
    }
    auto ss0 = a.step(order[0]);
    for (;;) {
        auto place = a.save();
        for (dim_t s=ss; --s>=0; a.mov(ss0)) {
            if constexpr (requires {early.def;}) {
                if (auto stop = *a) {
                    return stop.value();
                }
            } else {
                *a;
            }
        }
        a.load(place); // FIXME wasted if k=0. Cf test/iota.cc
        for (int k=0; ; ++k) {
            if (k>=rank) {
                if constexpr (requires {early.def;}) {
                    return early.def;
                } else {
                    return;
                }
            } else if (++ind[k]<sha[k]) {
                a.adv(ocd[k], 1);
                break;
            } else {
                ind[k] = 0;
                a.adv(ocd[k], 1-sha[k]);
            }
        }
    }
#pragma GCC diagnostic pop
}


// -------------------------
// ply, compile time order/rank.
// -------------------------

template <auto order, int k, int urank, class A, class S, class Early>
constexpr auto
subply(A & a, dim_t s, S const & ss0, Early & early)
{
    if constexpr (k < urank) {
        auto place = a.save();
        for (; --s>=0; a.mov(ss0)) {
            if constexpr (requires {early.def;}) {
                if (auto stop = *a) {
                    return stop;
                }
            } else {
                *a;
            }
        }
        a.load(place); // FIXME wasted if k was 0 at the top
    } else {
        dim_t size = a.len(order[k]); // TODO precompute above
        for (dim_t i=0; i<size; ++i) {
            if constexpr (requires {early.def;}) {
                if (auto stop = subply<order, k-1, urank>(a, s, ss0, early)) {
                    return stop;
                }
            } else {
                subply<order, k-1, urank>(a, s, ss0, early);
            }
            a.adv(order[k], 1);
        }
        a.adv(order[k], -size);
    }
    if constexpr (requires {early.def;}) {
        return static_cast<decltype(*a)>(std::nullopt);
    } else {
        return;
    }
}

template <IteratorConcept A, class Early = Nop>
constexpr decltype(auto)
ply_fixed(A && a, Early && early = Nop {})
{
    constexpr rank_t rank = rank_s(a);
    static_assert(0<=rank, "ply_fixed needs static rank");
// inside first. FIXME better heuristic - but first need a way to force row-major
    constexpr auto order = mp::tuple2array<int, mp::reverse<mp::iota<rank>>>();
    if constexpr (0==rank) {
        if constexpr (requires {early.def;}) {
            return (*a).value_or(early.def);
        } else {
            *a;
            return;
        }
    } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Warray-bounds"
    auto ss0 = a.step(order[0]); // gcc 14.1 with RA_DO_CHECK=0 and sanitizer on
#pragma GCC diagnostic pop
        if constexpr (requires {early.def;}) {
            return (subply<order, rank-1, 1>(a, a.len(order[0]), ss0, early)).value_or(early.def);
        } else {
            subply<order, rank-1, 1>(a, a.len(order[0]), ss0, early);
        }
    }
}


// ---------------------------
// default ply
// ---------------------------

template <IteratorConcept A, class Early = Nop>
constexpr decltype(auto)
ply(A && a, Early && early = Nop {})
{
    static_assert(0<=rank_s(a) || ANY==rank_s(a));
    if constexpr (ANY==size_s(a)) {
        return ply_ravel(RA_FWD(a), RA_FWD(early));
    } else {
        return ply_fixed(RA_FWD(a), RA_FWD(early));
    }
}

constexpr void
for_each(auto && op, auto && ... a) { ply(map(RA_FWD(op), RA_FWD(a) ...)); }

template <class T> struct Default { T def; };
template <class T> Default(T &&) -> Default<T>;

constexpr decltype(auto)
early(IteratorConcept auto && a, auto && def) { return ply(RA_FWD(a), Default { RA_FWD(def) }); }


// --------------------
// input/'output' iterator adapter. FIXME maybe random for rank 1?
// --------------------

template <IteratorConcept A>
struct STLIterator
{
    using difference_type = dim_t;
    using value_type = value_t<A>;

    A a;
    std::decay_t<decltype(ra::shape(a))> ind; // concrete type
    bool over;

    STLIterator(A a_): a(a_), ind(ra::shape(a_)), over(0==ra::size(a)) {}
    constexpr STLIterator(STLIterator &&) = default;
    constexpr STLIterator(STLIterator const &) = delete;
    constexpr STLIterator & operator=(STLIterator &&) = default;
    constexpr STLIterator & operator=(STLIterator const &) = delete;
    constexpr bool operator==(std::default_sentinel_t end) const { return over; }
    decltype(auto) operator*() const { return *a; }

    constexpr void
    next(rank_t k)
    {
        for (; k>=0; --k) {
            if (--ind[k]>0) {
                a.adv(k, 1);
                return;
            } else {
                ind[k] = a.len(k);
                a.adv(k, 1-a.len(k));
            }
        }
        over = true;
    }
    template <int k>
    constexpr void
    next()
    {
        if constexpr (k>=0) {
            if (--ind[k]>0) {
                a.adv(k, 1);
            } else {
                ind[k] = a.len(k);
                a.adv(k, 1-a.len(k));
                next<k-1>();
            }
            return;
        }
        over = true;
    }
    constexpr STLIterator & operator++() { if constexpr (ANY==rank_s(a)) { next(rank(a)-1); } else { next<rank_s(a)-1>(); } return *this; }
    constexpr void operator++(int) { ++(*this); } // see p0541 and p2550. Or just avoid.
};

template <class A> STLIterator(A &&) -> STLIterator<A>;

constexpr auto begin(is_ra auto && a) { return STLIterator(ra::start(RA_FWD(a))); }
constexpr auto end(is_ra auto && a) { return std::default_sentinel; }
constexpr auto range(is_ra auto && a) { return std::ranges::subrange(ra::begin(RA_FWD(a)), std::default_sentinel); }

// unqualified might find .begin() anyway through std::begin etc (!)
constexpr auto begin(is_ra auto && a) requires (requires { a.begin(); }) { static_assert(std::is_lvalue_reference_v<decltype(a)>); return a.begin(); }
constexpr auto end(is_ra auto && a) requires (requires { a.end(); }) { static_assert(std::is_lvalue_reference_v<decltype(a)>); return a.end(); }
constexpr auto range(is_ra auto && a) requires (requires { a.begin(); }) { static_assert(std::is_lvalue_reference_v<decltype(a)>); return std::ranges::subrange(a.begin(), a.end()); }

// fmt/ostream.h or https://stackoverflow.com/a/75738462
struct ostream_formatter: std::formatter<std::basic_string_view<char>, char>
{
    template <class T, class O>
    constexpr O
    format(T const & value, std::basic_format_context<O, char> & ctx) const
    {
        std::basic_stringstream<char> ss;
        ss << value;
        return std::formatter<std::basic_string_view<char>, char>::format(ss.view(), ctx);
    }
};

} // namespace ra


// ---------------------------
// i/o
// --------------------------

template <class A> requires (ra::is_ra<A> || (ra::is_fov<A> && !std::is_convertible_v<A, std::string_view>))
struct std::formatter<A>
{
    ra::format_t fmt;
    std::conditional_t<std::formattable<ra::value_t<A>, char>,
                       std::formatter<ra::value_t<A>>, ra::ostream_formatter> under;

    constexpr auto
    parse(auto & ctx)
    {
        auto i = ctx.begin();
        for (; i!=ctx.end() && ':'!=*i && '}'!=*i; ++i) {
            switch (*i) {
            case 'j': fmt = ra::jstyle; break;
            case 'c': fmt = ra::cstyle; break;
            case 'l': fmt = ra::lstyle; break;
            case 'p': fmt = ra::pstyle; break;
            case 'a': fmt.align = true; break;
            case 'e': fmt.align = false; break;
            case 's': fmt.shape = ra::withshape; break;
            case 'n': fmt.shape = ra::noshape; break;
            case 'd': fmt.shape = ra::defaultshape; break;
            default: throw std::format_error("Bad format for ra:: object.");
            }
        }
        if (i!=ctx.end() && ':'==*i) {
            ctx.advance_to(i+1);
            i = under.parse(ctx);
        }
        if (i!=ctx.end() && '}'!=*i) {
            throw std::format_error("Bad input while parsing format for ra:: object.");
        }
        return i;
    }
    constexpr auto
    format(A const & a_, auto & ctx, ra::format_t const & fmt) const
    {
        static_assert(!ra::has_len<A>, "len outside subscript context.");
        static_assert(ra::BAD!=ra::size_s<A>(), "Cannot print undefined size expr.");
        auto a = ra::start(a_); // [ra35]
        auto sha = ra::shape(a);
        assert(every(ra::start(sha)>=0));
        auto out = ctx.out();
// always print shape with defaultshape to avoid recursion on shape(shape(...)) = [1].
        if (fmt.shape==ra::withshape || (fmt.shape==ra::defaultshape && size_s(a)==ra::ANY)) {
            out = std::format_to(out, "{:d}\n", ra::start(sha));
        }
        ra::rank_t const rank = ra::rank(a);
        auto goin = [&](int k, auto & goin) -> void
        {
            using std::ranges::copy;
            if (k==rank) {
                ctx.advance_to(under.format(*a, ctx));
                out = ctx.out();
            } else {
                out = copy(fmt.open, out).out;
                for (int i=0; i<sha[k]; ++i) {
                    goin(k+1, goin);
                    if (i+1<sha[k]) {
                        a.adv(k, 1);
                        out = copy((k==rank-1 ? fmt.sep0 : fmt.sepn), out).out;
                        for (int i=0; i<std::max(0, rank-2-k); ++i) {
                            out = copy(fmt.rep, out).out;
                        }
                        if (fmt.align && k<rank-1) {
                            for (int i=0; i<(k+1)*ra::size(fmt.open); ++i) {
                                *out++ = ' ';
                            }
                        }
                    } else {
                        a.adv(k, 1-sha[k]);
                        break;
                    }
                }
                out = copy(fmt.close, out).out;
            }
        };
        goin(0, goin);
        return out;
    }
    constexpr auto format(A const & a_, auto & ctx) const { return format(a_, ctx, fmt); }
};

template <class A>
struct std::formatter<ra::FormatArray<A>>
{
    std::formatter<std::decay_t<A>> fmt;
    constexpr auto
    parse(auto & ctx)
    {
        auto i = ctx.begin();
        if (i!=ctx.end() && '}'!=*i) {
            throw std::format_error("Bad input while parsing format for ra:: format object.");
        }
        return i;
    }
    constexpr auto
    format(ra::FormatArray<A> const & f_, auto & ctx) const
    {
        return fmt.format(f_.a, ctx, f_.fmt);
    }
};

namespace ra {

template <class A>
constexpr std::ostream &
operator<<(std::ostream & o, FormatArray<A> const & fa)
{
    std::print(o, "{}", fa);
    return o;
}

template <class C> requires (ANY!=size_s<C>() && !is_scalar<C>)
inline std::istream &
operator>>(std::istream & i, C & c)
{
    for (auto & ci: c) { i >> ci; }
    return i;
}

template <class T, class A>
inline std::istream &
operator>>(std::istream & i, std::vector<T, A> & c)
{
    if (dim_t n; i >> n) {
        RA_CHECK(n>=0, "Negative length in input [", n, "].");
        std::vector<T, A> cc(n);
        swap(c, cc);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

template <class C> requires (ANY==size_s<C>() && !std::is_convertible_v<C, std::string_view>)
inline std::istream &
operator>>(std::istream & i, C & c)
{
    if (decltype(shape(c)) s; i >> s) {
        RA_CHECK(every(start(s)>=0), "Negative length in input [", noshape, s, "].");
        C cc(s, ra::none);
        swap(c, cc);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

} // namespace ra
