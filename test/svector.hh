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
// have to set this three values, as they are responsible for the correct handling of the move assignment operator.
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
        if (buffer != p) {
            alloc.deallocate(static_cast<T *>(p), n);
        }
    }
// when propagate_on_container_move_assignment is false and this comparison returns false, an elementwise move is done instead of just taking over the memory. So the comparison has to return false when the small buffer is active.
    constexpr bool
    operator==(sb_alloc const & rhs) const
    {
        return !this->used && !rhs.used;
    }
};

template <class T, size_t N=4>
struct vector: public std::vector<T, sb_alloc<T, N>>
{
    using V = std::vector<T, sb_alloc<T, N>>;
// tell V we already have this capacity. Must be done in every constructor.
    constexpr vector() noexcept { V::reserve(N); }
    constexpr vector(vector const &) = default;
    constexpr vector & operator=(vector const &) = default;
    constexpr vector(vector && v) noexcept(std::is_nothrow_move_constructible_v<T>): vector()
    {
        V::operator=(std::move(v));
    }
    constexpr vector & operator=(vector && v) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        V::operator=(std::move(v));
        return *this;
    }
    constexpr explicit vector(size_t count): vector() { V::resize(count); } // hmm
    constexpr vector(size_t count, T const & value): vector() { V::assign(count, value); }
    template <class It> constexpr vector(It first, It last): vector() { V::insert(V::begin(), first, last); }
    constexpr vector(std::initializer_list<T> init): vector() { V::insert(V::begin(), init); }
    constexpr friend void swap(vector & a, vector & b) noexcept
    {
        std::swap(static_cast<V &>(a), static_cast<V &>(b));
    }
    using V::data, V::size;
    template <rank_t c=0> constexpr auto iter()
    {
        if constexpr (0==c) { return ra::ptr(data(), size()); } else { static_assert(false); } // tbd
    }
    template <rank_t c=0> constexpr auto iter() const
    {
        if constexpr (0==c) { return ra::ptr(data(), size()); } else { static_assert(false); } // tbd
    }
};

} // namespace ra
