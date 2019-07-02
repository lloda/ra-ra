// -*- mode: c++; coding: utf-8 -*-
/// @file axpr.C
/// @brief Driverless Expr - WIP

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/format.H"


// -------------------------------------
// bit from example/throw.C which FIXME should be easier. Maybe an option in ra/macros.H.

struct ra_error: public std::exception
{
    std::string s;
    template <class ... A> ra_error(A && ... a): s(ra::format(std::forward<A>(a) ...)) {}
    virtual char const * what() const throw ()
    {
        return s.c_str();
    }
};

#ifdef RA_ASSERT
#error RA_ASSERT is already defined!
#endif
#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "]", __VA_ARGS__); }
// -------------------------------------

#include "ra/test.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout, std::endl, std::flush;


// -------------------
// newaxis using DIM_BAD

namespace ra {

template <int n> struct newaxos_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};

template <int n=1> constexpr newaxos_t<n> newaxos = newaxos_t<n>();

template <int n> struct is_beatable_def<newaxos_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = 0;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <int n, class ... I>
inline dim_t select_loop(Dim * dim, Dim const * dim_src, newaxos_t<n> newaxos, I && ... i)
{
    for (Dim * end = dim+n; dim!=end; ++dim) {
        dim->size = DIM_BAD;
        dim->stride = 0;
    }
    return select_loop(dim, dim_src, std::forward<I>(i) ...);
}

} // namespace ra

// -------------------


// -------------------
// library bits...

namespace mp {

template <class T> struct box { using type = T; };

template <class K, class T, class F, class I = int_t<0>>
constexpr auto
fold_tuple(K && k, T && t, F && f, I && i = int_t<0> {})
{
    if constexpr (I::value==std::tuple_size_v<std::decay_t<T>>) {
        return k;
    } else {
        return fold_tuple(f(k, std::get<i>(t)), t, f, int_t<I::value+1> {});
    }
}

} // namespace mp

namespace ra {

constexpr dim_t
chosen_size(dim_t sa, dim_t sb)
{
    if (sa==DIM_BAD) {
        return sb;
    } else if (sb==DIM_BAD) {
        return sa;
    } else if (sa==DIM_ANY) {
        return sb;
    } else if (sb==DIM_ANY) {
        return sa;
    } else {
        return sa;
    }
}
// -------------------


// -------------------
// like Expr but there is no driver; we select the size at each dimension.
// This is necessary for newaxis(DIM_BAD) to work, e.g. a(newaxis, :) + a(:, newaxis). What else?

// E is Axpr and ev. Axpr/Pick
// TODO separate the static and the dynamic checks so the static check is done regardless of CHECK_BOUNDS.
template <class E>
constexpr void check_axpr(E const & e)
{
    using T = typename E::T;
    constexpr rank_t rs = E::rank_s();
    if constexpr (rs>=0) {
        constexpr auto fk =
            [](auto && fk, auto k_)
            {
                constexpr int k = decltype(k_)::value;
                if constexpr (k<rs) {
                    constexpr auto fi =
                        [](auto && fi, auto i_, auto sk_)
                        {
                            constexpr dim_t sk = decltype(sk_)::value;
                            constexpr int i = decltype(i_)::value;
                            if constexpr (i<mp::len<T>) {
                                using Ti = std::decay_t<mp::ref<T, i>>;
                                if constexpr (k<Ti::rank_s()) {
                                    constexpr dim_t si = Ti::size_s(k);
                                    static_assert(sk<0 || si<0 || si==sk, "mismatched static dimensions");
                                    fi(fi, mp::int_t<i+1> {}, mp::int_t<chosen_size(sk, si)> {});
                                } else {
                                    fi(fi, mp::int_t<i+1> {}, mp::int_t<sk> {});
                                }
                            }
                        };
                    fi(fi, mp::int_t<0> {}, mp::int_t<DIM_BAD> {});
                    fk(fk, mp::int_t<k+1> {});
                }
            };
        fk(fk, mp::int_t<0> {});
    }
    if constexpr (E::size_s()==DIM_ANY) {
        rank_t rs = e.rank();
        for (int k=0; k!=rs; ++k) {
            auto fi =
                [&k, &e](auto && fi, auto i_, int sk)
                {
                    constexpr int i = decltype(i_)::value;
                    if constexpr (i<mp::len<T>) {
                        if (k<std::get<i>(e.t).rank()) {
                            dim_t si = std::get<i>(e.t).size(k);
                            RA_ASSERT((sk==DIM_BAD || si==DIM_BAD || si==sk),
                                      " k ", k, " sk ", sk, " != ", si, ": mismatched dimensions");
                            fi(fi, mp::int_t<i+1> {}, chosen_size(sk, si));
                        } else {
                            fi(fi, mp::int_t<i+1> {}, sk);
                        }
                    }
                };
            fi(fi, mp::int_t<0> {}, DIM_BAD);
        }
    }
}

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Axpr;

// forward decl in atom.H
// TODO 'dynamic' version where the driver is selected at run time (e.g. if args are RANK_ANY).
template <class Op, class ... P, int ... I>
struct Axpr<Op, std::tuple<P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P ...>;

    Op op;
    T t;

// rank of largest subexpr. This is true for either prefix or suffix match.
    constexpr static rank_t rank_s()
    {
        return mp::fold_tuple(RANK_BAD, mp::map<mp::box, T> {},
                              [](rank_t r, auto a)
                              {
                                  constexpr rank_t ar = decltype(a)::type::rank_s();
                                  return gt_rank(r, ar) ?  r : ar;
                              });
    }
    constexpr rank_t rank() const
    {
        if constexpr (constexpr rank_t rs=rank_s(); rs==RANK_ANY) {
            return mp::fold_tuple(RANK_BAD, t,
                                  [](rank_t r, auto && a)
                                  {
                                      rank_t ar = a.rank();
                                      assert(ar!=RANK_ANY); // cannot happen at runtime
                                      return gt_rank(r, ar) ?  r : ar;
                                  });
        } else {
            return rs;
        }
    }

// any size which is not DIM_BAD. FIXME early exit.
    constexpr static dim_t size_s(int k)
    {
        dim_t s = mp::fold_tuple(DIM_BAD, mp::map<mp::box, T> {},
                                 [&k](dim_t s, auto a)
                                 {
                                     if (decltype(a)::type::rank_s()>=0 && k>=decltype(a)::type::rank_s()) {
                                         return s;
                                     } else if (s!=DIM_BAD) {
                                         return s;
                                     } else {
                                         return decltype(a)::type::size_s(k);
                                     }
                                 });
        assert(s!=DIM_BAD);
        return s;
    }

    constexpr dim_t size(int k) const
    {
        if (dim_t ss=size_s(k); ss==DIM_ANY) {
            dim_t s = mp::fold_tuple(DIM_BAD, t,
                                  [&k](dim_t s, auto && a)
                                  {
                                      if (s!=DIM_BAD || k>=a.rank()) {
                                          return s;
                                      } else {
                                          dim_t as = a.size(k);
                                          assert(as!=DIM_ANY); // cannot happen at runtime
                                          return as;
                                      }
                                  });
            assert(s!=DIM_BAD);
            return s;
        } else {
            return ss;
        }
    }

    constexpr decltype(auto) shape() const
    {
        if constexpr (rank_s()==RANK_ANY) {
            return map([this](int k) { return this->size(k); }, ra::iota(rank()));
        } else {
            std::array<dim_t, rank_s()> s {};
            for_each([this, &s](int k) { s[k] = this->size(k); }, ra::iota(rank_s()));
            return s;
        }
    }

    constexpr dim_t size()
    {
        return prod(this->shape());
    }

    constexpr static dim_t size_s()
    {
        if constexpr (rank_s()==RANK_ANY) {
            return DIM_ANY;
        } else {
            ra::dim_t s = 1;
            for (int i=0; i!=rank_s(); ++i) {
                if (int ss=size_s(i); ss==DIM_ANY) {
                    return DIM_ANY;
                } else {
                    s *= ss;
                }
            }
            return s;
        }
    }

    constexpr Axpr(Op op_, P ... p_): op(std::forward<Op>(op_)), t(std::forward<P>(p_) ...)
    {
        check_axpr(*this);
    }

    template <class J>  constexpr decltype(auto) at(J const & i)
    {
        return op(std::get<I>(t).at(i) ...);
    }

    constexpr void adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }

    constexpr bool keep_stride(dim_t step, int z, int j) const
    {
        return (std::get<I>(t).keep_stride(step, z, j) && ...);
    }

    constexpr auto stride(int i) const
    {
        return std::make_tuple(std::get<I>(t).stride(i) ...);
    }

    constexpr decltype(auto) flat()
    {
        return ra::flat(op, std::get<I>(t).flat() ...);
    }
};

template <class Op, class ... P> inline constexpr auto
axpr(Op && op, P && ... p)
{
    return Axpr<Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(p) ... };
}
// -------------------

} // namespace ra

template <int i> using TI = ra::TensorIndex<i, int>;
template <int i> using UU = decltype(std::declval<ra::Unique<double, i>>().iter());
using mp::int_t;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("view");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 2, 3, 4}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        cout << a << endl;
    }
    tr.section("II");
    {
        std::tuple<int_t<6>, int_t<3>, int_t<-4>> x;
        constexpr int ma = mp::fold_tuple(-99, x, [](auto && k, auto && a) { return max(k, std::decay_t<decltype(a)>::value); });
        constexpr int mi = mp::fold_tuple(+99, x, [](auto && k, auto && a) { return min(k, std::decay_t<decltype(a)>::value); });
        constexpr int su = mp::fold_tuple(0, x, [](auto && k, auto && a) { return k + std::decay_t<decltype(a)>::value; });
        cout << ma << endl;
        cout << mi << endl;
        cout << su << endl;
    }
    tr.section("static size - like Expr");
    {
        ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
        ra::Small<int, 2, 3, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
#define AXPR axpr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, AXPR.rank());
        tr.test_eq(b.size(0), AXPR.size(0));
        tr.test_eq(b.size(1), AXPR.size(1));
        tr.test_eq(b.size(2), AXPR.size(2));
        tr.test_eq(b.size(3), AXPR.size(3));
        tr.test_eq(2*3*4*5, AXPR.size());
        static_assert(4==AXPR.rank_s());
        static_assert(b.size_s(0)==AXPR.size_s(0));
        static_assert(b.size_s(1)==AXPR.size_s(1));
        static_assert(b.size_s(2)==AXPR.size_s(2));
        static_assert(b.size_s(3)==AXPR.size_s(3));
        static_assert(2*3*4*5 == AXPR.size_s());
#undef AXPR
    }
    // properly fails to compile, which we cannot check at present [ra42]
//     tr.section("check mismatches - static");
//     {
//         ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
//         ra::Small<int, 2, 4, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
// #define AXPR axpr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
//         tr.test_eq(2*3*4*5, AXPR.size_s());
//         tr.test_eq(3, AXPR.size_s(1));
// #undef AXPR
//     }
    tr.section("static rank, dynamic size - like Expr");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define AXPR axpr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
#define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, AXPR.rank());
        tr.test_eq(b.size(0), AXPR.size(0));
        tr.test_eq(b.size(1), AXPR.size(1));
        tr.test_eq(b.size(2), AXPR.size(2));
        tr.test_eq(b.size(3), AXPR.size(3));
        tr.test_eq(2*3*4*5, AXPR.size());
// these _s are all static, but cannot Big cannot be constexpr yet.
// also decltype(AXPR) fails until p0315r3 (c++20).
// so check at runtime instead.
        tr.test_eq(4, AXPR.rank_s());
        tr.test_eq(ra::DIM_ANY, AXPR.size_s());
        tr.test_eq(ra::DIM_ANY, AXPR.size_s(0));
        tr.test_eq(ra::DIM_ANY, AXPR.size_s(1));
        tr.test_eq(ra::DIM_ANY, AXPR.size_s(2));
        tr.test_eq(ra::DIM_ANY, AXPR.size_s(3));
        tr.test_eq(ra::DIM_ANY, AXPR.size_s());
        cout << EXPR << endl;
        cout << AXPR << endl;
#undef AXPR
    }
    tr.section("check mismatches - dynamic");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 4, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define AXPR axpr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        { int x = 0;
            try {
                tr.test_eq(ra::DIM_ANY, AXPR.size_s(1));
                x = 1;
            } catch (ra_error & e) {
                tr.info("caught error: ", e.s).test_eq(0, x);
            }
        }
#undef AXPR
    }
    tr.section("dynamic rank - Expr driver selection is broken in this case.");
    {
        ra::Big<int, 3> as({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int> ad({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> bs({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        ra::Big<int> bd({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define AXPR(a, b) axpr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        auto test = [&tr](auto tag, auto && a, auto && b)
                    {
                        tr.section(tag);
                        tr.test_eq(4, AXPR(a, b).rank());
                        tr.info("0d").test_eq(b.size(0), AXPR(a, b).size(0));
                        tr.test_eq(b.size(1), AXPR(a, b).size(1));
                        tr.test_eq(b.size(2), AXPR(a, b).size(2));
                        tr.test_eq(b.size(3), AXPR(a, b).size(3));
                        tr.test_eq(2*3*4*5, AXPR(a, b).size());
                        tr.test_eq(ra::RANK_ANY, AXPR(a, b).rank_s());
                        tr.info("0s").test_eq(ra::DIM_ANY, AXPR(a, b).size_s());
                        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(0));
                        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(1));
                        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(2));
                        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(3));
                        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s());
                    };
        test("sta-dyn", as, bd);
        test("dyn-sta", ad, bs);
        test("dyn-dyn", ad, bd);
#undef AXPR
    }
    tr.section("cases with periodic axes - dynamic (broken with Expr)");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        auto b = a(ra::all, ra::newaxos<1>, ra::iota(4, 0, 0));
#define AXPR(a, b) axpr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, AXPR(a, b).rank());
        tr.test_eq(b.size(0), AXPR(a, b).size(0));
        tr.test_eq(a.size(1), AXPR(a, b).size(1));
        tr.test_eq(b.size(2), AXPR(a, b).size(2));
        tr.test_eq(b.size(3), AXPR(a, b).size(3));
        tr.test_eq(2*3*4*4, AXPR(a, b).size());
// these _s are all static, but cannot Big cannot be constexpr yet.
// also decltype(AXPR(a, b)) fails until p0315r3 (c++20).
// so check at runtime instead.
        tr.test_eq(4, AXPR(a, b).rank_s());
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s());
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(0));
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(1));
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(2));
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(3));
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s());
        cout << AXPR(a, b) << endl;
// value test.
        ra::Big<int, 4> c({2, 3, 4, 4}, 0);
        c(ra::all, 0) = a(ra::all, ra::iota(4, 0, 0));
        c(ra::all, 1) = a(ra::all, ra::iota(4, 0, 0));
        c(ra::all, 2) = a(ra::all, ra::iota(4, 0, 0));
        tr.test_eq((a+c), AXPR(a, b));
// order doesn't affect prefix matching with Axpr
        tr.test_eq((a+c), AXPR(b, a));
#undef AXPR
    }
    tr.section("broadcasting - like outer product");
    {
        ra::Big<int, 2> a({4, 3}, 10*ra::_1+100*ra::_0);
        ra::Big<int, 1> b({5}, ra::_0);
        cout << ra::start(ra::shape(from([](auto && a, auto && b) { return a-b; }, a, b))) << endl;
#define AXPR(a, b) axpr([](auto && a, auto && b) { return a-b; }, start(a(ra::dots<2>, ra::newaxos<1>)), start(b(ra::newaxos<2>, ra::dots<1>)))
        tr.test_eq(3, AXPR(a, b).rank_s());
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(0));
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(1));
        tr.test_eq(ra::DIM_ANY, AXPR(a, b).size_s(2));
        tr.test_eq(3, AXPR(a, b).rank());
        tr.test_eq(4, AXPR(a, b).size(0));
        tr.test_eq(3, AXPR(a, b).size(1));
        tr.test_eq(5, AXPR(a, b).size(2));
        tr.test_eq(from([](auto && a, auto && b) { return a-b; }, a, b), AXPR(a, b));
#undef AXPR
    }

    return tr.summary();
}
