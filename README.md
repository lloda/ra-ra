
# ra-ra [![C/C++ CI](https://github.com/lloda/ra-ra/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/lloda/ra-ra/actions/workflows/c-cpp.yml)

**ra-ra** is a C++20, header-only multidimensional array library in the spirit of [Blitz++](http://blitz.sourceforge.net).

Multidimensional arrays can be indexed in multiple dimensions. For example, vectors can be seen as rank 1 and matrices are arrays of rank 2. C has built-in multidimensional array types, but even in modern C++ there's very little you can do with those, and anything practical requires a separate library.

**ra-ra** implements [expression templates](https://en.wikipedia.org/wiki/Expression_templates). This is a C++ technique (pioneered by Blitz++) to delay the execution of expressions involving large array operands, and in this way avoid the unnecessary creation of large temporary array objects.

**ra-ra** is small (about 6k loc), easy to extend, and generic — there are no arbitrary type restrictions or limits on rank or argument count.

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
* Compatibility with builtin arrays and with the standard library, including `<ranges>`.
* Interoperability with other libraries and/or languages through transparent memory layout.
* Iterators over cells (subarrays) of any rank.
* Rank conjunction as in J (compile time ranks only).
* Slicing with indices of arbitrary rank, beating of linear range indices, index skipping and elision.
* Outer product operation.
* Tensor index object.
* Short-circuiting logical operators.
* Argument list selection operators (`where` with bool selector, or `pick` with integer selector).
* Axis insertion (e.g. for broadcasting).
* Reshape, transpose, reverse, collapse/explode, stencils.
* Arbitrary types as array elements, or as scalar operands.
* Many predefined array operations. Adding yours is trivial.

`constexpr` is suported as much as possible. For example:

```
  constexpr ra::Small<int, 3> a = { 1, 2, 3 };
  static_assert(6==ra::sum(a));
```

Performance is competitive with hand written scalar (element by element) loops, but probably not with cache-tuned code such as your platform BLAS, or with code using SIMD. Please have a look at the benchmarks in [bench/](bench/).

#### Building the tests and the benchmarks

The library itself is header-only and has no dependencies other than a C++20 compiler and the standard library.

The test suite in [test/](test/) runs under either SCons (`CXXFLAGS=-O3 scons`) or CMake (`CXXFLAGS=-O3 cmake . && make && make test`). Running the test suite will also build and run the examples ([examples/](examples/)) and the benchmarks ([bench/](bench/)). You can also build each of these separately. The performance of **ra-ra** depends heavily on the optimization level, so although the test suite should pass with `-O0`, that can take a long time.

* Some of the benchmarks will try to use BLAS if you have define `RA_USE_BLAS=1` in the environment.
* The test suite is built with `-fsanitize=address` by default, and this can cause significant slowdown. Disable by passing `-fno-sanitize=address` to the compiler.

**ra-ra** requires support for `-std=c++20`, including `<source_location>`. The most recent versions tested are:

* gcc 13.1: `f632865f35fefe9e8cf00558cdc75e17f9fb9c2a` (`-std=c++2b`)
* gcc 12.2: `f632865f35fefe9e8cf00558cdc75e17f9fb9c2a` (`-std=c++2b`)
* gcc 11.3: `f632865f35fefe9e8cf00558cdc75e17f9fb9c2a` (`-std=c++20`)

It's not practical for me to test Clang systematically, so some snags with that are likely.

#### Notes

* Both index and size types are signed. Index base is 0.
* The default array order is C or row-major (last dimension changes fastest). You can make array views with other orders, but newly created arrays use C-order.
* The selection (subscripting) operator is `()` or `[]` indistinctly. Multi-argument `[]` requires `__cpp_multidimensional_subscript > 202110L` (in gcc 12 with `-std=c++2b`).
* Indices are checked by default. This can be disabled with a compilation flag.
* **ra-ra** doesn't itself use exceptions, but it provides a hook so you can throw your own exceptions on **ra-ra** errors. See ‘Error handling’ in the manual.
* **ra-ra** uses zero size arrays and VLAs internally. I know standard C++ doesn't allow them, which is irritating.

#### Bugs & defects

* Operations that require allocation, such as concatenation or search, are mostly absent.
* Lack of a good abstraction for reductions. You can write reductions abusing rank extension, but it's awkward.
* Traversal of arrays is naive – just unrolling of inner dimensions.
* Handling of nested (‘ragged’) arrays is inconsistent.
* No support for parallel operations or GPU.
* No SIMD to speak of.

Please see the TODO file for a concrete list of known bugs.

#### Out of scope

* Sparse arrays.
* Linear algebra, quaternions, etc.

#### Motivation

I do numerical work in C++, so I need a library of this kind. At the time of C++11 when I started writing it, most C++ array libraries seemed to support only vectors and matrices, or small objects for vector algebra.

Blitz++ was a major inspiration as a *generic* library. But it was a heroic feat to write such a library in C++ in the late 90s. Variadic templates, lambdas, perfect forwarding, etc. make things *much* easier, for the library writer as well as for the user.

From APL and J I've taken the rank extension mechanism, and perhaps an inclination for carrying each feature to its logical end.

**ra-ra** wants to remain simple. I try not to second-guess the compiler and I don't stress performance as much as Blitz++ did. However, I'm wary of adding features that could become an obstacle if I ever tried to make things fast(er). I believe that the implementation of new traversal methods, or perhaps the optimization of specific expression patterns, should be possible without having to turn the library inside out.

#### Other C++ array libraries

* [Blitz++](http://www.oonumerics.org/blitz/manual/blitz.html)
* [Eigen](https://eigen.tuxfamily.org)
* [Boost.MultiArray](www.boost.org/doc/libs/master/libs/multi_array/doc/user.html)
* [xtensor](https://github.com/QuantStack/xtensor)
* [Adept](http://www.met.reading.ac.uk/clouds/adept/download.html)
* [Towards a standard for a C++ multi-dimensional array library for scientific applications](http://www.met.reading.ac.uk/clouds/cpp_arrays/) Reviews a number of C++ array libraries, including **ra-ra** (2020-08).

#### Links

* [libsimdpp](https://github.com/p12tic/libsimdpp) C++ SIMD library
* [J for C programmers](http://www.jsoftware.com/help/jforc/contents.htm)
* [GNU APL](https://www.gnu.org/software/apl/)
* [Fortran wiki](http://fortranwiki.org/fortran/show/diff/HomePage)
* [Numpy](https://numpy.org/)
* [Octave](https://www.gnu.org/software/octave/)
