// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Basic dynamic size vector that isn't foreign
// From https://github.com/KonanM/vector. Removed rebinding support...

#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>
#include "ra/expr.hh"

namespace ra {

template <class T, size_t N>
struct sb_alloc
{
// Cf https://old.reddit.com/r/cpp/comments/hfv24j/small_vector_implementation_using_propagate_on/fw1kw7u/
    alignas(alignof(T)) T buffer[N];
    std::allocator<T> alloc;
    bool used = false;

    using value_type = T;
// must set these three values, which are responsible for the correct handling of the move assignment operator.
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::false_type;
    template <class U> struct rebind { using other = sb_alloc<U, N>; };

    constexpr sb_alloc() noexcept = default;
    template <class U> constexpr sb_alloc(sb_alloc<U, N> const &) noexcept {}
// don't copy the small buffer for the copy/move constructors, as that is done through the vector.
    constexpr sb_alloc(sb_alloc const & s) noexcept : used(s.used) {}
    constexpr sb_alloc & operator=(sb_alloc const & s) noexcept { used = s.used; return *this; }
    constexpr sb_alloc(sb_alloc &&) noexcept {}
    constexpr sb_alloc & operator=(sb_alloc const &&) noexcept { return *this; }

    [[nodiscard]] constexpr T *
    allocate(size_t const n)
    {
        used = (n <= N);
        if (used) {
            return buffer;
        } else {
            return alloc.allocate(n);
        }
    }
    constexpr void
    deallocate(void * p, const size_t n)
    {
        used = false;
        if (buffer!=p) {
            alloc.deallocate(static_cast<T *>(p), n);
        }
    }
// when propagate_on_container_move_assignment is false and this comparison returns false, an elementwise move is done instead of just taking over the memory. So the comparison has to return false when the small buffer is active.
    constexpr bool operator==(sb_alloc const & rhs) const { return !used && !rhs.used; }
};

template <class T, size_t N=4>
struct svector: public std::vector<T, sb_alloc<T, N>>
{
    using V = std::vector<T, sb_alloc<T, N>>;
// tell V we already have this capacity. Must be done in every constructor.
    constexpr svector() noexcept { V::reserve(N); }
    constexpr svector(svector const &) = default;
    constexpr svector & operator=(svector const &) = default;
    constexpr svector(svector && v) noexcept (std::is_nothrow_move_constructible_v<T>): svector()
    {
        V::operator=(std::move(v));
    }
    constexpr svector & operator=(svector && v) noexcept (std::is_nothrow_move_constructible_v<T>)
    {
        V::operator=(std::move(v));
        return *this;
    }
    constexpr explicit svector(size_t count): svector() { V::resize(count); } // hmm
    constexpr svector(size_t count, T const & value): svector() { V::assign(count, value); }
    template <class It> constexpr svector(It first, It last): svector() { V::insert(V::begin(), first, last); }
    constexpr svector(std::initializer_list<T> init): svector() { V::insert(V::begin(), init); }
    constexpr friend void swap(svector & a, svector & b) noexcept
    {
        std::swap(static_cast<V &>(a), static_cast<V &>(b));
    }
};

} // namespace ra
