
# ra-ra [![C/C++ CI](https://github.com/lloda/ra-ra/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/lloda/ra-ra/actions/workflows/c-cpp.yml)

**ra-ra** is a C++20, header-only multidimensional array library in the spirit of [Blitz++](http://blitz.sourceforge.net).

Multidimensional arrays are containers that can be indexed in multiple dimensions. For example, vectors are arrays of rank 1 and matrices are arrays of rank 2. C has built-in multidimensional array types, but even in modern C++ there's very little you can do with those, and a separate library is required for any practical endeavor.

**ra-ra** implements [expression templates](https://en.wikipedia.org/wiki/Expression_templates). This is a C++ technique (pioneered by Blitz++) to delay the execution of expressions involving large array operands, and in this way avoid the unnecessary creation of large temporary array objects.

**ra-ra** tries to distinguish itself from established C++ libraries in this space (such as [Eigen](https://eigen.tuxfamily.org) or [Boost.MultiArray](www.boost.org/doc/libs/master/libs/multi_array/doc/user.html)) by being more APLish, more general, smaller, and more hackable.

In this example ([examples/readme.cc](examples/readme.cc)), we add each element of a vector to each row of a matrix, and then print the result.

```c++
#include "ra/ra.hh"
#include <iostream>

int main()
{
  ra::Big<float, 2> A {{1, 2}, {3, 4}};  // compile-time rank, dynamic shape
  A += std::vector<float> {10, 20};      // rank-extending op with STL object
  std::cout << "A: " << A << std::endl;  // shape is dynamic, so it will be printed
}
```
⇒
```
A: 2 2
11 12
23 24
```

Please check the manual online at [lloda.github.io/ra-ra](https://lloda.github.io/ra-ra), or have a look at the [examples/](examples/) folder.

**ra-ra** offers:

* Array types with arbitrary compile time or runtime rank, and compile time or runtime shape.
* Memory owning types as well as views over any piece of memory.
* Rank extension by prefix matching, as in APL/J, for functions of any number of arguments.
* Compatibility with builtin arrays and with the STL.
* Transparent memory layout, for interoperability with other libraries and/or languages.
* Iterators over cells (slices/subarrays) of any rank.
* Rank conjunction as in J, with some limitations.
* Slicing with indices of arbitrary rank, beating of linear range indices, index skipping and elision.
* Outer product operation.
* Tensor index object.
* Short-circuiting logical operators.
* Argument list selection operators (`where` with bool selector, or `pick` with integer selector).
* Axis insertion (e.g. for broadcasting).
* Reshape, transpose, reverse, collapse/explode, stencils.
* Arbitrary types as array elements, or as scalar operands.
* Many predefined array operations. Adding yours is trivial.

There is some `constexpr` support for the compile time size types. For example, this works:

```
constexpr ra::Small<int, 3> a = { 1, 2, 3 };
using T = std::integral_constant<int, ra::sum(a)>;
static_assert(T::value==6);
```

Performance is competitive with hand written scalar (element by element) loops, but probably not with cache-tuned code such as your platform BLAS, or with code using SIMD. Please have a look at the benchmarks in [bench/](bench/).

#### Building the tests and the benchmarks

The library itself is header-only and has no dependencies other than a C++20 compiler and the standard library.

The test suite in [test/](test/) runs under either SCons (`CXXFLAGS=-O3 scons`) or CMake (`CXXFLAGS=-O3 cmake . && make && make test`). Running the test suite will also build and run the examples ([examples/](examples/)) and the benchmarks ([bench/](bench/)), although you can build each of these separately. None of them has any dependencies, but some of the benchmarks will try to use BLAS if you have `RA_USE_BLAS=1` in the environment.

**ra-ra** requires support for `-std=c++20` including `<source_location>`. The most recent versions tested are:

* gcc 12.2: `993ff15def0b60c151c63cadcdef7e9088ea1fee`
* gcc 11.3: `ecc0a1ac47f22588f8aca1139b7a9ed5836b4561`

Remember to pass `-O2` or `-O3` to the compiler, otherwise some of the tests will take a very long time to run. Clang 10 doesn't currently work (I'll keep trying) but the code is meant to be standard C++.

<!-- All the tests pass under clang++-7.0 [trunk 322817, tested on Linux] except for: -->

<!-- * [bench/bench-pack.cc](bench/bench-pack.cc), crashes clang. -->
<!-- * [test/iterator-small.cc](test/iterator-small.cc), crashes clang. -->
<!-- * [test/optimize.cc](test/optimize.cc), gives compilation errors. -->

<!-- For clang on OS X you have to remove the `-Wa,-q` option in SConstruct which is meant for gcc by setting CCFLAGS to something else, say: -->

<!--   ``` -->
<!--   CCFLAGS="-march=native" CXXFLAGS=-O3 CXX=clang++ scons -j4 -->
<!--   ``` -->

<!-- I haven't tested on Windows. If you can do that, I'd appreciate a report! -->

#### Notes

* Both index and size types are signed. Index base is 0.
* Default array order is C or row-major (last dimension changes fastest). You can make array views with other orders, but newly created arrays use C-order.
* The selection (subscripting) operator is `()`. `[]` means exactly the same as `()`. It's unfortunate that `[]` was wasted on subscripting when `()` works perfectly well for that...
* Indices are checked by default. This can be disabled with a compilation flag.
* **ra-ra** doesn't itself use exceptions, but it provides a hook so you can throw your own exceptions on **ra-ra** errors. See ‘Error handling’ in the manual.

#### Bugs & defects

* Lack of good reduction mechanisms.
* Operations that require allocation, such as concatenation or search, are mostly absent.
* Traversal of arrays is naive (just unrolling of inner dimensions).
* Handling of nested (‘ragged’) arrays is inconsistent.
* No SIMD to speak of.

Please have a look at TODO for a concrete list of known bugs.

#### Out of scope

* Parallelization (closer to wish...).
* GPU / calls to external libraries.
* Linear algebra, quaternions, etc. Those things belong in other libraries, and calling them with **ra-ra** objects is trivial.
* Sparse arrays. You'd still want to mix & match with dense arrays, so maybe at some point.

#### Motivation

I do numerical work in C++, so I need a library of this kind. Most C++ array libraries seem to support only vectors and matrices, or small objects for low-dimensional vector algebra. Blitz++ was a great early *generic* array library (even though the focus was numerical) and it hasn't really been replaced as far as I can tell.

It was a heroic feat to write a library such as Blitz++ in C++ in the late 90s, even discounting the fragmented compiler landscape and the patchy support for the standard at that time. Variadic templates, lambdas, rvalue arguments, etc. make things *much* simpler, for the library writer as well as for the user.

From APL and J I've taken the rank extension mechanism, and perhaps an inclination for carrying each feature to its logical end.

**ra-ra** wants to remain a simple library. I try not to second-guess the compiler and I don't stress performance as much as Blitz++ did. However, I'm wary of adding features that could become an obstacle if I ever tried to make things fast(er). I believe that the implementation of new traversal methods, or perhaps the optimization of specific expression patterns, should be possible without having to turn the library inside out.

#### Other C++ array libraries

* [Blitz++](http://www.oonumerics.org/blitz/manual/blitz.html)
* [Eigen](https://eigen.tuxfamily.org)
* [Boost.MultiArray](www.boost.org/doc/libs/master/libs/multi_array/doc/user.html)
* [xtensor](https://github.com/QuantStack/xtensor)
* [Towards a standard for a C++ multi-dimensional array library for scientific applications](http://www.met.reading.ac.uk/clouds/cpp_arrays/) Reviews a number of C++ array libraries, including **ra-ra** (2020-08).

#### Links

* [libsimdpp](https://github.com/p12tic/libsimdpp) C++ SIMD library
* [J for C programmers](http://www.jsoftware.com/help/jforc/contents.htm)
* [GNU APL](https://www.gnu.org/software/apl/)
* [Fortran wiki](http://fortranwiki.org/fortran/show/diff/HomePage)
* [Numpy](https://numpy.org/)
* [Octave](https://www.gnu.org/software/octave/)
