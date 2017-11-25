
# ra-ra ![(travis build status)](https://travis-ci.org/lloda/ra-ra.svg?branch=master) #

**ra-ra** is a C++ header-only multidimensional array and expression template
library in the spirit of [Blitz++](http://blitz.sourceforge.net). Most of the code is
C++14, but some C++17 features are used.

Multidimensional arrays are containers that are indexable in zero or more
dimensions. For example, vectors are arrays of rank 1 and matrices are arrays of
rank 2. C has had built-in multidimensional array types since forever, but even
in C++17 there's very little you can do with those, and a separate library is
required for any practical endeavor.

[Expression templates](https://en.wikipedia.org/wiki/Expression_templates) are a
C++ technique (pioneered by Blitz++) to delay the execution of expressions
involving large array operands, and in this way avoid the unnecessary creation
of large temporary array objects.

**ra-ra** tries to distinguish itself from established C++ libraries in this
space (such as [Eigen](https://eigen.tuxfamily.org) or
[Boost.MultiArray](www.boost.org/doc/libs/master/libs/multi_array/doc/user.html))
by being more APLish, more general, smaller, and more hackable.

In this example from [examples/readme.C](examples/readme.C), we add each element
of a vector to each row of a matrix, and then print the result.

```c++
#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

int main()
{
  ra::Big<float, 2> A({2, 2}, {1, 2, 3, 4});  // A = [1 2; 3 4], dynamic shape, compile-time rank
  A += std::vector<float>({10, 20});          // rank-extending op with STL object
  std::cout << "A: " << A << "\n\n";          // shape is dynamic, so it will be printed
  return 0;
}
```
⇒
```
A: 2 2
11 12
23 24
```

**ra-ra** supports:

* Array types with arbitrary compile time or runtime rank.
* Array types with compile time or runtime shape.
* Memory owning types as well as views, with all the rank and shape options above. You can make array views over any piece of memory.
* Transparent memory layout, for interoperability with other libraries and/or languages.
* Rank extension (broadcasting) for functions with any number of arguments of any rank.
* Slicing with indices of arbitrary rank, beating of linear range indices, index skipping and elision, and more.
* A rank conjunction as in J, with some limitations.
* An outer product operation.
* Iterators over slices (subarrays) of any rank.
* A tensor index object, with some limitations.
* Arbitrary types as array elements, or as scalar operands.
* Many predefined array operations. Adding yours is trivial.
* Lazy selection operators (e.g. pick from argument list according to index).
* Short-circuiting logical operators.
* Reshape, transpose, reverse, collapse/explode, stencils…
* Partial compatibility with the STL.

**ra-ra** has a manual (work in progress) maintained at
[doc/ra-ra.texi](doc/ra-ra.texi). You can view the manual online at
[lloda.github.io/ra-ra](https://lloda.github.io/ra-ra). Please check it out for
details, or have a look at the [examples/](examples/) folder.

Performance is competitive with hand written scalar (element by element) loops,
but probably not with cache-tuned code such as your platform BLAS, or with code
using SIMD. Please have a look at the benchmarks in [bench/](bench/).

#### Building the tests and the benchmarks

The library itself is header-only and has no dependencies other than a C++17 compiler
and the standard library.

The test suite ([test/](test/)) runs under SCons. Running the test suite will
also build and run the examples ([examples/](examples/)) and the benchmarks
([bench/](bench/)), although you can easily build each of these separately. None
of them has any dependencies, but some of the benchmarks will try to use BLAS if
you have `RA_USE_BLAS=1` in the environment.

All the tests pass under g++-7.2. Remember to pass `-O2` or `-O3` to the compiler,
otherwise some of the tests will take a very long time to run.

All the tests pass under clang++-5.0 (with `-Wno-missing-braces`) except for:

* [test/bench-pack.C](test/bench-pack.C), crashes clang.
* [test/test-optimize.C](test/test-optimize.C), a required specialization is missed and I haven't
  figured out why.

For clang on OS X you have to remove the `-Wa,-q` option in SConstruct which is
meant for gcc by setting CCFLAGS to something else, say:

  ```
  CCFLAGS="-march=native -Wno-missing-braces -DRA_OPTIMIZE_SMALLVECTOR=0" CXXFLAGS=-O3 CXX=clang++-5.0 scons -j4
  ```

I haven't tested on Windows. If you can do that, I'd appreciate a report!

#### Notes

* Index and size types are all signed. Index base is always 0.
* Default array order is C or row-major (last dimension changes fastest). You
  can make array views using other orders by transposing or manipulating the
  strides yourself, but newly created arrays use C-order.
* The selection (subscripting) operator is `()`. `[]` means exactly the same as `()`, except that it accepts one
  subscript only.
* Array constructors follow a regular format:
  - Single argument constructors take a ‘content’ argument which must provide
    enough shape information to construct the new array. If the array type
    has static shape, then the argument is subjected to the regular
    argument shape agreement rules (rank extension rules).
  - Two-argument constructors always take a shape argument and a content argument.
* Indices are checked by default. This can be disabled with a compilation flag.


#### Bugs & defects

* Not completely namespace-clean.
* Beatable subscripts are not beaten if mixed with non-beatable subscripts.
* Inconsistencies with subscripting; for example, if `A` is rank>1 and
  `i` is rank 1, then `A(i)` will return a nested expression instead of
  preserving `A`'s rank.
* Poor reduction mechanisms.
* Missing concatenation, search, and other infinite rank or rank > 0 operations.
* Traversal of arrays is naive.
* Poor handling of nested arrays.
* No SIMD.


#### Out of scope

* Parallelization (closer to wish...).
* GPU / calls to external libraries.
* Linear algebra, quaternions, etc. Those things belong in other libraries. The
  library includes a dual number implementation but it's more of a demo of how
  to adapt user types to the library.
* Sparse arrays. You'd still want to mix & match with dense arrays, so maybe at
  some point.


#### Motivation

I do numerical work in C++, so I need a library of this kind. Most C++ array
libraries seem to support only vectors and matrices, or small objects for
low-dimensional vector algebra. Blitz++ was a great early *generic* array
library (even though the focus was numerical) and it hasn't really been replaced
as far as I can tell.

It was a heroic feat to write a library such as Blitz++ in C++ in the late 90s,
even discounting the fragmented compiler landscape and the patchy support for
the standard at that time. Variadic templates, lambdas, rvalue arguments,
etc. make things *much* simpler, for the library writer as well as for the user.

From APL and J I've taken the rank extension mechanism, and perhaps an
inclination for carrying each feature to its logical end.

**ra-ra** wants to remain a simple library. I try not to second-guess the compiler and I
don't stress performance as much as Blitz++ did. However, I'm wary of adding
features that could become an obstacle if I ever tried to make things
fast(er). I believe that the implementation of new traversal methods, or perhaps
the optimization of specific expression patterns, should be possible without
having to turn the library inside out.
