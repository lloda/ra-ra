
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ownership.C
/// @brief Test ownership logic of array types.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/large.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl;
using real = double;

// TODO Test construction both by-value and by-ref, and between types.

// TODO Correct and complete this table, both for
// array-type-A::operator=(array-type-B) and for
// array-type-A::array-type::A(array-type-B).

// TODO Do/organize the tests for these tables.
// The constructors are all plain, the fields of the array record are copied/moved.
// TODO This table contain errors; review thoroughly.
/*
| to\fro cons | View            | Shared | Unique    | Owned     | Small          | SmallView | Tested in        |
|-------------+----------------+--------+-----------+-----------+----------------+------------+------------------|
| R           | borrow         | borrow | borrow    | borrow    | borrow         | ...        |                  |
| S           | share, null d. | share  | move      | share     | share, null d. |            | test-ownership.C |
| U           | copy           | copy   | move      | copy/move | copy           |            |                  |
| O           | copy           | copy   | copy/move | copy/move | copy           |            |                  |
| Small       |                |        |           |           | copy           |            |                  |
*/

// operator= however copies into. This is so that array ops look natural.
// The reason for the diagonal exceptions is to allow array-types to be initialized from a ref argument, which is required in operator>>(istream &, array-type). Maybe there's a better solution.
/*
| to\fro op= | View       | Shared    | Unique    | Owned       | Small     | Tested in        |
|------------+-----------+-----------+-----------+-------------+-----------+------------------|
| View        | copy into | copy into | copy into | copy into   | copy into | test-operators.C |
| Shared     | copy into | *share*   | copy into | copy into   | copy into |                  |
| Unique     | copy into | copy into | *move*    | copy into   | copy into |                  |
| Owned      | copy into | copy into | copy into | *copy/move* | copy into |                  |
| Small      | copy into | copy into | copy into | copy into   | *copy*    |                  |
*/

// TODO Maybe I want WithStorage/View<T> const and WithStorage/View<T const> to behave differently....
int main()
{
    real const check99[5] = {99, 99, 99, 99, 99};
    real const check11[5] = {11, 11, 11, 11, 11};

    TestRecorder tr;

    tr.section("Unique");
    {
        ra::Unique<real, 1> o({5}, 11.);
        tr.test(o.store!=nullptr);
        // ra::Unique<real, 1> z(o); // TODO Check that it fails to compile
        ra::Unique<real, 1> z(std::move(o));
        tr.test(o.store==nullptr);
        tr.test(std::equal(check11, check11+5, z.begin())); // was moved
        ra::Unique<real, 1> const c(std::move(z));
        tr.test(z.store==nullptr);
        tr.test(std::equal(check11, check11+5, c.begin())); // was moved
        {
            ra::Unique<real, 1> o({5}, 11.);
            ra::Unique<real, 1> q {};
            q = std::move(o);
            tr.test(o.store==nullptr);
            tr.test(std::equal(check11, check11+5, q.begin())); // was moved
        }
    }
    tr.section("Owned");
    {
        ra::Owned<real, 1> o({5}, 11.);
        ra::Owned<real, 1> z(o);
        o = 99.;
        tr.test(std::equal(check11, check11+5, z.begin()));
        tr.test(std::equal(check99, check99+5, o.begin()));
        tr.section("copy");
        {
            ra::Owned<real, 1> const c(o);
            tr.test(std::equal(check99, check99+5, c.begin()));
            tr.test(c.data()!=o.data());
            ra::Owned<real, 1> const q(c);
            tr.test(q.data()!=c.data());
            tr.test(std::equal(check99, check99+5, q.begin()));
            ra::Owned<real, 1> p(c);
            tr.test(p.data()!=c.data());
            tr.test(std::equal(check99, check99+5, p.begin()));
        }
        tr.section("WithStorage operator=(WithStorage) replaces, unlike View [ra20]");
        {
            ra::Owned<real, 1> o({5}, 11.);
            ra::Owned<real, 1> const p({5}, 99.);
            {
                ra::Owned<real, 1> q({7}, 4.);
                q = o;
                tr.test(std::equal(check11, check11+5, q.begin()));
            }
            {
                ra::Owned<real, 1> q({7}, 4.);
                q = p;
                tr.test(std::equal(check99, check99+5, q.begin()));
            }
            {
                ra::Owned<real, 1> q({7}, 4.);
                q = ra::Owned<real, 1>({5}, 11.);
                tr.test(std::equal(check11, check11+5, q.begin()));
            }
            tr.test(std::equal(check99, check99+5, p.begin()));
            tr.test(std::equal(check11, check11+5, o.begin()));
        }
        tr.section("move");
        {
            ra::Owned<real, 1> c(std::move(z));
            tr.test(z.store.size()==0); // std::vector does this on move...
            tr.test(std::equal(check11, check11+5, c.begin())); // was moved
        }
    }
    tr.section("Shared");
    {
        ra::Shared<real, 1> o({5}, 11.);
        ra::Shared<real, 1> z(o);
        o = 99.;
        tr.test(std::equal(check99, check99+5, z.begin()));

        ra::Shared<real, 1> const c(o);
        tr.test(std::equal(check99, check99+5, c.begin()));
        tr.test(c.data()==o.data());
        ra::Shared<real, 1> const q(c);
        tr.test(q.data()==c.data());
        ra::Shared<real, 1> p(c); // May be a BUG here; shared_ptr doesn't prevent this copy.
        tr.test(p.data()==c.data());
    }
    // The use of deleters allows Shared to work like View storage wise.
    tr.section("Shared with borrowed data");
    {
        ra::Shared<real, 1> o({5}, 11.);
        {
            ra::Shared<real, 1> borrower(ra::shared_borrowing(o));
            borrower = 99.;
            // Check that shared_borrowing really borrowed.
            tr.test(o.data()==borrower.data());
        }
        tr.test_eq(o, 99.);
    }

    return tr.summary();
}
