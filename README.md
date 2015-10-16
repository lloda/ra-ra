
# ra-ra #

ra-ra is an expression template / multidimensional array library for
C++. It's not very mature, so if you need an array library for
'production', you may want to look somewhere else.


Building
-----------

The library is header-only and has no dependencies beyond a C++14 compiler and
the standard C++ library. There is a test suite of sorts in test/. These tests
test internal details and are not meant as demonstrations of how to use the
library.

I have tested in g++-5.2 and clang++-3.8. For clang on OS X you have to remove
the -Wa,q option in the SConstruct which is meant for gcc by setting CCFLAGS to
something else, for example:

  ```CCFLAGS="-march=native" CXXFLAGS=-O3 CXX=clang++-3.8 scons -j4```

I haven't tested on Windows.

There is a compilation error, only on clang++. I hope to fix this as soon as
possible.

* On clang++-3.8 there's a failed assertion on test-ra-operators.C for a
  compile-time computation that works fine in gcc.


Motivation
-----------

I do numerical work in C++ so I need a multidimensional array library. Most C++
array libraries seem to support little more than vectors and matrices, or the
small objects people need for low-dimensional vector algebra. Blitz++ was a
great early array library and it hasn't really been replaced as far as I can
tell.

I don't focus on performance as much as Blitz++ did, but I avoid stuff that I
think would become a barrier if I really tried to make things fast.

My other inspiration are the array languages, APL and J. They have gone further
than anyone else in exploring how array operations should work, and yet when new
array languages or libraries come out they seem for the most part to try and
copy Matlab. It's a pity.

There may still be some library out there that already does everything this one
does, but it's been a lot more fun doing my own and I hope to have learned some
C++ in the process.


Sui generis
-----------

* Index and size types are all signed.

* Index base is always 0.

* Default array order is C or row-major (last dimension changes fastest). You
  can make arrays stored in other orders by transposing or manipulating the
  strides yourself, but there is no built-in support for orders other than
  C-order.

* The selection operator is (). [] is supported for rank-1 arrays only, where it
  works in the exact same way as ().

* Array constructors have a very regularized format which isn't STL
  compatible. Single argument constructors always take an 'init-expression'
  which must provide enough shape information to construct the new array (unless
  the array type has static shape), and is otherwise subject to the regular
  argument shape agreement rules.


Features (not all working properly)
-----------

* Dynamic or static array rank. Dynamic or static array shape (all dimensions or
  none, alas).

* J-style shape agreement rules and rank extension for rank-0 operations of any
  arity and operands of any rank, any of which can a reference (so you can write
  on them).

* Iterators over cells of arbitrary rank.

* A rank conjunction (only for static rank operands and somewhat fragile).

* A proper selection operator with 'beating' of range or scalar
  subscripts. Beatable subscripts are not beaten if mixed with non-beatable
  subscripts, which is a defect.

* A TensorIndex object as in Blitz++ (doesn't work in the same exact way).

* Some compatibility with the STL.

* Somewhat type agnostic: you can make a array view over any piece of memory.


Missing (things that should be added or fixed at some point)
-----------

* Be namespace-clean.

* Concatenation, search, reshape, and other infinite rank or rank-!=0
  operations.

* Proper reductions. There are some reduction operations but no general
  mechanism. This may be the most obvious feature hole.

* Stencils, like in Blitz++.

* More clever traversal of arrays, like in Blitz++. Currently the traversal is
  always in row-major order, although the library does linearize the loop
  consistent with this limitation.


Non-features (things I won't try to add)
-----------

* No GPU / parallel support. You can parallelize above sometimes. Obviously
  that's not always the case, especially for the GPU. I don't want the library
  to become too complicated though.

* Linear algebra, quaternions, etc. Those things belong in other libraries. The
  library includes a dual number implementation but it's more of a demo of how
  to adapt user types to the library.

* Sparse arrays. These are fundamentally different and probably belong somewhere
  else. I'm not entirely sure though. You'd still want to mix & match with dense
  arrays, so maybe...
