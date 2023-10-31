// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Test ownership logic of array types.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/complex.hh"

using std::cout, std::endl, ra::TestRecorder;
using real = double;

// TODO Test construction both by-value and by-ref, and between types.
// TODO Maybe I want Container/View<T> const and Container/View<T const> to behave differently....
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
    tr.section("Big");
    {
        ra::Big<real, 1> o({5}, 11.);
        ra::Big<real, 1> z(o);
        o = 99.;
        tr.test(std::equal(check11, check11+5, z.begin()));
        tr.test(std::equal(check99, check99+5, o.begin()));
        tr.section("copy");
        {
            ra::Big<real, 1> const c(o);
            tr.test(std::equal(check99, check99+5, c.begin()));
            tr.test(c.data()!=o.data());
            ra::Big<real, 1> const q(c);
            tr.test(q.data()!=c.data());
            tr.test(std::equal(check99, check99+5, q.begin()));
            ra::Big<real, 1> p(c);
            tr.test(p.data()!=c.data());
            tr.test(std::equal(check99, check99+5, p.begin()));
        }
        auto test_container_assigment_op =
            [&]<class T>(T type, char const * tag)
            {
                tr.section("Container operator=(Container) replaces, unlike View [ra20] ", tag);
                {
                    T o({5}, 11.);
                    T const p({5}, 99.);
                    {
                        T q({7}, 4.);
                        q = o;
                        tr.test(std::equal(check11, check11+5, q.begin()));
                    }
                    {
                        T q({7}, 4.);
                        q = p;
                        tr.test(std::equal(check99, check99+5, q.begin()));
                    }
                    {
                        T q({7}, 4.);
                        q = T({5}, 11.);
                        tr.test(std::equal(check11, check11+5, q.begin()));
                    }
                    tr.test(std::equal(check99, check99+5, p.begin()));
                    tr.test(std::equal(check11, check11+5, o.begin()));
                }
                tr.section("move");
                {
                    T zz = z;
                    T c(std::move(zz));
                    tr.test(zz.store.size()==0); // std::vector does this on move...
                    tr.test(std::equal(check11, check11+5, c.begin())); // was moved
                }
            };
        test_container_assigment_op(ra::Big<real, 1>(), "static rank");
        test_container_assigment_op(ra::Big<real>(), "dynamic rank");
    }
    tr.section("Shared");
    {
        ra::Shared<real, 1> o({5}, 11.);
        ra::Shared<real, 1> z(o);
        o = 99.;
        tr.test_eq(5, o.size());
        tr.test_eq(5, z.size());
        tr.test_eq(99, z);
        tr.test_eq(99, o);

        ra::Shared<real, 1> const c(o);
        tr.test_eq(5, c.size());
        tr.test_eq(99, c);
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
