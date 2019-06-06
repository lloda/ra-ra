// -*- mode: c++; coding: utf-8 -*-
/// @file const.C
/// @brief Reduced regression from 8.3 to 9.1.

// /opt/gcc-9.1/bin/g++ -o const -std=c++17 -Wall -Werror -ftemplate-backtrace-limit=0 const.C
// /opt/gcc-8.3/bin/g++ -o const -std=c++17 -Wall -Werror -ftemplate-backtrace-limit=0 const.C

// from redi @ #gcc
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90745

/*
<lloda> I posted on ##C++ but maybe here is better                      [16:29]
<lloda> https://wandbox.org/permlink/WNhI31fE28NQqIzP
<lloda> error with 9.1 but fine in 8.3
<lloda> not using tuple (#ifdef 0) makes the error go away              [16:30]
<lloda> I don't understand why cell<view<const int>> operator= is even
        instantiated :-/                                                [16:31]
<redi> lloda: I'm pretty sure it's a consequence of
       https://gcc.gnu.org/r263625 for https://gcc.gnu.org/PR86963 but I'm not
       sure why yet                                                     [17:09]
<redi> I think there's a failure outside the immediate context of a SFINAE
       check
<lloda> redi: thanks                                                    [17:15]
<redi> lloda: I also don't understand why cell<view<const int>> operator= is
       even instantiated
<lloda> clang gives a similar error, only it hides the library steps. I'll try
        to digest the links.                                            [17:27]
<redi> lloda: yeah I see the problem. It's as I thought                 [17:34]
<redi> the copy assignment operator of std::tuple now uses
       conditional<__assignable<const _T1&, const _T2&>(), const tuple&, const
       __nonesuch_no_braces&>
<redi> but it's not a dependent context, so SFINAE doesn't apply
<redi> the __is_assignable check blows up                               [17:35]
<redi> the instantiation of std::tuple<x,y> triggers the instantiations of its
       copy assignment operator (because it's not a template) and that blows
       up                                                               [17:36]
<redi> drat, I need to fix this
<zid> Agreed, burn the C++ spec                                         [17:37]
*/

#include <tuple>

template <class Op, class P0, class P1>
struct Expr
{
    Op op;
    std::tuple<P0, P1> t;

    using R = decltype(op(*(std::get<0>(t).p), *(std::get<1>(t).p)));
    operator R() { return op(*(std::get<0>(t).p), *(std::get<1>(t).p)); }
};

template <class Op, class P0, class P1> inline auto
expr(Op && op, P0 && p0, P1 && p1)
{
    return Expr<Op, P0, P1> { op, { p0, p1 } };
}

template <class Op, class P0, class P1> inline void
for_each(Op && op, P0 && p0, P1 && p1)
{
    expr(op, p0, p1);
}

template <class V>
struct cell
{
    typename V::value_type * p;
    cell(typename V::value_type * p_): p { p_ } {}

    template <class X> decltype(auto) operator =(X && x)
    {
        for_each([](auto && y, auto && x)
                 { y = x; },
            *this, x);
    }
};

template <class T>
struct view
{
    T * p;
    using value_type = T;
    cell<view<T>> iter() { return p; }
    cell<view<T const>> iter() const { return p; }
    view(T * p_): p(p_) {}
};

int main()
{
    {
        int cdata[2] = {44, 44};
        int ndata[2] = {77, 77};
        view<int> const c {cdata};
        view<int> n {ndata};
        for_each([](auto && n, auto && c) { n = c; }, n.iter(), c.iter());
    }
}
