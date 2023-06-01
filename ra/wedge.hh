// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Wedge product and cross product.

// (c) Daniel Llorens - 2008-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "small.hh"

namespace ra::mp {

template <class P, class Plist>
struct FindCombination
{
    template <class A> using match = bool_c<0 != PermutationSign<P, A>::value>;
    using type = IndexIf<Plist, match>;
    constexpr static int where = type::value;
    constexpr static int sign = (where>=0) ? PermutationSign<P, typename type::type>::value : 0;
};

// A combination antiC complementary to C wrt [0, 1, ... Dim-1], but permuted to make the permutation [C, antiC] positive with respect to [0, 1, ... Dim-1].
template <class C, int D>
struct AntiCombination
{
    using EC = complement<C, D>;
    static_assert((len<EC>)>=2, "can't correct this complement");
    constexpr static int sign = PermutationSign<append<C, EC>, iota<D>>::value;
// Produce permutation of opposite sign if sign<0.
    using type = mp::cons<std::tuple_element_t<(sign<0) ? 1 : 0, EC>,
                          mp::cons<std::tuple_element_t<(sign<0) ? 0 : 1, EC>,
                                   mp::drop<EC, 2>>>;
};

template <class C, int D> struct MapAntiCombination;
template <int D, class ... C>
struct MapAntiCombination<std::tuple<C ...>, D>
{
    using type = std::tuple<typename AntiCombination<C, D>::type ...>;
};

template <int D, int O>
struct ChooseComponents
{
    static_assert(D>=O, "bad dimension or form order");
    using type = mp::combinations<iota<D>, O>;
};

template <int D, int O> using ChooseComponents_ = typename ChooseComponents<D, O>::type;

template <int D, int O>
requires ((D>1) && (2*O>D))
struct ChooseComponents<D, O>
{
    static_assert(D>=O, "bad dimension or form order");
    using type = typename MapAntiCombination<ChooseComponents_<D, D-O>, D>::type;
};

// Works *almost* to the range of std::size_t.
constexpr std::size_t
n_over_p(std::size_t const n, std::size_t p)
{
    if (p>n) {
        return 0;
    } else if (p>(n-p)) {
        p = n-p;
    }
    std::size_t v = 1;
    for (std::size_t i=0; i!=p; ++i) {
        v = v*(n-i)/(i+1);
    }
    return v;
}

// We form the basis for the result (Cr) and split it in pieces for Oa and Ob; there are (D over Oa) ways. Then we see where and with which signs these pieces are in the bases for Oa (Ca) and Ob (Cb), and form the product.
template <int D, int Oa, int Ob>
struct Wedge
{
    constexpr static int Or = Oa+Ob;
    static_assert(Oa<=D && Ob<=D && Or<=D, "bad orders");
    constexpr static int Na = n_over_p(D, Oa);
    constexpr static int Nb = n_over_p(D, Ob);
    constexpr static int Nr = n_over_p(D, Or);
// in lexicographic order. Can be used to sort Ca below with FindPermutation.
    using LexOrCa = mp::combinations<mp::iota<D>, Oa>;
// the actual components used, which are in lex. order only in some cases.
    using Ca = mp::ChooseComponents_<D, Oa>;
    using Cb = mp::ChooseComponents_<D, Ob>;
    using Cr = mp::ChooseComponents_<D, Or>;
// optimizations.
    constexpr static bool yields_expr = (Na>1) != (Nb>1);
    constexpr static bool yields_expr_a1 = yields_expr && Na==1;
    constexpr static bool yields_expr_b1 = yields_expr && Nb==1;
    constexpr static bool both_scalars = (Na==1 && Nb==1);
    constexpr static bool dot_plus = Na>1 && Nb>1 && Or==D && (Oa<Ob || (Oa>Ob && !ra::odd(Oa*Ob)));
    constexpr static bool dot_minus = Na>1 && Nb>1 && Or==D && (Oa>Ob && ra::odd(Oa*Ob));
    constexpr static bool general_case = (Na>1 && Nb>1) && ((Oa+Ob!=D) || (Oa==Ob));

    template <class Va, class Vb>
    using valtype = std::decay_t<decltype(std::declval<Va>()[0] * std::declval<Vb>()[0])>;

    template <class Xr, class Fa, class Va, class Vb>
    constexpr static valtype<Va, Vb>
    term(Va const & a, Vb const & b)
    {
        if constexpr (!mp::nilp<Fa>) {
            using Fa0 = mp::first<Fa>;
            using Fb = mp::complement_list<Fa0, Xr>;
            using Sa = mp::FindCombination<Fa0, Ca>;
            using Sb = mp::FindCombination<Fb, Cb>;
            constexpr int sign = Sa::sign * Sb::sign * mp::PermutationSign<mp::append<Fa0, Fb>, Xr>::value;
            static_assert(sign==+1 || sign==-1, "Bad sign in wedge term.");
            return valtype<Va, Vb>(sign)*a[Sa::where]*b[Sb::where] + term<Xr, mp::drop1<Fa>>(a, b);
        } else {
            return 0.;
        }
    }
    template <class Va, class Vb, class Vr, int wr>
    constexpr static void
    coeff(Va const & a, Vb const & b, Vr & r)
    {
        if constexpr (wr<Nr) {
            using Xr = mp::ref<Cr, wr>;
            using Fa = mp::combinations<Xr, Oa>;
            r[wr] = term<Xr, Fa>(a, b);
            coeff<Va, Vb, Vr, wr+1>(a, b, r);
        }
    }
    template <class Va, class Vb, class Vr>
    constexpr static void
    product(Va const & a, Vb const & b, Vr & r)
    {
        static_assert(int(Va::size())==Na, "bad Va dim");  // gcc accepts a.size(), etc.
        static_assert(int(Vb::size())==Nb, "bad Vb dim");
        static_assert(int(Vr::size())==Nr, "bad Vr dim");
        coeff<Va, Vb, Vr, 0>(a, b, r);
    }
};

// This is for Euclidean space, it only does component shuffling.
template <int D, int O>
struct Hodge
{
    using W = Wedge<D, O, D-O>;
    using Ca = typename W::Ca;
    using Cb = typename W::Cb;
    using Cr = typename W::Cr;
    using LexOrCa = typename W::LexOrCa;
    constexpr static int Na = W::Na;
    constexpr static int Nb = W::Nb;

    template <int i, class Va, class Vb>
    constexpr static void
    hodge_aux(Va const & a, Vb & b)
    {
        static_assert(i<=W::Na, "Bad argument to hodge_aux");
        if constexpr (i<W::Na) {
            using Cai = mp::ref<Ca, i>;
            static_assert(mp::len<Cai> == O, "bad");
// sort Cai, because mp::complement only accepts sorted combinations.
// ref<Cb, i> should be complementary to Cai, but I don't want to rely on that.
            using SCai = mp::ref<LexOrCa, mp::FindCombination<Cai, LexOrCa>::where>;
            using CompCai = mp::complement<SCai, D>;
            static_assert(mp::len<CompCai> == D-O, "bad");
            using fpw = mp::FindCombination<CompCai, Cb>;
// for the sign see e.g. DoCarmo1991 I.Ex 10.
            using fps = mp::FindCombination<mp::append<Cai, mp::ref<Cb, fpw::where>>, Cr>;
            static_assert(fps::sign!=0, "bad");
            b[fpw::where] = decltype(a[i])(fps::sign)*a[i];
            hodge_aux<i+1>(a, b);
        }
    }
};

// The order of components is taken from Wedge<D, O, D-O>; this works for whatever order is defined there.
// With lexicographic order, component order is reversed, but signs vary.
// With the order given by ChooseComponents<>, fpw::where==i and fps::sign==+1 in hodge_aux(), always. Then hodge() becomes a free operation, (with one exception) and the next function hodge() can be used.
template <int D, int O, class Va, class Vb>
constexpr void
hodgex(Va const & a, Vb & b)
{
    static_assert(O<=D, "bad orders");
    static_assert(Va::size()==mp::Hodge<D, O>::Na, "error"); // gcc accepts a.size(), etc.
    static_assert(Vb::size()==mp::Hodge<D, O>::Nb, "error");
    mp::Hodge<D, O>::template hodge_aux<0>(a, b);
}

} // namespace ra::mp

namespace ra {

// This depends on Wedge<>::Ca, Cb, Cr coming from ChooseCombinations, as enforced in test_wedge_product. hodgex() should always work, but this is cheaper.
// However if 2*O=D, it is not possible to differentiate the bases by order and hodgex() must be used.
// Likewise, when O(N-O) is odd, Hodge from (2*O>D) to (2*O<D) change sign, since **w= -w in that case, and the basis in the (2*O>D) case is selected to make Hodge(<)->Hodge(>) trivial; but can't do both!
#define TRIVIAL(D, O) (2*O!=D && ((2*O<D) || !ra::odd(O*(D-O))))

template <int D, int O, class Va, class Vb>
constexpr void
hodge(Va const & a, Vb & b)
{
    if constexpr (TRIVIAL(D, O)) {
        static_assert(Va::size()==mp::Hodge<D, O>::Na, "error"); // gcc accepts a.size(), etc
        static_assert(Vb::size()==mp::Hodge<D, O>::Nb, "error");
        b = a;
    } else {
        ra::mp::hodgex<D, O>(a, b);
    }
}

template <int D, int O, class Va>
requires (TRIVIAL(D, O))
constexpr Va const &
hodge(Va const & a)
{
    static_assert(Va::size()==mp::Hodge<D, O>::Na, "error"); // gcc accepts a.size()
    return a;
}

template <int D, int O, class Va>
requires (!TRIVIAL(D, O))
constexpr Va &
hodge(Va & a)
{
    Va b(a);
    ra::mp::hodgex<D, O>(b, a);
    return a;
}
#undef TRIVIAL


// --------------------
// Wedge product
// TODO Handle the simplifications dot_plus, yields_scalar, etc. just as vec::wedge does.
// --------------------

template <int D, int Oa, int Ob, class A, class B>
requires (ra::is_scalar<A> && ra::is_scalar<B>)
constexpr auto wedge(A const & a, B const & b) { return a*b; }

template <class A>
struct torank1
{
    using type = std::conditional_t<is_scalar<A>, Small<std::decay_t<A>, 1>, A>;
};

template <class Wedge, class Va, class Vb>
struct fromrank1
{
    using valtype = typename Wedge::template valtype<Va, Vb>;
    using type = std::conditional_t<Wedge::Nr==1, valtype, Small<valtype, Wedge::Nr>>;
};

#define DECL_WEDGE(condition)                                           \
    template <int D, int Oa, int Ob, class Va, class Vb>                \
    requires (!(is_scalar<Va> && is_scalar<Vb>))                        \
    decltype(auto)                                                      \
    wedge(Va const & a, Vb const & b)
DECL_WEDGE(general_case)
{
    Small<value_t<Va>, size_s<Va>()> aa = a;
    Small<value_t<Vb>, size_s<Vb>()> bb = b;

    using Ua = decltype(aa);
    using Ub = decltype(bb);

    typename fromrank1<mp::Wedge<D, Oa, Ob>, Ua, Ub>::type r;

    auto & r1 = reinterpret_cast<typename torank1<decltype(r)>::type &>(r);
    auto & a1 = reinterpret_cast<typename torank1<Ua>::type const &>(aa);
    auto & b1 = reinterpret_cast<typename torank1<Ub>::type const &>(bb);
    mp::Wedge<D, Oa, Ob>::product(a1, b1, r1);

    return r;
}
#undef DECL_WEDGE

#define DECL_WEDGE(condition)                                           \
    template <int D, int Oa, int Ob, class Va, class Vb, class Vr>      \
    requires (!(is_scalar<Va> && is_scalar<Vb>))                        \
    void                                                                \
    wedge(Va const & a, Vb const & b, Vr & r)
DECL_WEDGE(general_case)
{
    Small<value_t<Va>, size_s<Va>()> aa = a;
    Small<value_t<Vb>, size_s<Vb>()> bb = b;

    using Ua = decltype(aa);
    using Ub = decltype(bb);

    auto & r1 = reinterpret_cast<typename torank1<decltype(r)>::type &>(r);
    auto & a1 = reinterpret_cast<typename torank1<Ua>::type const &>(aa);
    auto & b1 = reinterpret_cast<typename torank1<Ub>::type const &>(bb);
    mp::Wedge<D, Oa, Ob>::product(a1, b1, r1);
}
#undef DECL_WEDGE

template <class A, class B> requires (size_s<A>()==2 && size_s<B>()==2)
constexpr auto
cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(FLAT(a_))>, 2> a = a_;
    Small<std::decay_t<decltype(FLAT(b_))>, 2> b = b_;
    Small<std::decay_t<decltype(FLAT(a_) * FLAT(b_))>, 1> r;
    mp::Wedge<2, 1, 1>::product(a, b, r);
    return r[0];
}

template <class A, class B> requires (size_s<A>()==3 && size_s<B>()==3)
constexpr auto
cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(FLAT(a_))>, 3> a = a_;
    Small<std::decay_t<decltype(FLAT(b_))>, 3> b = b_;
    Small<std::decay_t<decltype(FLAT(a_) * FLAT(b_))>, 3> r;
    mp::Wedge<3, 1, 1>::product(a, b, r);
    return r;
}

template <class V>
constexpr auto
perp(V const & v)
{
    static_assert(v.size()==2, "dimension error");
    return Small<std::decay_t<decltype(FLAT(v))>, 2> {v[1], -v[0]};
}

template <class V, class U>
constexpr auto
perp(V const & v, U const & n)
{
    if constexpr (is_scalar<U>) {
        static_assert(v.size()==2, "dimension error");
        return Small<std::decay_t<decltype(FLAT(v) * n)>, 2> {v[1]*n, -v[0]*n};
    } else {
        static_assert(v.size()==3, "dimension error");
        return cross(v, n);
    }
}

} // namespace ra
