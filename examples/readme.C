
// (c) Daniel Llorens - 2016
// Examples used in top-level README.md

// @TODO Generate README.md and/or these examples.

#include "ra/ra-operators.H"
#include "ra/test.H"
#include <iostream>

using std::cout; using std::endl;

int main()
{
    section("dynamic/static shape");
    // Dynamic or static array rank. Dynamic or static array shape (all dimensions or none).
    {
        ra::Owned<char> A({2, 3}, 'a');     // dynamic rank = 2, dynamic shape = {2, 3}
        ra::Owned<char, 2> B({2, 3}, 'b');  // static rank = 2, dynamic shape = {2, 3}
        ra::Small<char, 2, 3> C('c');       // static rank = 2, static shape = {2, 3}
        cout << "A: " << A << "\n\n";
        cout << "B: " << B << "\n\n";
        cout << "C: " << C << "\n\n";
    }
    section("storage");
    // Memory-owning types and views. You can make array views over any piece of memory.
    {
        // memory-owning types
        ra::Owned<char, 2> A({2, 3}, 'a');   // storage is std::vector inside A
        ra::Unique<char, 2> B({2, 3}, 'b');  // storage is owned by std::unique_ptr inside B
        ra::Small<char, 2, 3> C('c');        // storage is owned by C itself, on the stack

        cout << "A: " << A << "\n\n";
        cout << "B: " << B << "\n\n";
        cout << "C: " << C << "\n\n";

        // view types
        char cs[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
        ra::Raw<char, 2> D1({2, 3}, cs);            // dynamic sizes and strides, C order
        ra::Raw<char, 2> D2({{2, 1}, {3, 2}}, cs);  // dynamic sizes and strides, Fortran order.
        ra::SmallSlice<char, mp::int_list<2, 3>, mp::int_list<3, 1>> D3(cs); // static sizes & strides, C order.
        ra::SmallSlice<char, mp::int_list<2, 3>, mp::int_list<1, 2>> D4(cs); // static sizes & strides, Fortran order.

        cout << "D1: " << D1 << "\n\n";
        cout << "D2: " << D2 << "\n\n";
        cout << "D3: " << D3 << "\n\n";
        cout << "D4: " << D4 << "\n\n";
    }
    section("shape agreement");
    // Shape agreement rules and rank extension (broadcasting) for rank-0 operations of any arity
    // and operands of any rank, any of which can a reference (so you can write on them). These
    // rules are taken from the array language, J.
    // (See examples/agreement.C for more examples.)
    {
        ra::Owned<float, 2> A({2, 3}, { 1, 2, 3, 1, 2, 3 });
        ra::Owned<float, 1> B({-1, +1});

        ra::Owned<float, 2> C({2, 3}, 99.);
        C = A * B;   // C(i, j) = A(i, j) * C(i)
        cout << "C: " << C << "\n\n";

        ra::Owned<float, 1> D({2}, 0.);
        D += A * B;  // D(i) += A(i, j) * C(i)
        cout << "D: " << D << "\n\n";
    }
    section("rank iterators");
    // Iterators over cells of arbitrary rank.
    {
        ra::TensorIndex<0> i;
        ra::TensorIndex<1> j;
        ra::TensorIndex<2> k;
        ra::Owned<float, 3> A({2, 3, 4}, i+j+k);
        ra::Owned<float, 2> B({2, 3}, 0);
        cout << "A: " << A << "\n\n";

        // store the sum of A(i, j, ...) in B(i, j). All these are equivalent.
        B = 0; B += A;                                                           // default agreement matches prefixes
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A);            // default agreement matches prefixes
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A.iter<1>());  // give cell rank
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A.iter<-2>()); // give frame rank
        cout << "B: " << B << "\n\n";

        // store the sum of A(i, ...) in B(i, j). The op is re-executed for each j, so don't do it this way.
        for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, A.iter<2>()); // give cell rank
        cout << "B: " << B << "\n\n";
    }
    // A rank conjunction (only for static rank and somewhat fragile).
    section("rank conjuction");
    {
        // This is a translation of J: A = (i.3) -"(0 1) i.4, that is: A(i, j) = b(i)-c(j).
        // @TODO ra::map doesn't support verbs-with-rank yet. When it does, map will replace ryn here.
        ra::Owned<float, 2> A = ryn(ra::wrank<0, 1>::make(std::minus<float>()), ra::iota(3), ra::iota(4));
        cout << "A: " << A << "\n\n";
    }
    // A proper selection operator with 'beating' of range or scalar subscripts.
    // See examples/slicing.C for more examples.
    section("selector");
    {
        // @TODO do implicit reshape in constructors?? so I can accept any 1-array and not only an initializer_list.
        ra::Owned<char, 3> A({2, 2, 2}, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'});
        cout << "A: " << A << "\n\n";

        // these are all equivalent to e.g. A(:, 0, :) in Octave.
        cout << "A1: " << A(ra::all, 0) << "\n\n";
        cout << "A2: " << A(ra::all, 0, ra::all) << "\n\n";
        cout << "A3: " << A(ra::all, 0, ra::dots<1>) << "\n\n";

        // an inverted range.
        cout << "A4: " << A(ra::iota(2, 1, -1)) << "\n\n";

        // indices can be arrays of any rank.
        ra::Owned<int, 2> I({2, 2}, {0, 3, 1, 2});
        ra::Owned<char, 1> B({4}, {'a', 'b', 'c', 'd'});
        cout << "B(I): " << B(I) << "\n\n";

        // multiple indexing performs an implicit outer product.
        // this results in a rank 4 array X = A(J, 1, J) -> X(i, j, k, l) = A(J(i, j), 1, J(k, l))
        ra::Owned<int, 2> J({2, 2}, {1, 0, 0, 1});
        cout << "A(J, 1, J): " << A(J, 1, J) << "\n\n";

        // explicit indices do not result in a Raw view (= pointer + strides), but the resulting
        // expression can still be written on.
        B(I) = ra::Owned<char, 2>({2, 2}, {'x', 'y', 'z', 'w'});
        cout << "B: " << B << endl;
    }
    // A TensorIndex object as in Blitz++ (with some differences).
    section("tensorindex");
    {
        // as shown above.
    }
    section("STL compat");
    {
        ra::Owned<char, 1> A = {'x', 'z', 'y'};
        std::sort(A.begin(), A.end());
        cout << "A: " << A << "\n\n";

        ra::Owned<float, 2> B({2, 2}, {1, 2, 3, 4});
        B += std::vector<float>({10, 20});
        cout << "B: " << B << "\n\n";
    }
    return 0;
}
