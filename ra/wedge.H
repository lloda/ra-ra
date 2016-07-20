
// (c) Daniel Llorens - 2008-2011, 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_WEDGE_H
#define RA_WEDGE_H

/// @file wedge.H
/// @brief Wedge product and cross product.

#include "ra/tuple-list.H"
#include "ra/ra-type.H" // only for is_scalar<>

// This global version must be available so that ra::wedge<> may be searched by ADL even when giving explicit template args. See http://stackoverflow.com/questions/9838862 .
template <int D, int Oa, int Ob, class A, class B,
          std::enable_if_t<ra::is_scalar<A> && ra::is_scalar<B>, int> =0>
auto wedge(A const & a, B const & b) { return a*b; }

namespace mp {

template <class P>
struct MatchPermutationP
{
    template <class A> using type = int_t<(PermutationSign<P, A>::value!=0)>;
};

template <class P, class Plist> struct FindCombination
{
    using type = IndexIf<Plist, mp::MatchPermutationP<P>::template type>;
    static int const where = type::value;
    static int const sign = (where>=0) ? PermutationSign<P, typename type::type>::value :0;
};

// Produce a permutation of opposite sign if sign = -1.
template <int sign, class C> struct PermutationFlipSign;
template <class C0, class C1, class ... C>
struct PermutationFlipSign<-1, std::tuple<C0, C1, C ...>>
{
    using type = std::tuple<C1, C0, C ...>;
};
template <class C0, class C1, class ... C>
struct PermutationFlipSign<+1, std::tuple<C0, C1, C ...>>
{
    using type = std::tuple<C0, C1, C ...>;
};

// A combination antiC complementary to C wrt [0, 1, ... Dim-1], but permuted to make the permutation [C, antiC] positive with respect to [0, 1, ... Dim-1].
template <class C, int D>
struct AntiCombination
{
    using EC = typename Complement<C, D>::type;
    static_assert((len<EC>)>=2, "can't correct this complement");
    using type = typename PermutationFlipSign
        <PermutationSign<Append_<C, EC>, Iota_<D>>::value,
         EC>::type;
};

template <class C, int D> struct MapAntiCombination;
template <int D, class ... C>
struct MapAntiCombination<std::tuple<C ...>, D>
{
    using type = std::tuple<typename AntiCombination<C, D>::type ...>;
};

template <int D, int O, class Enable=void>
struct ChooseComponents
{
    static_assert(D>=O, "bad dimension or form order");
    using type = Combinations_<Iota_<D>, O>;
};
template <int D, int O>
struct ChooseComponents<D, O, std::enable_if_t<(D>1) && (2*O>D)>>
{
    static_assert(D>=O, "bad dimension or form order");
    using C = typename ChooseComponents<D, D-O>::type;
    using type = typename MapAntiCombination<C, D>::type;
};
template <int D, int O> using ChooseComponents_ = typename ChooseComponents<D, O>::type;

} // namespace mp

namespace fun {

// We form the basis for the result (Cr) and split it in pieces for Oa and Ob; there are (D over Oa) ways. Then we see where and with which signs these pieces are in the bases for Oa (Ca) and Ob (Cb), and form the product.
template <int D, int Oa, int Ob>
struct Wedge
{
    constexpr static int Or = Oa+Ob;
    static_assert(Oa<=D && Ob<=D && Or<=D, "bad orders");
    constexpr static int Na = mp::n_over_p(D, Oa);
    constexpr static int Nb = mp::n_over_p(D, Ob);
    constexpr static int Nr = mp::n_over_p(D, Or);
// in lexicographic order. Can be used to sort Ca below with FindPermutation.
    using LexOrCa = mp::Combinations_<mp::Iota_<D>, Oa>;
// the actual components used, which are in lex. order only in some cases.
    using Ca = mp::ChooseComponents_<D, Oa>;
    using Cb = mp::ChooseComponents_<D, Ob>;
    using Cr = mp::ChooseComponents_<D, Or>;
// optimizations.
    constexpr static bool yields_expr = (Na>1) != (Nb>1);
    constexpr static bool yields_expr_a1 = yields_expr && Na==1;
    constexpr static bool yields_expr_b1 = yields_expr && Nb==1;
    constexpr static bool both_scalars = (Na==1 && Nb==1);
    constexpr static bool dot_plus = Na>1 && Nb>1 && Or==D && (Oa<Ob || (Oa>Ob && !odd(Oa*Ob)));
    constexpr static bool dot_minus = Na>1 && Nb>1 && Or==D && (Oa>Ob && odd(Oa*Ob));
    constexpr static bool general_case = (Na>1 && Nb>1) && ((Oa+Ob!=D) || (Oa==Ob));

    template <class Va, class Vb>
    using valtype = std::decay_t<decltype(std::declval<Va>()[0] * std::declval<Vb>()[0])>;

    template <class Xr, class Fa, class Va, class Vb>
    static std::enable_if_t<mp::nilp<Fa>, valtype<Va, Vb>>
    term(Va const & a, Vb const & b)
    {
        return 0.;
    }
    template <class Xr, class Fa, class Va, class Vb>
    static std::enable_if_t<!mp::nilp<Fa>, valtype<Va, Vb>>
    term(Va const & a, Vb const & b)
    {
        using Fa0 = mp::First_<Fa>;
        using Fb = mp::ComplementList_<Fa0, Xr>;
        using Sa = mp::FindCombination<Fa0, Ca>;
        using Sb = mp::FindCombination<Fb, Cb>;
        constexpr int sign = Sa::sign * Sb::sign * mp::PermutationSign<mp::Append_<Fa0, Fb>, Xr>::value;
        static_assert(sign==+1 || sign==-1, "bad sign in wedge term");
        return valtype<Va, Vb>(sign)*a[Sa::where]*b[Sb::where] + term<Xr, mp::Drop1_<Fa>>(a, b);
    }
    template <class Va, class Vb, class Vr, int wr>
    static std::enable_if_t<(wr<Nr)>
    coeff(Va const & a, Vb const & b, Vr & r)
    {
        using Xr = mp::Ref_<Cr, wr>;
        using Fa = mp::Combinations_<Xr, Oa>;
        r[wr] = term<Xr, Fa>(a, b);
        coeff<Va, Vb, Vr, wr+1>(a, b, r);
    }
    template <class Va, class Vb, class Vr, int wr>
    static std::enable_if_t<(wr==Nr)>
    coeff(Va const & a, Vb const & b, Vr & r)
    {
    }
    template <class Va, class Vb, class Vr>
    static void product(Va const & a, Vb const & b, Vr & r)
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
    static std::enable_if_t<(i==W::Na)>
    hodge_aux(Va const & a, Vb & b)
    {
    }
    template <int i, class Va, class Vb>
    static std::enable_if_t<(i<W::Na)>
    hodge_aux(Va const & a, Vb & b)
    {
        using Cai = mp::Ref_<Ca, i>;
        static_assert(mp::len<Cai> == O, "bad");
// sort Cai, because Complement only accepts sorted combinations.
// Ref<Cb, i> should be complementary to Cai, but I don't want to rely on that.
        using SCai = mp::Ref_<LexOrCa, mp::FindCombination<Cai, LexOrCa>::where>;
        using CompCai = typename mp::Complement<SCai, D>::type;
        static_assert(mp::len<CompCai> == D-O, "bad");
        using fpw = mp::FindCombination<CompCai, Cb>;
// for the sign see e.g. DoCarmo1991 I.Ex 10.
        using fps = mp::FindCombination<mp::Append_<Cai, mp::Ref_<Cb, fpw::where>>, Cr>;
        static_assert(fps::sign!=0, "bad");
        b[fpw::where] = decltype(a[i])(fps::sign)*a[i];
        hodge_aux<i+1>(a, b);
    }
};

// The order of components is taken from Wedge<D, O, D-O>; this works for whatever order is defined there.
// With lexicographic order, component order is reversed, but signs vary.
// With the order given by ChooseComponents<>, fpw::where==i and fps::sign==+1 in hodge_aux(), always. Then hodge() becomes a free operation, (with one exception) and the next function hodge() can be used.
template <int D, int O, class Va, class Vb>
void hodgex(Va const & a, Vb & b)
{
    static_assert(O<=D, "bad orders");
    static_assert(Va::size()==Hodge<D, O>::Na, "error"); // gcc accepts a.size(), etc.
    static_assert(Vb::size()==Hodge<D, O>::Nb, "error");
    Hodge<D, O>::template hodge_aux<0>(a, b);
}

// This depends on Wedge<>::Ca, Cb, Cr coming from ChooseCombinations, as enforced in the tests in test_wedge_product. hodgex() should always work, but this is cheaper.
// However if 2*O=D, it is not possible to differentiate the bases by order and hodgex() must be used.
// Likewise, when O(N-O) is odd, Hodge from (2*O>D) to (2*O<D) change sign, since **w= -w in that case, and the basis in the (2*O>D) case is selected to make Hodge(<)->Hodge(>) trivial; but can't do both!
#define TRIVIAL(D, O) (2*O!=D && ((2*O<D) || !odd(O*(D-O))))
template <int D, int O, class Va, class Vb>
std::enable_if_t<TRIVIAL(D, O)> hodge(Va const & a, Vb & b)
{
    static_assert(Va::size()==fun::Hodge<D, O>::Na, "error"); // gcc accepts a.size(), etc
    static_assert(Vb::size()==fun::Hodge<D, O>::Nb, "error");
    b = a;
}
template <int D, int O, class Va>
std::enable_if_t<TRIVIAL(D, O), Va const &> hodge(Va const & a)
{
    static_assert(Va::size()==fun::Hodge<D, O>::Na, "error"); // gcc accepts a.size()
    return a;
}
template <int D, int O, class Va, class Vb>
std::enable_if_t<!TRIVIAL(D, O)> hodge(Va const & a, Vb & b)
{
    fun::hodgex<D, O>(a, b);
}
template <int D, int O, class Va>
std::enable_if_t<!TRIVIAL(D, O), Va &> hodge(Va & a)
{
    Va b(a);
    fun::hodgex<D, O>(b, a);
    return a;
}
#undef TRIVIAL

} // namespace fun

#endif // RA_WEDGE
