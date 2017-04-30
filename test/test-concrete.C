
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-concrete.C
/// @brief Tests for concrete_type.

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/concrete.H"
#include "ra/test.H"
#include "ra/mpdebug.H"
#include <memory>

using std::cout; using std::endl;

int main()
{
    TestRecorder tr(std::cout);

    tr.section("scalars");
    {
        int a = 3;
        int b = 4;
        using K = ra::concrete_type<decltype(a+b)>;
        cout << mp::type_name<K>() << endl;
        tr.info("scalars are their own concrete_types").test(std::is_same<K, int>::value);
        auto c = ra::concrete(a+b);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a+b, c);
        auto d = ra::concrete(a);
        d = 99;
        tr.info("concrete() makes copies (", d, ")").test_eq(a, 3);
    }
    tr.section("fixed size");
    {
        ra::Small<int, 3> a = {1, 2, 3};
        ra::Small<int, 3> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same<K, ra::Small<int, 3>>::value);
        auto c = concrete(a+b);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a+b, c);
    }
    tr.section("var size");
    {
        ra::Owned<int, 1> a = {1, 2, 3};
        ra::Owned<int, 1> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same<K, ra::Owned<int, 1>>::value);
        auto c = concrete(a+b);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a+b, c);
    }
    tr.section("var size + fixed size");
    {
        ra::Small<int, 3, 2> a = {1, 2, 3, 4, 5, 6};
        ra::Owned<int, 1> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same<K, ra::Small<int, 3, 2>>::value);
        auto c = concrete(a+b);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a+b, c);
    }
    tr.section("var size + var rank");
    {
        ra::Owned<int, 1> a = {1, 2, 3};
        ra::Owned<int> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
// ra:: b could be higher rank and that decides the type.
        tr.test(std::is_same<K, ra::Owned<int>>::value);
        auto c = concrete(a+b);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a+b, c);
    }
    tr.section("concrete on is_slice fixed size");
    {
        ra::Small<int, 3> a = {1, 2, 3};
        auto c = concrete(a);
        using K = decltype(c);
        tr.test(std::is_same<K, ra::Small<int, 3>>::value);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a, c);
        a = 99;
        tr.test_eq(99, a);
        tr.info("concrete() makes copies").test_eq(K {1, 2, 3}, c);
    }
    tr.section("concrete on is_slice var size");
    {
        ra::Owned<int, 1> a = {1, 2, 3};
        auto c = concrete(a);
        using K = decltype(c);
        tr.test(std::is_same<K, ra::Owned<int, 1>>::value);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a, c);
        a = 99;
        tr.test_eq(99, a);
        tr.info("concrete() makes copies").test_eq(K {1, 2, 3}, c);
    }
    tr.section("concrete on foreign vector");
    {
        std::vector<int> a = {1, 2, 3};
        auto c = ra::concrete(a);
        using K = decltype(c);
        tr.test(std::is_same<K, ra::Owned<int, 1>>::value);
        tr.test(std::is_same<decltype(c), K>::value);
        tr.test_eq(a, c);
        ra::start(a) = 99;
        tr.test_eq(99, ra::start(a));
        tr.info("concrete() makes copies").test_eq(K {1, 2, 3}, c);
    }
    return tr.summary();
}
