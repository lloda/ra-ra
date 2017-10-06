

# ra-ra ![(travis build status)](https://travis-ci.org/lloda/ra-ra.svg?branch=master) #

ra-ra is a C++ header-only, expression template / multidimensional array
library with an APL/J bent. Some C++17 features are used, see below for
compiler support.

ra-ra has a manual (work in progress) maintained at doc/ra-ra.texi. If
you are reading this on the web, there should be a version of that
manual in the 'Wiki'.


Features
-----------

These examples are necessarily terse. Please check the ```examples/``` folder.

* Dynamic or static array rank. Dynamic or static array shape.

```
ra::Owned<char> A({2, 3}, 'a');     // dynamic rank = 2, dynamic shape = {2, 3}
ra::Owned<char, 2> B({2, 3}, 'b');  // static rank = 2, dynamic shape = {2, 3}
ra::Small<char, 2, 3> C('c');       // static rank = 2, static shape = {2, 3}
cout << "A: " << A << "\n\n";
cout << "B: " << B << "\n\n";
cout << "C: " << C << "\n\n";

```

```
A: 2
2 3
a a a
a a a

B: 2 3
b b b
b b b

C: c c c
c c c
```

* Memory-owning types and views. You can make array views over any piece of
  memory.

```
// memory-owning types
ra::Owned<char, 2> A({2, 3}, 'a');   // storage is std::vector inside A
ra::Unique<char, 2> B({2, 3}, 'b');  // storage is owned by std::unique_ptr inside B
ra::Small<char, 2, 3> C('c');        // storage is owned by C itself, on the stack

// view types
char cs[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
ra::View<char, 2> D1({2, 3}, cs);            // dynamic sizes and strides, C order
ra::View<char, 2> D2({{2, 1}, {3, 2}}, cs);  // dynamic sizes and strides, Fortran order.
ra::SmallView<char, mp::int_list<2, 3>, mp::int_list<3, 1>> D3(cs); // static sizes & strides, C order.
ra::SmallView<char, mp::int_list<2, 3>, mp::int_list<1, 2>> D4(cs); // static sizes & strides, Fortran order.
```

* Shape agreement rules and rank extension (broadcasting) for rank-0 operations
  of any arity and operands of any rank, any of which can be a reference (so you
  can write on them). These rules are based on those of the array language, J.

```
ra::Owned<float, 2> A({2, 3}, { 1, 2, 3, 1, 2, 3 });
ra::Owned<float, 1> B({-1, +1});

ra::Owned<float, 2> C({2, 3}, 99.);
C = A * B;   // C(i, j) = A(i, j) * B(i)

ra::Owned<float, 1> D({2}, 0.);
D += A * B;  // D(i) += A(i, j) * B(i)
```
```
C: 2 3
-1 -2 -3
1 2 3

D: 2
-6 6
```

* A TensorIndex object as in Blitz++ (with some differences).

* Iterators over cells of arbitrary rank.

```
ra::TensorIndex<0> i;
ra::TensorIndex<1> j;
ra::TensorIndex<2> k;
ra::Owned<float, 3> A({2, 3, 4}, i+j+k);
ra::Owned<float, 2> B({2, 3}, 0);

// store the sum of A(i, j, ...) in B(i, j). These are equivalent.
for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, iter<1>(A));  // give cell rank
for_each([](auto && b, auto && a) { b = ra::sum(a); }, B, iter<-2>(A)); // give frame rank
```
```
A: 2 3 4
0 1 2 3
1 2 3 4
2 3 4 5

1 2 3 4
2 3 4 5
3 4 5 6

B: 2 3
6 10 14
10 14 18
```

* A rank conjunction (only for static rank and somewhat fragile).

```
// This is a direct translation of J: A = (i.3) -"(0 1) i.4, that is: A(i, j) = b(i)-c(j).
ra::Owned<float, 2> A = map(ra::wrank<0, 1>(std::minus<float>()), ra::iota(3), ra::iota(4));
cout << "A: " << A << "\n\n";
```
```
A: 3 4
0 -1 -2 -3
1 0 -1 -2
2 1 0 -1
```

* A proper selection operator with 'beating' of range or scalar subscripts. This
  means that when the operands of the selection are scalars or linear ranges,
  the selection is performed immediately by adjusting strides, instead of being
  delayed until the execution of the expression template.

```
ra::Owned<char, 3> A({2, 2, 2}, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'});
```
```
A: 2 2 2
a b
c d

e f
g h
```

```
// these are all equivalent to e.g. A(:, 0, :) in Octave.
cout << "A1: " << A(ra::all, 0) << "\n\n";
cout << "A2: " << A(ra::all, 0, ra::all) << "\n\n";
cout << "A3: " << A(ra::all, 0, ra::dots<1>) << "\n\n";
```
```
A3: 2 2
a b
e f
```

```
// an inverted range.
cout << "A4: " << A(ra::iota(2, 1, -1)) << "\n\n";
```
```
A4: 2 2 2
e f
g h

a b
c d
```

```
// indices can be arrays of any rank.
ra::Owned<int, 2> I({2, 2}, {0, 3, 1, 2});
ra::Owned<char, 1> B({4}, {'a', 'b', 'c', 'd'});
cout << "B(I): " << B(I) << "\n\n";
```
```
B(I): 2 2
a d
b c
```

```
// multiple indexing performs an implicit outer product, as in APL.
// this results in a rank 4 array X = A(J, 1, J) -> X(i, j, k, l) = A(J(i, j), 1, J(k, l))
ra::Owned<int, 2> J({2, 2}, {1, 0, 0, 1});
cout << "A(J, 1, J): " << A(J, 1, J) << "\n\n";
```
```
A(J, 1, J): 2 2 2 2
h g
g h

d c
c d


d c
c d

h g
g h
```

```
// explicit indices do not result in a View (= pointer + strides), but the resulting
// expression can still be written on.
B(I) = ra::Owned<char, 2>({2, 2}, {'x', 'y', 'z', 'w'});
cout << "B: " << B << endl;
```
```
B: 4
x z w y
```

* Some compatibility with the STL.

```
ra::Owned<char, 1> A = {'x', 'z', 'y'};
std::sort(A.begin(), A.end());

ra::Owned<float, 2> B({2, 2}, {1, 2, 3, 4});
B += std::vector<float>({10, 20});
```

```
A: 3
x y z

B: 2 2
11 12
23 24
```


Sui generis
-----------

* Index and size types are all signed.

* Index base is always 0.

* Default array order is C or row-major (last dimension changes fastest). You
  can make array views using other orders by transposing or manipulating the
  strides yourself, but all newly created arrays use C-order.

* The selection operator is (). [] means the same as () but only accepts one
  subscript.

* Array constructors follow a regular format. Single argument constructors
  always take an 'init-expression' which must provide enough shape information
  to construct the new array (unless the array type has static shape), and is
  otherwise subject to the regular argument shape agreement rules. Two-argument
  constructors always take a shape argument and a content argument.

* Indices are checked by default. This can be disabled with a compilation flag.


Bugs & wishes
-----------

* Should be namespace-clean.

* Beatable subscripts are not beaten if mixed with non-beatable subscripts.

* Better reduction mechanisms.

* Concatenation, search, reshape, and other infinite rank or rank>0 operations
  are missing.

* More clever/faster traversal of arrays, like in Blitz++.

* Systematic handling of nested arrays.


Out of scope
-----------

* GPU / parallelization / calls to external libraries.

* Linear algebra, quaternions, etc. Those things belong in other libraries. The
  library includes a dual number implementation but it's more of a demo of how
  to adapt user types to the library.

* Sparse arrays. You'd still want to mix & match with dense arrays, so
  maybe at some point.


Building
-----------

The library is header-only and has no dependencies other than a C++17 compiler
and the standard library. There is a test suite in ```test/```. These tests test
internal details and are not meant as demonstrations of how to use the
library. There is a directory with ```examples/```, some ported from Blitz++.

All tests pass under g++-7.2.

(OUTDATED) All tests pass under clang++-4.0 except for:

* test/bench-pack.C, crashes clang.

* test/test-optimize.C, fails to compile to a defect in the implementation of
  the vector_size attribute.

For clang on OS X you have to remove the -Wa,-q option in SConstruct which is
meant for gcc by setting CCFLAGS to something else:

  ```CCFLAGS="-march=native -DRA_OPTIMIZE_SMALLVECTOR=0" CXXFLAGS=-O3
  CXX=clang++-3.9 scons -j4```

I haven't tested on Windows.


Motivation
-----------

I do numerical work in C++ so I need a library of this kind. Most C++ array
libraries seem to support only vectors and matrices, or small objects for
low-dimensional vector algebra. Blitz++ was a great early array library and it
hasn't really been replaced as far as I can tell.

I've also been inspired by APL and J, which have led the way in exploring how
array operations could be generalized.

This is a simple library. I don't want to second-guess the compiler and I don't
stress performance as much as Blitz++ did. However, I am wary of adding features
that could become an obstacle if I ever tried to make things fast(er). I believe
that improvements such as new traversal methods or the optimization of specific
expression patterns should be easy to implement without turning the library
inside out.
