
# ra-ra #

ra-ra is an expression template / multidimensional array library for C++14. It
is a work in progress.


Features
-----------

* Dynamic or static array rank. Dynamic or static array shape (all dimensions or
  none, alas).

* Both memory-owning types and views. You can make array views over any piece of
  memory.

* Shape agreement rules and rank extension for rank-0 operations of any arity
  and operands of any rank, any of which can a reference (so you can write on
  them). These rules are taken from the array language, J.

* Iterators over cells of arbitrary rank.

* A rank conjunction (only for static rank operands and somewhat fragile).

* A proper selection operator with 'beating' of range or scalar
  subscripts. Beatable subscripts are not beaten if mixed with non-beatable
  subscripts, which is a defect.

* A TensorIndex object as in Blitz++ (doesn't work in the same exact way).

* Some compatibility with the STL.


Sui generis
-----------

* Index and size types are all signed.

* Index base is always 0.

* Default array order is C or row-major (last dimension changes fastest). You
  can make array views using other orders by transposing or manipulating the
  strides yourself, but all newly created arrays use C-order.

* The selection operator is (). [] is supported for rank-1 arrays only, where it
  means the same as ().

* Array constructors have a very regularized format. Single argument
  constructors always take an 'init-expression' which must provide enough shape
  information to construct the new array (unless the array type has static
  shape), and is otherwise subject to the regular argument shape agreement
  rules. Two argument constructors always have a shape argument and an content
  argument.

* Indices are checked by default and can be disabled with a compilation flag.

Bugs & wishes
-----------

* Be namespace-clean.

* Proper reductions. There are some reduction operations but no general
  mechanism. This may be the most obvious feature hole.

* Concatenation, search, reshape, and other infinite rank or rank>0 operations.

* Stencils, like in Blitz++.

* More clever/faster traversal of arrays, like in Blitz++.

* Handling of nested arrays; there's no real support for this.


Out of scope
-----------

* No GPU / parallelization / calls to external libraries.

* Linear algebra, quaternions, etc. Those things belong in other libraries. The
  library includes a dual number implementation but it's more of a demo of how
  to adapt user types to the library.

* Sparse arrays. You'd still want to mix & match with dense arrays, so
  maybe at some point.


Building
-----------

The library is header-only and has no dependencies beyond a C++14 compiler and
the standard C++ library. There is a test suite in test/. These tests test
internal details and are not meant as demonstrations of how to use the
library. There is a directory with examples/, mostly ported from Blitz++.

I have tested with g++-5.3 and clang++-3.8. For clang on OS X you have to remove
the -Wa,q option in the SConstruct which is meant for gcc by setting CCFLAGS to
something else, for example:

  ```CCFLAGS="-march=native" CXXFLAGS=-O3 CXX=clang++-3.8 scons -j4```

I haven't tested on Windows.

test-wedge-product.C and test-ra-operators.C fail to compile under
clang++-3.8. I hope to fix these issues as soon as possible.


Motivation
-----------

I do numerical work in C++ so I need a library of this kind. Most C++ array
libraries seem to support only vectors and matrices, or small objects for
low-dimensional vector algebra. Blitz++ was a great early array library and it
hasn't really been replaced as far as I can tell.

My other inspirations are APL and J, which have gone further than most in
exploring how array operations could or should be generalized.

This is a simple library. I tend to avoid second-guessing the compiler and I
don't focus on performance as much as Blitz++ did. However, I am wary of adding
stuff if I think it could become a barrier if I really tried to make things
fast. I believe that improvements such as new traversal methods or the
optimization of specific expression patterns should be easy to implement without
turning the library inside out.
