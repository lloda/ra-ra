
[![C/C++ CI](https://github.com/lloda/ra-ra/actions/workflows/gcc14.yml/badge.svg)](https://github.com/lloda/ra-ra/actions/workflows/gcc14.yml) [![C/C++ CI](https://github.com/lloda/ra-ra/actions/workflows/gcc14-no-sanitize.yml/badge.svg)](https://github.com/lloda/ra-ra/actions/workflows/gcc14-no-sanitize.yml)

# ra-ra

**ra-ra** is a C++23 header-only library for handling multidimensional dense arrays. These are objects that can be indexed in 0 or more dimensions; the number of dimensions is known as ‘rank’. For example, vectors are arrays of rank 1 and matrices are arrays of rank 2.

**ra-ra** implements [expression templates](https://en.wikipedia.org/wiki/Expression_templates). This is a C++ technique (pioneered by [Blitz++](http://blitz.sourceforge.net)) to delay the execution of expressions involving array operands, and in this way avoid the unnecessary creation of large temporary array objects.

**ra-ra** is compact, generic, and easy to extend.

In this example ([examples/read-me.cc](examples/read-me.cc)), we create some arrays, operate on them, and print the result.

```c++
  #include "ra/ra.hh"
  #include <cstdio>

  int main()
  {
    // run time rank
    ra::Big<float> A = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    // static rank, run time dimensions
    ra::Big<float, 2> B = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    // static dimensions
    ra::Small<float, 2, 4> C = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    // rank-extending op with STL object
    B += A + C + std::vector {100., 200.};
    // negate right half
    B(ra::all, ra::iota(ra::len/2, ra::len/2)) *= -1;
    // print c style
    std::println(stdout, "B:\n{:c:4.2f}", B);
  }
```
⇒
```
B:
{{103.00, 106.00, -109.00, -112.00},
 {215.00, 218.00, -221.00, -224.00}}
```

Check the manual at [lloda.github.io/ra-ra](https://lloda.github.io/ra-ra), or have a look at the [examples/](examples/).

**ra-ra** offers:

* array types with arbitrary compile time or runtime rank and shape
* memory owning types, views over memory, sequence views
* compatibility with built-in arrays and with the standard library, including ranges, streams, and `<format>`
* generalized slicing with indices of arbitrary rank and contextual `len`
* rank extension by prefix matching, as in APL/J, for functions of any number of arguments
* iterators over subarrays (cells) of any rank
* rank conjunction as in J (compile time rank only), outer product operation
* short-circuiting logical operators
* operators to select from argument list (`where`, `pick`)
* view operations: reshape, transpose, reverse, collapse/explode, stencils
* arbitrary types as array elements, or as scalar operands
* many predefined array operations; adding yours is trivial
* configurable error handling
* as much `constexpr` as possible.

Performance is competitive with hand written scalar (element by element) loops, but probably not with cache-tuned code such as your platform BLAS, or with code using SIMD. Have a look at the benchmarks in [bench/](bench/).

#### Building the tests and the benchmarks

**ra-ra** is header-only and only depends on the standard library. A C++23 compiler is required. At the moment I test with gcc 14. If you can test with Clang, please let me know.

The test suite in [test/](test/) runs under either SCons (`CXXFLAGS=-O3 scons`) or CMake (`CXXFLAGS=-O3 cmake . && make && make test`). Running the test suite will also build and run the [examples](examples/) and the [benchmarks](bench/). Check the manual for options.

#### Notes

* Both index and size types are signed. Index base is 0.
* The default array order is C or row-major (last dimension changes fastest). You can make array views with other orders.
* The subscripting operator is `()` or `[]`, indistinctly.
* Indices are checked by default. Checks can be disabled with a compilation flag.

#### Bugs & defects

* Operations that require allocation, such as concatenation or search, are mostly absent.
* No good abstraction for reductions. You can write reductions abusing rank extension, but it's awkward.
* Basic array traversal, just unrolling of inner dimensions.
* Handling of nested / ragged arrays is inconsistent.
* No parallel operations, GPU, or SIMD (but should be thread safe).

#### Motivation

I do numerical work in C++, and I need support for array operations. The built-in array types that C++ inherits from C are insufficient, so at the time of C++11 when I started writing **ra-ra**, a number of libraries were already available. However, most of these libraries seemed to support only vectors and matrices, or small objects for vector algebra.

Blitz++ was a major inspiration as an early *generic* library. But it was a heroic feat to write such a library in C++ in the late 90s. Variadic templates, lambdas, perfect forwarding, etc. make things much easier, for the library writer as well as for the user.

From APL and J I've taken the rank extension mechanism, as well as an inclination for carrying each feature to its logical end.

I want **ra-ra** to remain simple. I try not to second-guess the compiler and I don't fret over performance as much as Blitz++ did. However, I'll avoid features that could become an obstacle if I ever tried to make things fast(er). It should be possible to implement new traversal methods, or perhaps optimize specific expression patterns, without having to turn the library inside out.

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
