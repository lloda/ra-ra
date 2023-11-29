
[![C/C++ CI](https://github.com/lloda/ra-ra/actions/workflows/gcc11.yml/badge.svg)](https://github.com/lloda/ra-ra/actions/workflows/gcc11.yml) [![C/C++ CI](https://github.com/lloda/ra-ra/actions/workflows/gcc11-no-sanitize.yml/badge.svg)](https://github.com/lloda/ra-ra/actions/workflows/gcc11-no-sanitize.yml)

# ra-ra

**ra-ra** is a C++20 header-only library for handling multidimensional dense arrays. These are objects that can be indexed in 0 or more dimensions; the number of dimensions is known as ‘rank’. For example, vectors are arrays of rank 1 and matrices are arrays of rank 2.

**ra-ra** implements [expression templates](https://en.wikipedia.org/wiki/Expression_templates). This is a C++ technique (pioneered by [Blitz++](http://blitz.sourceforge.net)) to delay the execution of expressions involving array operands, and in this way avoid the unnecessary creation of large temporary array objects.

**ra-ra** is compact (~5k loc), easy to extend, and generic. There are no arbitrary type restrictions or limits on rank or argument count.

In this example ([examples/read-me.cc](examples/read-me.cc)), we create some arrays, do operations on them, and print the result.

```c++
  #include "ra/ra.hh"
  #include <iostream>

  int main()
  {
    // run time rank
    ra::Big<float> A = { {1, 2, 3, 4}, {5, 6, 7, 8} };
    // static rank, run time dimensions
    ra::Big<float, 2> B = { {1, 2, 3, 4}, {5, 6, 7, 8} };
    // static dimensions
    ra::Small<float, 2, 4> C = { {1, 2, 3, 4}, {5, 6, 7, 8} };
    // rank-extending op with STL object
    B += A + C + std::vector {100., 200.};
    // negate right half
    B(ra::all, ra::iota(ra::len/2, ra::len/2)) *= -1;
    // shape is dynamic, so will be printed
    std::cout << "B: " << B << std::endl;
  }
```
⇒
```
B: 2 4
103 106 -109 -112
215 218 -221 -224
```

Please check the manual online at [lloda.github.io/ra-ra](https://lloda.github.io/ra-ra), or have a look at the [examples/](examples/) folder.

**ra-ra** offers:

* Array types with arbitrary compile time or runtime rank and shape.
* Memory owning types as well as views over any piece of memory.
* Compatibility with builtin arrays and with the standard library, including ranges.
* Interoperability with other libraries and languages through transparent memory layout.
* Slicing with indices of arbitrary rank, axis skipping and insertion (e.g. for broadcasting), and contextual `len`.
* Rank extension by prefix matching, as in APL/J, for functions of any number of arguments.
* Iterators over subarrays (cells) of any rank.
* Rank conjunction as in J (compile time rank only), outer product operation.
* Short-circuiting logical operators.
* Argument list selection operators (`where` with bool selector, `pick` with integer selector).
* Reshape, transpose, reverse, collapse/explode, stencils.
* Arbitrary types as array elements, or as scalar operands.
* Many predefined array operations. Adding yours is trivial.
* Configurable error checking.
* Mostly `constexpr`.

Performance is competitive with hand written scalar (element by element) loops, but probably not with cache-tuned code such as your platform BLAS, or with code using SIMD. Please have a look at the benchmarks in [bench/](bench/).

#### Building the tests and the benchmarks

**ra-ra** is header-only and has no dependencies other than a C++20 compiler and the standard library. I test regularly with gcc ≥ 11.3. If you can test with Clang, please let me know.

The test suite in [test/](test/) runs under either SCons (`CXXFLAGS=-O3 scons`) or CMake (`CXXFLAGS=-O3 cmake . && make && make test`). Running the test suite will also build and run the [examples](examples/) and the [benchmarks](bench/).

* Some of the benchmarks will try to use BLAS if you have define `RA_USE_BLAS=1` in the environment.
* The test suite is built with `-fsanitize=address` by default, and this can cause significant slowdown. Disable by adding `-fno-sanitize=address` to `CXXFLAGS` at build time.
* The performance of **ra-ra** depends heavily on optimization level. The test suite should pass with `-O0`, but that can take a long time.

#### Notes

* Both index and size types are signed. Index base is 0.
* The default array order is C or row-major (last dimension changes fastest). You can make array views with other orders, but newly created arrays use C order.
* The subscripting operator is `()` or `[]` indistinctly. Multi-argument `[]` requires `__cpp_multidimensional_subscript > 202110L` (in gcc 12 with `-std=c++2b`).
* Indices are checked by default. This can be disabled with a compilation flag.
* **ra-ra** doesn't use exceptions, but it provides a hook so you can throw your own exceptions on **ra-ra** errors. See ‘Error handling’ in the manual.
* **ra-ra** uses zero size arrays and VLAs internally.

#### Bugs & defects

* Operations that require allocation, such as concatenation or search, are mostly absent.
* No good abstraction for reductions. You can write reductions abusing rank extension, but it's awkward.
* Traversal of arrays is basic, just unrolling of inner dimensions.
* Handling of nested / ragged arrays is inconsistent.
* No support for parallel operations or GPU.
* No SIMD to speak of.

Please see the TODO file for a concrete list of known issues.

#### Motivation

I do numerical work in C++, and I need support for array operations. The built-in array types that C++ inherits from C are very insufficient, so at the time of C++11 when I started writing **ra-ra**, a number of libraries where already available. However, most of these libraries seemed to support only vectors and matrices, or small objects for vector algebra.

Blitz++ was a major inspiration as an early *generic* library. But it was a heroic feat to write such a library in C++ in the late 90s. Variadic templates, lambdas, perfect forwarding, etc. make things much easier, for the library writer as well as for the user.

From APL and J I've taken the rank extension mechanism, and perhaps an inclination for carrying each feature to its logical end.

**ra-ra** wants to remain simple. I try not to second-guess the compiler and I don't stress performance as much as Blitz++ did. However, I'm wary of adding features that could become an obstacle if I ever tried to make things fast(er). I believe that implementating new traversal methods, or perhaps optimizing specific expression patterns, should be possible without having to turn the library inside out.

#### Other C++ array libraries

* [Blitz++](http://www.oonumerics.org/blitz/manual/blitz.html)
* [Eigen](https://eigen.tuxfamily.org)
* [Boost.MultiArray](www.boost.org/doc/libs/master/libs/multi_array/doc/user.html)
* [xtensor](https://github.com/QuantStack/xtensor)
* [Adept](http://www.met.reading.ac.uk/clouds/adept/download.html)

#### Links

* [Towards a standard for a C++ multi-dimensional array library for scientific applications](http://www.met.reading.ac.uk/clouds/cpp_arrays/) Reviews C++ array libraries, including **ra-ra** (2020-08)
* [libsimdpp](https://github.com/p12tic/libsimdpp) C++ SIMD library
* [J for C programmers](http://www.jsoftware.com/help/jforc/contents.htm) Array programming concepts
* [GNU APL](https://www.gnu.org/software/apl/)
* [Fortran wiki](http://fortranwiki.org/fortran/show/diff/HomePage)
* [Numpy](https://numpy.org/)
* [Octave](https://www.gnu.org/software/octave/)
