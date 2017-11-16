

# ra-ra ![(travis build status)](https://travis-ci.org/lloda/ra-ra.svg?branch=master) #

**ra-ra** is a C++ header-only multidimensional array and expression template
library in the spirit of [Blitz++](blitz.sourceforge.net). Most of the code is
C++14, but some C++17 features are used.

Multidimensional arrays are a type of container that is indexable in zero or
more dimensions. For example, vectors are arrays of rank 1 and matrices are
arrays of rank 2. Even though C has had multidimensional array types since
early on, standard support for arrays of rank greater than 1 is still near
non-existent in C++17, and a library is required for any practical endeavor.

[Expression templates](https://en.wikipedia.org/wiki/Expression_templates) are a
C++ technique to delay the execution of expressions involving (large) array
operands, and in this way avoid the unnecessary creation of large temporary
array objects. It was pioneered by Blitz++, although the essential idea is at
least as old as Abrams' 1970 thesis, *An APL machine*.

**ra-ra** tries to distinguish itself from established C++ libraries in this
space (such as [Eigen](eigen.tuxfamily.org) or
[Boost.MultiArray](www.boost.org/doc/libs/master/libs/multi_array/doc/user.html))
by being more APLish, more general, smaller, and more hackable.

This is a standalone example from `examples/readme.C`:

```c++
#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

int main()
{
  ra::Owned<float, 2> A({2, 2}, {1, 2, 3, 4});   // A = [1, 2; 3 4], dynamic dimensions, compile-time rank
  A += std::vector<float>({10, 20});             // broadcast op with STL object
  std::cout << "A: " << A << "\n\n";             // dimensions are dynamic, so they'll be printed
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

* Array types with compile time or runtime rank. Either can be arbitrarily large.
* Array types with compile time or runtime dimensions.
* Memory owning types as well as views, with all the rank and dimension options above. You can make array views over any piece of memory.
* Transparent memory layout, for interoperability with other libraries and/or languages.
* Rank extension (broadcasting) for functions with any number of arguments of any rank.
* Slicing with indices of arbitrary rank, beating of linear range indices, index skipping and elision, and more.
* An outer product operation.
* A rank conjunction as in J, with some limitations.
* Iterators over slices (subarrays) of any rank.
* A tensor index object, with some limitations.
* Stencil operations.
* Arbitrary types as array elements, or as scalar operands.
* Lazy selection operators (e.g. pick from argument list according to index). Short-circuiting logical operators.
* Partial compatibility with the STL.
* Many predefined array operations. Adding yours is trivial.
* etc.

**ra-ra** has a manual (work in progress) maintained at `doc/ra-ra.texi`. If you
are reading this on the web, there should be a version of that manual in the
‘wiki’. Please check it out for details, or have a look at the `examples/`
folder.

Performance is competitive with hand written scalar (element by
element) loops, but not with cache-tuned code such as your platform BLAS, or
with code using SIMD. Please have a look at the `benchmarks/`.

#### Building the tests and the benchmarks

The library itself is header-only and has no dependencies other than a C++17 compiler
and the standard library.

To run the test suite (```test/```) you need Scons. There is a `Makefile`, but
it will just try to run SCons. Running the test suite will also build and run
the examples (```examples/```) and the benchmarks (```bench/```), although you
can easily build each of these separately. None of them has any dependencies,
but some of the benchmarks will try to use BLAS if you have ```RA_USE_BLAS=1```
in the environment.

All the tests pass under g++-7.2. Remember to pass `-O2` or `-O3` to the compiler,
otherwise some of the tests will take a very long time to run.

All the tests pass under clang++-5.0 (with `-Wno-missing-braces`) except for:

* test/bench-pack.C, crashes clang.
* test/test-optimize.C, a required specialization is missed and I haven't
  figured out why.

For clang on OS X you have to remove the `-Wa,-q` option in SConstruct which is
meant for gcc by setting CCFLAGS to something else:

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


#### Bugs & wishes

* Should be namespace-clean.
* Beatable subscripts are not beaten if mixed with non-beatable subscripts.
* Some inconsistencies with subscripting; for example, if ```A``` is rank>1 and
  ```i``` is rank 1, then ```A(i)``` will return a nested expression instead of
  preserving ```A```'s rank.
* Better reduction mechanisms.
* Better concatenation, search, and other infinite rank or rank>0
  operations.
* More clever/faster traversal of arrays, like in Blitz++.
* Systematic handling of nested arrays.
* SIMD?


#### Out of scope

* Parallelization (closer to wish...).
* GPU /  / calls to external libraries.
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
