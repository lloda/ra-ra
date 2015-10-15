
// (c) Daniel Llorens - 2011-2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_EXPR_H
#define RA_EXPR_H

/// @file ra-expr.H
/// @brief Expression templates for arrays with frame matching: operator nodes.

#include "ra/ra-ply.H"

#ifdef EK_CHECK_BOUNDS
    #define EK_CHECK_BOUNDS_RA_EXPR EK_CHECK_BOUNDS
#else
    #ifndef EK_CHECK_BOUNDS_RA_EXPR
        #define EK_CHECK_BOUNDS_RA_EXPR 1
    #endif
#endif
#if EK_CHECK_BOUNDS_RA_EXPR==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

// Manipulate Expr/expr through leaf (raw pointer-like) iterators P ...
// Of these, the A-th one is used to advance (+) and end (!=) the traversal.
template <class F, int A, class T, class I=mp::index_tuple_<T>>
struct Leaves;

template <class F, int A, class ... P, class ... I>
struct Leaves<F, A, std::tuple<P ...>, std::tuple<I ...>>
{
    using T = std::tuple<P ...>;
    using K = std::tuple<I ...>;

    F f;
    T t;
    Leaves(F f_, P ... p_): f(f_), t(p_ ...) {}
    decltype(auto) operator*() { return f(*std::get<I::value>(t) ...); }

    template <int II, class S>
    enableifc_<(II==mp::Len<K>::value)> add(S const & s)  {}

// @TODO Step over (or flag) sub xpr without stride (TensorIndex, Scalar).
    template <int II, class S>
    enableifc_<(II<mp::Len<K>::value && std::is_integral<S>::value)> add(S const & s)
    {
        std::get<mp::Ref_<K, II>::value>(t) += s;
        add<II+1>(s);
    }
    template <int II, class S>
    enableifc_<(II<mp::Len<K>::value && !std::is_integral<S>::value)> add(S const & s)
    {
        std::get<mp::Ref_<K, II>::value>(t) += std::get<mp::Ref_<K, II>::value>(s);
        add<II+1>(s);
    }
    template <class S> void operator+=(S const & s) { add<0>(s); }

// go down to the first concrete iterator type. These functions match each other.
    auto operator+(dim_t const s) const { return std::get<A>(t)+s; }
    template <class PP> bool operator!=(PP const & p0_) const { return (std::get<A>(t)+0)!=p0_; }
    template <class PP> bool operator==(PP const & p0_) const { return (std::get<A>(t)+0)==p0_; }
};


template <class Op, class ... P>
Expr<Op, std::tuple<P ...>> expr(Op && f, P && ... p)
{
    return Expr<Op, std::tuple<P ...>> { std::forward<Op>(f), std::forward<P>(p) ... };
}

// forward decl in ra-atom.H
template <class Op, class ... P, class ... I>
struct Expr<Op, std::tuple<P ...>, std::tuple<I ...>>
{
    using T = std::tuple<P ...>;
    using K = std::tuple<I ...>;

// A-th argument decides rank and shape.
    static int const A = largest_rank<P ...>::value;
    using PA = std::decay_t<mp::Ref_<T, A>>;

    using AK = mp::Cons_<mp::int_t<A>, mp::ComplementList_<mp::int_list<A>, K>>;
    using leaves_t = Leaves<Op, A, std::tuple<decltype(std::declval<P>().flat()) ...>>;
    using stride_t = std::tuple<decltype(std::declval<P>().stride(0)) ...>;

    Op op;
    T t;

// If driver is RANK_ANY, driver selection should be delayed to run time, except when we can tell statically that RANK_ANY would be selected anyway.
    static bool const HAS_DRIVER = PA::size_s()!=DIM_BAD ; //&& (PA::rank_s()!=RANK_ANY || sizeof...(P)==1);

    template <bool depend=true> auto & driver() const
    {
        static_assert(HAS_DRIVER, "can't drive this xpr");
        return std::get<A>(t);
    }

    template <int iarg>
    enableifc_<(iarg==mp::Len<AK>::value)>
    check(int const driver_rank) const {}

    template <int iarg>
    enableifc_<(iarg<mp::Len<AK>::value)>
    check(int const driver_rank) const
    {
        rank_t ranki = std::get<mp::Ref_<AK, iarg>::value>(t).rank();
// @TODO Check separately the valid rank of TensorIndex exprs [(w+1) here].
        if (ranki!=RANK_BAD) {
// Provide safety where RANK_ANY was selected as driver in a leap of faith. @TODO Disable this test when it's known statically that it isn't needed. @TODO Do dynamic driver selection if static is uncertain.
            assert(ranki<=driver_rank && "driver not max rank (could be RANK_ANY)");
            for (int k=0; k!=ranki; ++k) {
                dim_t sk = std::get<mp::Ref_<AK, iarg>::value>(t).size(k);
                dim_t sk0 = std::get<A>(t).size(k);
                assert((sk==sk0 || sk==DIM_BAD) && "mismatched dimensions");
            }
        }
        return check<iarg+1>(driver_rank);
    }

    Expr(Op op_, P ... p_): op(op_), t(p_ ...)
    {
// @TODO Try to static_assert. E.g., size_s() vs size_s() can static_assert if we try real3==real2.
// @TODO Should check only the top xpr: do this on ply.
        check<1>(rank());
    }

    template <class J>
    decltype(auto) at(J const & i)
    {
        return op(std::get<I::value>(t).at(i) ...);
    }

    template <int iarg>
    enableifc_<(iarg==mp::Len<T>::value)>
    adv(rank_t const k, dim_t const d) {}

    template <int iarg=0>
    enableifc_<(iarg<mp::Len<T>::value)>
    adv(rank_t const k, dim_t const d)
    {
        std::get<iarg>(t).adv(k, d);
        adv<iarg+1>(k, d);
    }

// there's one size (by A), but each arg has its own strides.
// Note: do not require driver. This is needed by check for all leaves.
    constexpr auto size(int const i) const -> dim_t
    {
        return std::get<A>(t).size(i);
    }
    auto stride(int const i) const -> stride_t
    {
        return stride_t(std::get<I::value>(t).stride(i) ...);
    }
    auto preferred_stride(int const i) const -> dim_t
    {
        return driver().preferred_stride(i);
    }

    template <int iarg>
    enableifc_<(iarg==mp::Len<K>::value), bool>
    compact_stride(dim_t const step, int const z, int const j) const
    {
        return true;
    }
    template <int iarg=0>
    enableifc_<(iarg<mp::Len<K>::value), bool>
    compact_stride(dim_t const step, int const z, int const j) const
    {
        return std::get<mp::Ref_<K, iarg>::value>(t).compact_stride(step, z, j)
               && compact_stride<iarg+1>(step, z, j);
    }

    auto flat() const -> leaves_t
    {
        return leaves_t(op, std::get<I::value>(t).flat() ...);
    }
    bool done() const { return driver().done(); }

    constexpr static dim_t size_s() { return PA::size_s(); }
    constexpr static rank_t rank_s() { return PA::rank_s(); }
// Note: do not require driver. This is needed by check for all leaves.
    constexpr rank_t rank() const { return std::get<A>(t).rank(); }
    decltype(auto) shape() const { return driver().shape(); }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    using atom_type = decltype(*std::declval<leaves_t>());
    operator atom_type()
    {
        static_assert(rank_s()==0 || rank_s()==RANK_ANY || (rank_s()==1 && size_s()==1), // for coord types
                      "bad rank in conversion to scalar");
        assert(rank()==0 || (rank_s()==1 && size_s()==1)); // for coord types; so fixed only
        return *flat();
    }

// forward to make sure value y is not misused as ref.
#define DEF_EXPR_EQOPS(OP)                                              \
    template <class X> void operator OP(X && x)                         \
    {                                                                   \
        ply_either(expr([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, start(x))); \
    }
    FOR_EACH(DEF_EXPR_EQOPS, =, *=, +=, -=, /=)
#undef DEF_EXPR_EQOPS
};

// forward to make sure value y is not misused as ref.
#define DEF_VECTOR_EQOPS(OP)                                            \
    template <class V> template <class X> void Vector<V>::operator OP(X && x) \
    {                                                                   \
        ply_either(expr([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, start(x))); \
    }
FOR_EACH(DEF_VECTOR_EQOPS, =, *=, +=, -=, /=)
#undef DEF_VECTOR_EQOPS

} // namespace ra

#undef CHECK_BOUNDS
#undef EK_CHECK_BOUNDS_RA_EXPR

#endif // RA_EXPR_H
