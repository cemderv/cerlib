// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

// This file originates from gch::small_vector,
// licensed under the MIT license:

/** small_vector.hpp
 * An implementation of `small_vector` (a vector with a small
 * buffer optimization). I would probably have preferred to
 * call this `inline_vector`, but I'll just go with the canonical
 * name for now.
 *
 * Copyright © 2020-2021 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Modified for use in cerlib.

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace cer
{

namespace concepts
{

template <typename T>
concept Complete = requires { sizeof(T); };

// Note: this mirrors the named requirements, not the standard concepts, so we don't require
// the destructor to be noexcept for Destructible.
template <typename T>
concept Destructible = std::is_destructible_v<T>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename T>
concept NoThrowDestructible = std::is_nothrow_destructible_v<T>;

// Note: this mirrors the named requirements, not the standard library concepts,
// so we don't require Destructible here.

template <typename T, typename... Args>
concept ConstructibleFrom = std::is_constructible_v<T, Args...>;

template <typename T, typename... Args>
concept NoThrowConstructibleFrom = std::is_nothrow_constructible_v<T, Args...>;

template <typename From, typename To>
concept ConvertibleTo =
    std::is_convertible_v<From, To> &&
    requires(std::add_rvalue_reference_t<From> (&f)(void)) { static_cast<To>(f()); };

template <typename From, typename To>
concept NoThrowConvertibleTo = std::is_nothrow_convertible_v<From, To> &&
                               requires(std::add_rvalue_reference_t<From> (&f)() noexcept) {
                                   {
                                       static_cast<To>(f())
                                   } noexcept;
                               };

// Note: std::default_initializable requires std::destructible.
template <typename T>
concept DefaultConstructible = ConstructibleFrom<T> && requires { T{}; } &&
                               requires { ::new (static_cast<void*>(nullptr)) T; };

template <typename T>
concept MoveAssignable = std::assignable_from<T&, T>;

template <typename T>
concept CopyAssignable = MoveAssignable<T> && std::assignable_from<T&, T&> &&
                         std::assignable_from<T&, const T&> && std::assignable_from<T&, const T>;

template <typename T>
concept MoveConstructible = ConstructibleFrom<T, T> && ConvertibleTo<T, T>;

template <typename T>
concept NoThrowMoveConstructible = NoThrowConstructibleFrom<T, T> && NoThrowConvertibleTo<T, T>;

template <typename T>
concept CopyConstructible =
    MoveConstructible<T> && ConstructibleFrom<T, T&> && ConvertibleTo<T&, T> &&
    ConstructibleFrom<T, const T&> && ConvertibleTo<const T&, T> && ConstructibleFrom<T, const T> &&
    ConvertibleTo<const T, T>;

template <typename T>
concept NoThrowCopyConstructible =
    NoThrowMoveConstructible<T> && NoThrowConstructibleFrom<T, T&> && NoThrowConvertibleTo<T&, T> &&
    NoThrowConstructibleFrom<T, const T&> && NoThrowConvertibleTo<const T&, T> &&
    NoThrowConstructibleFrom<T, const T> && NoThrowConvertibleTo<const T, T>;

template <typename T>
concept Swappable = std::swappable<T>;

template <typename T>
concept EqualityComparable = std::equality_comparable<T>;

// T is a type
// X is a Container
// A is an Allocator
// if X::allocator_type then
//   std::same_as<typename X::allocator_type,
//                typename std::allocator_traits<A>::template rebind_alloc<T>>
// otherwise
//   no condition; we use std::allocator<T> regardless of A
//
// see [22.2.1].16
template <typename T, typename X, typename A, typename ...Args>
    concept EmplaceConstructible =
          std::same_as<typename X::value_type, T>
      &&  (  (  requires { typename X::allocator_type; } // only perform this check if X is
            &&  std::same_as<typename X::allocator_type, // allocator-aware
                             typename std::allocator_traits<A>::template rebind_alloc<T>>
            &&  (  requires (A m, T *p, Args&&... args)
                   {
                     m.construct (p, std::forward<Args> (args)...);
                   }
               ||  requires (T *p, Args&&... args)
                   {
                     { std::construct_at (p, std::forward<Args> (args)...) } -> std::same_as<T *>;
                   }))
         ||  (! requires { typename X::allocator_type; }
            &&  requires (T *p, Args&&... args)
                {
                  { std::construct_at (p, std::forward<Args> (args)...) } -> std::same_as<T *>;
                }));

template <typename T, typename X, typename A = std::conditional_t<requires {
                                      typename X::allocator_type;
                                  }, typename X::allocator_type, std::allocator<T>>>
concept DefaultInsertable = EmplaceConstructible<T, X, A>;

template <typename T, typename X, typename A = std::conditional_t<requires {
                                      typename X::allocator_type;
                                  }, typename X::allocator_type, std::allocator<T>>>
concept MoveInsertable = EmplaceConstructible<T, X, A, T>;

template <typename T, typename X, typename A = std::conditional_t<requires {
                                      typename X::allocator_type;
                                  }, typename X::allocator_type, std::allocator<T>>>
concept CopyInsertable = MoveInsertable<T, X, A> && EmplaceConstructible<T, X, A, T&> &&
                         EmplaceConstructible<T, X, A, const T&>;

// same method as with EmplaceConstructible
template <typename T, typename X, typename A = std::conditional_t<requires {
                                      typename X::allocator_type;
                                  }, typename X::allocator_type, std::allocator<T>>>
concept Erasable = std::same_as<typename X::value_type, T> &&
                   ((requires { typename X::allocator_type; } // if X is allocator aware
                     && std::same_as<typename X::allocator_type,
                                     typename std::allocator_traits<A>::template rebind_alloc<T>> &&
                     (requires(A m, T* p) { m.destroy(p); } || std::is_destructible_v<T>)) ||
                    (!requires { typename X::allocator_type; } && std::is_destructible_v<T>));

template <typename T>
concept ContextuallyConvertibleToBool = std::constructible_from<bool, T>;

template <typename T>
concept BoolConstant =
    std::derived_from<T, std::true_type> || std::derived_from<T, std::false_type>;

template <typename T>
concept NullablePointer =
    EqualityComparable<T> && DefaultConstructible<T> && CopyConstructible<T> && CopyAssignable<T> &&
    Destructible<T> && ConstructibleFrom<T, std::nullptr_t> && ConvertibleTo<std::nullptr_t, T> &&
    requires(T p, T q, std::nullptr_t np) {
        T(np);
        {
            p = np
        } -> std::same_as<T&>;
        {
            p != q
        } -> ContextuallyConvertibleToBool;
        {
            p == np
        } -> ContextuallyConvertibleToBool;
        {
            np == p
        } -> ContextuallyConvertibleToBool;
        {
            p != np
        } -> ContextuallyConvertibleToBool;
        {
            np != p
        } -> ContextuallyConvertibleToBool;
    };

static_assert(NullablePointer<int*>);
static_assert(!NullablePointer<int>);

template <typename A, typename T, typename U = T*>
concept AllocatorFor =
    NoThrowCopyConstructible<A> &&
    requires(A                                                           a,
             typename std::allocator_traits<A>::template rebind_alloc<U> b,
             U                                                           xp,
             typename std::allocator_traits<A>::pointer                  p,
             typename std::allocator_traits<A>::const_pointer            cp,
             typename std::allocator_traits<A>::void_pointer             vp,
             typename std::allocator_traits<A>::const_void_pointer       cvp,
             typename std::allocator_traits<A>::value_type&              r,
             typename std::allocator_traits<A>::size_type                n) {
        /** Inner types **/
        // A::pointer
        requires NullablePointer<typename std::allocator_traits<A>::pointer>;
        requires std::random_access_iterator<typename std::allocator_traits<A>::pointer>;
        requires std::contiguous_iterator<typename std::allocator_traits<A>::pointer>;

        // A::const_pointer
        requires NullablePointer<typename std::allocator_traits<A>::const_pointer>;
        requires std::random_access_iterator<typename std::allocator_traits<A>::const_pointer>;
        requires std::contiguous_iterator<typename std::allocator_traits<A>::const_pointer>;

        requires std::convertible_to<typename std::allocator_traits<A>::pointer,
                                     typename std::allocator_traits<A>::const_pointer>;

        // A::void_pointer
        requires NullablePointer<typename std::allocator_traits<A>::void_pointer>;

        requires std::convertible_to<typename std::allocator_traits<A>::pointer,
                                     typename std::allocator_traits<A>::void_pointer>;

        requires std::same_as<typename std::allocator_traits<A>::void_pointer,
                              typename std::allocator_traits<decltype(b)>::void_pointer>;

        // A::const_void_pointer
        requires NullablePointer<typename std::allocator_traits<A>::const_void_pointer>;

        requires std::convertible_to<typename std::allocator_traits<A>::pointer,
                                     typename std::allocator_traits<A>::const_void_pointer>;

        requires std::convertible_to<typename std::allocator_traits<A>::const_pointer,
                                     typename std::allocator_traits<A>::const_void_pointer>;

        requires std::convertible_to<typename std::allocator_traits<A>::void_pointer,
                                     typename std::allocator_traits<A>::const_void_pointer>;

        requires std::same_as<typename std::allocator_traits<A>::const_void_pointer,
                              typename std::allocator_traits<decltype(b)>::const_void_pointer>;

        // A::value_type
        typename A::value_type;
        requires std::same_as<typename A::value_type, T>;
        requires std::same_as<typename A::value_type,
                              typename std::allocator_traits<A>::value_type>;

        // A::size_type
        requires std::unsigned_integral<typename std::allocator_traits<A>::size_type>;

        // A::difference_type
        requires std::signed_integral<typename std::allocator_traits<A>::difference_type>;

        // A::template rebind<U>::other [optional]
        requires !requires { typename A::template rebind<U>::other; } || requires {
            requires std::same_as<decltype(b), typename A::template rebind<U>::other>;
            requires std::same_as<A, typename decltype(b)::template rebind<T>::other>;
        };

        /** Operations on pointers **/
        {
            *p
        } -> std::same_as<typename A::value_type&>;
        {
            *cp
        } -> std::same_as<const typename A::value_type&>;

        // Language in the standard implies that `decltype (p)` must either
        // be a raw pointer or implement `operator->`. There is no mention
        // of `std::to_address` or `std::pointer_traits<Ptr>::to_address`.
        requires std::same_as<decltype(p), typename A::value_type*> || requires {
            {
                p.operator->()
            } -> std::same_as<typename A::value_type*>;
        };

        requires std::same_as<decltype(cp), const typename A::value_type*> || requires {
            {
                cp.operator->()
            } -> std::same_as<const typename A::value_type*>;
        };

        {
            static_cast<decltype(p)>(vp)
        } -> std::same_as<decltype(p)>;
        {
            static_cast<decltype(cp)>(cvp)
        } -> std::same_as<decltype(cp)>;

        {
            std::pointer_traits<decltype(p)>::pointer_to(r)
        } -> std::same_as<decltype(p)>;

        /** Storage and lifetime operations **/
        // a.allocate (n)
        {
            a.allocate(n)
        } -> std::same_as<decltype(p)>;

        // a.allocate (n, cvp) [optional]
        requires !requires { a.allocate(n, cvp); } || requires {
            {
                a.allocate(n, cvp)
            } -> std::same_as<decltype(p)>;
        };

        // a.deallocate (p, n)
        {
            a.deallocate(p, n)
        } -> std::convertible_to<void>;

        // a.max_size () [optional]
        requires !requires { a.max_size(); } || requires {
            {
                a.max_size()
            } -> std::same_as<decltype(n)>;
        };

        // a.construct (xp, args) [optional]
        requires !requires { a.construct(xp); } || requires {
            {
                a.construct(xp)
            } -> std::convertible_to<void>;
        };

        // a.destroy (xp) [optional]
        requires !requires { a.destroy(xp); } || requires {
            {
                a.destroy(xp)
            } -> std::convertible_to<void>;
        };

        /** Relationship between instances **/
        requires NoThrowConstructibleFrom<A, decltype(b)>;
        requires NoThrowConstructibleFrom<A, decltype(std::move(b))>;

        requires BoolConstant<typename std::allocator_traits<A>::is_always_equal>;

        /** Influence on container operations **/
        // a.select_on_container_copy_construction () [optional]
        requires !requires { a.select_on_container_copy_construction(); } || requires {
            {
                a.select_on_container_copy_construction()
            } -> std::same_as<A>;
        };

        requires BoolConstant<
            typename std::allocator_traits<A>::propagate_on_container_copy_assignment>;

        requires BoolConstant<
            typename std::allocator_traits<A>::propagate_on_container_move_assignment>;

        requires BoolConstant<typename std::allocator_traits<A>::propagate_on_container_swap>;

        {
            a == b
        } -> std::same_as<bool>;
        {
            a != b
        } -> std::same_as<bool>;
    } &&
    requires(A a1, A a2) {
        {
            a1 == a2
        } -> std::same_as<bool>;
        {
            a1 != a2
        } -> std::same_as<bool>;
    };

static_assert(AllocatorFor<std::allocator<int>, int>,
              "std::allocator<int> failed to meet Allocator concept requirements.");

template <typename A>
concept Allocator = AllocatorFor<A, typename A::value_type>;

namespace List
{

// Basically, these shut off the concepts if we have an incomplete type.
// This namespace is only needed because of issues on Clang
// preventing us from short-circuiting for incomplete types.

template <typename T>
concept Destructible = !concepts::Complete<T> || concepts::Destructible<T>;

template <typename T>
concept MoveAssignable = !concepts::Complete<T> || concepts::MoveAssignable<T>;

template <typename T>
concept CopyAssignable = !concepts::Complete<T> || concepts::CopyAssignable<T>;

template <typename T>
concept MoveConstructible = !concepts::Complete<T> || concepts::MoveConstructible<T>;

template <typename T>
concept CopyConstructible = !concepts::Complete<T> || concepts::CopyConstructible<T>;

template <typename T>
concept Swappable = !concepts::Complete<T> || concepts::Swappable<T>;

template <typename T, typename SmallVector, typename Alloc>
concept DefaultInsertable =
    !concepts::Complete<T> || concepts::DefaultInsertable<T, SmallVector, Alloc>;

template <typename T, typename SmallVector, typename Alloc>
concept MoveInsertable = !concepts::Complete<T> || concepts::MoveInsertable<T, SmallVector, Alloc>;

template <typename T, typename SmallVector, typename Alloc>
concept CopyInsertable = !concepts::Complete<T> || concepts::CopyInsertable<T, SmallVector, Alloc>;

template <typename T, typename SmallVector, typename Alloc>
concept Erasable = !concepts::Complete<T> || concepts::Erasable<T, SmallVector, Alloc>;

template <typename T, typename SmallVector, typename Alloc, typename... Args>
concept EmplaceConstructible =
    !concepts::Complete<T> || concepts::EmplaceConstructible<T, SmallVector, Alloc, Args...>;

template <typename Alloc, typename T>
concept AllocatorFor = !concepts::Complete<T> || concepts::AllocatorFor<Alloc, T>;

template <typename Alloc>
concept Allocator = AllocatorFor<Alloc, typename Alloc::value_type>;
} // namespace List
} // namespace concepts

template <typename Allocator>
    requires concepts::List::Allocator<Allocator>
struct default_buffer_size;

template <typename T,
          unsigned InlineCapacity = default_buffer_size<std::allocator<T>>::value,
          typename Allocator      = std::allocator<T>>
    requires concepts::List::AllocatorFor<Allocator, T>
class List;

template <typename Allocator>
    requires concepts::List::Allocator<Allocator>
struct default_buffer_size
{
  private:
    template <typename, typename Enable = void>
    struct is_complete : std::false_type
    {
    };

    template <typename U>
    struct is_complete<U, decltype(static_cast<void>(sizeof(U)))> : std::true_type
    {
    };

  public:
    using allocator_type = Allocator;
    using value_type     = typename std::allocator_traits<allocator_type>::value_type;
    using EmptyList      = List<value_type, 0, allocator_type>;

    static_assert(is_complete<value_type>::value,
                  "Calculation of a default number of elements requires that `T` be complete.");

    static constexpr unsigned buffer_max  = 256;
    static constexpr unsigned ideal_total = 64;

    static constexpr unsigned ideal_buffer = ideal_total - sizeof(EmptyList);

    static_assert(sizeof(EmptyList) != 0, "Empty List should not have size 0.");

    static_assert(ideal_buffer < ideal_total, "Empty List is larger than ideal_total.");

    static constexpr unsigned value =
        (sizeof(value_type) <= ideal_buffer) ? (ideal_buffer / sizeof(value_type)) : 1;
};

template <typename Allocator>
inline constexpr unsigned default_buffer_size_v = default_buffer_size<Allocator>::value;

template <typename Pointer, typename DifferenceType>
class ListIterator
{
  public:
    using difference_type   = DifferenceType;
    using value_type        = typename std::iterator_traits<Pointer>::value_type;
    using pointer           = typename std::iterator_traits<Pointer>::pointer;
    using reference         = typename std::iterator_traits<Pointer>::reference;
    using iterator_category = typename std::iterator_traits<Pointer>::iterator_category;
    using iterator_concept  = std::contiguous_iterator_tag;

         ListIterator(const ListIterator&)                   = default;
         ListIterator(ListIterator&&) noexcept               = default;
    auto operator=(const ListIterator&) -> ListIterator&     = default;
    auto operator=(ListIterator&&) noexcept -> ListIterator& = default;
    ~    ListIterator()                                      = default;

#ifdef NDEBUG
    ListIterator() = default;
#else
    constexpr ListIterator() noexcept
        : m_ptr()
    {
    }
#endif

    constexpr explicit ListIterator(const Pointer& p) noexcept
        : m_ptr(p)
    {
    }

    template <typename U,
              typename D,
              std::enable_if_t<std::is_convertible_v<U, Pointer>>* = nullptr>
    constexpr explicit(false) ListIterator(const ListIterator<U, D>& other) noexcept
        : m_ptr(other.base())
    {
    }

    constexpr ListIterator& operator++() noexcept
    {
        ++m_ptr;
        return *this;
    }

    constexpr ListIterator operator++(int) noexcept
    {
        return ListIterator(m_ptr++);
    }

    constexpr ListIterator& operator--() noexcept
    {
        --m_ptr;
        return *this;
    }

    constexpr ListIterator operator--(int) noexcept
    {
        return ListIterator(m_ptr--);
    }

    constexpr ListIterator& operator+=(difference_type n) noexcept
    {
        m_ptr += n;
        return *this;
    }

    constexpr ListIterator operator+(difference_type n) const noexcept
    {
        return ListIterator(m_ptr + n);
    }

    constexpr ListIterator& operator-=(difference_type n) noexcept
    {
        m_ptr -= n;
        return *this;
    }

    constexpr ListIterator operator-(difference_type n) const noexcept
    {
        return ListIterator(m_ptr - n);
    }

    constexpr reference operator*() const noexcept
    {
        return launder_and_dereference(m_ptr);
    }

    constexpr pointer operator->() const noexcept
    {
        return get_pointer(m_ptr);
    }

    constexpr reference operator[](difference_type n) const noexcept
    {
        return launder_and_dereference(m_ptr + n);
    }

    constexpr const Pointer& base() const noexcept
    {
        return m_ptr;
    }

  private:
    template <typename Ptr                                                     = Pointer,
              typename std::enable_if<std::is_pointer<Ptr>::value, bool>::type = true>
    static constexpr pointer get_pointer(Pointer ptr) noexcept
    {
        return ptr;
    }

    template <typename Ptr = Pointer, std::enable_if_t<!std::is_pointer_v<Ptr>, bool> = false>
    static constexpr auto get_pointer(Pointer ptr) noexcept -> pointer
    {
        // Given the requirements for Allocator, Pointer must either be a raw pointer, or
        // have a defined operator-> which returns a raw pointer.
        return ptr.operator->();
    }

    template <typename Ptr = Pointer, std::enable_if_t<std::is_pointer_v<Ptr>, bool> = true>
    static constexpr auto launder_and_dereference(Pointer ptr) noexcept -> reference
    {
        return *std::launder(ptr);
    }

    template <typename Ptr = Pointer, std::enable_if_t<!std::is_pointer_v<Ptr>, bool> = false>
    static constexpr auto launder_and_dereference(Pointer ptr) noexcept -> reference
    {
        return *ptr;
    }

    Pointer m_ptr;
};

// Note: Passing this on from "Gaby" in stl_iterator.h -- templated
//       comparisons in generic code should have overloads for both
//       homogenous and heterogeneous types. This is because we get
//       ambiguous overload resolution when std::rel_ops is visible
//       (ie. `using namespace std::rel_ops`).

template <typename PointerLHS,
          typename DifferenceTypeLHS,
          typename PointerRHS,
          typename DifferenceTypeRHS>
constexpr auto operator==(const ListIterator<PointerLHS, DifferenceTypeLHS>& lhs,
                          const ListIterator<PointerRHS, DifferenceTypeRHS>& rhs) noexcept -> bool
{
    return lhs.base() == rhs.base();
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator==(const ListIterator<Pointer, DifferenceType>& lhs,
                          const ListIterator<Pointer, DifferenceType>& rhs) noexcept -> bool
{
    return lhs.base() == rhs.base();
}

template <typename PointerLHS,
          typename DifferenceTypeLHS,
          typename PointerRHS,
          typename DifferenceTypeRHS>
constexpr auto operator!=(const ListIterator<PointerLHS, DifferenceTypeLHS>& lhs,
                          const ListIterator<PointerRHS, DifferenceTypeRHS>& rhs) noexcept -> bool
{
    return lhs.base() != rhs.base();
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator!=(const ListIterator<Pointer, DifferenceType>& lhs,
                          const ListIterator<Pointer, DifferenceType>& rhs) noexcept -> bool
{
    return lhs.base() != rhs.base();
}

template <typename PointerLHS,
          typename DifferenceTypeLHS,
          typename PointerRHS,
          typename DifferenceTypeRHS>
constexpr auto operator<(const ListIterator<PointerLHS, DifferenceTypeLHS>& lhs,
                         const ListIterator<PointerRHS, DifferenceTypeRHS>& rhs) noexcept -> bool
{
    return lhs.base() < rhs.base();
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator<(const ListIterator<Pointer, DifferenceType>& lhs,
                         const ListIterator<Pointer, DifferenceType>& rhs) noexcept -> bool
{
    return lhs.base() < rhs.base();
}

template <typename PointerLHS,
          typename DifferenceTypeLHS,
          typename PointerRHS,
          typename DifferenceTypeRHS>
constexpr auto operator>=(const ListIterator<PointerLHS, DifferenceTypeLHS>& lhs,
                          const ListIterator<PointerRHS, DifferenceTypeRHS>& rhs) noexcept -> bool
{
    return lhs.base() >= rhs.base();
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator>=(const ListIterator<Pointer, DifferenceType>& lhs,
                          const ListIterator<Pointer, DifferenceType>& rhs) noexcept -> bool
{
    return lhs.base() >= rhs.base();
}

template <typename PointerLHS,
          typename DifferenceTypeLHS,
          typename PointerRHS,
          typename DifferenceTypeRHS>
constexpr auto operator>(const ListIterator<PointerLHS, DifferenceTypeLHS>& lhs,
                         const ListIterator<PointerRHS, DifferenceTypeRHS>& rhs) noexcept -> bool
{
    return lhs.base() > rhs.base();
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator>(const ListIterator<Pointer, DifferenceType>& lhs,
                         const ListIterator<Pointer, DifferenceType>& rhs) noexcept -> bool
{
    return lhs.base() > rhs.base();
}

template <typename PointerLHS,
          typename DifferenceTypeLHS,
          typename PointerRHS,
          typename DifferenceTypeRHS>
constexpr auto operator<=(const ListIterator<PointerLHS, DifferenceTypeLHS>& lhs,
                          const ListIterator<PointerRHS, DifferenceTypeRHS>& rhs) noexcept -> bool
{
    return lhs.base() <= rhs.base();
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator<=(const ListIterator<Pointer, DifferenceType>& lhs,
                          const ListIterator<Pointer, DifferenceType>& rhs) noexcept -> bool
{
    return lhs.base() <= rhs.base();
}

template <typename PointerLHS, typename PointerRHS, typename DifferenceType>
constexpr auto operator-(const ListIterator<PointerLHS, DifferenceType>& lhs,
                         const ListIterator<PointerRHS, DifferenceType>& rhs) noexcept
    -> DifferenceType
{
    return static_cast<DifferenceType>(lhs.base() - rhs.base());
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator-(const ListIterator<Pointer, DifferenceType>& lhs,
                         const ListIterator<Pointer, DifferenceType>& rhs) noexcept
    -> DifferenceType
{
    return static_cast<DifferenceType>(lhs.base() - rhs.base());
}

template <typename Pointer, typename DifferenceType>
constexpr auto operator+(DifferenceType n, const ListIterator<Pointer, DifferenceType>& it) noexcept
    -> ListIterator<Pointer, DifferenceType>
{
    return it + n;
}

namespace detail
{

template <typename T, unsigned InlineCapacity>
class inline_storage
{
  public:
    using value_ty = T;

         inline_storage()                                        = default;
         inline_storage(const inline_storage&)                   = delete;
         inline_storage(inline_storage&&) noexcept               = delete;
    auto operator=(const inline_storage&) -> inline_storage&     = delete;
    auto operator=(inline_storage&&) noexcept -> inline_storage& = delete;
    ~    inline_storage()                                        = default;

    [[nodiscard]] constexpr auto get_inline_ptr() noexcept -> value_ty*
    {
        return static_cast<value_ty*>(static_cast<void*>(std::addressof(*m_data)));
    }

  private:
    union alignas(alignof(value_ty)) {
        unsigned char _[sizeof(value_ty)];
    } m_data[InlineCapacity];
};

template <typename Allocator,
          bool AvailableForEBO = std::is_empty_v<Allocator> && !std::is_final_v<Allocator>>
class allocator_inliner;

template <typename Allocator>
class
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023918L
    __declspec(empty_bases)
#endif
        allocator_inliner<Allocator, true> : Allocator
{
    using alloc_traits = std::allocator_traits<Allocator>;

    static constexpr bool copy_assign_is_noop =
        !alloc_traits::propagate_on_container_copy_assignment::value;

    static constexpr bool move_assign_is_noop =
        !alloc_traits::propagate_on_container_move_assignment::value;

    static constexpr bool swap_is_noop = !alloc_traits::propagate_on_container_swap::value;

    template <bool IsNoOp = copy_assign_is_noop, std::enable_if_t<IsNoOp, bool> = true>
    constexpr void maybe_assign(const allocator_inliner&) noexcept
    {
    }

    template <bool IsNoOp                                  = copy_assign_is_noop,
              typename std::enable_if<!IsNoOp, bool>::type = false>
    constexpr void maybe_assign(const allocator_inliner& other) noexcept(
        noexcept(std::declval<Allocator&>().operator=(other)))
    {
        Allocator::operator=(other);
    }

    template <bool IsNoOp = move_assign_is_noop, std::enable_if_t<IsNoOp, bool> = true>
    constexpr void maybe_assign(allocator_inliner&&) noexcept
    {
    }

    template <bool IsNoOp = move_assign_is_noop, std::enable_if_t<!IsNoOp, bool> = false>
    constexpr void maybe_assign(allocator_inliner&& other) noexcept(
        noexcept(std::declval<Allocator&>().operator=(std::move(other))))
    {
        Allocator::operator=(std::move(other));
    }

  public:
    allocator_inliner()                             = default;
    allocator_inliner(const allocator_inliner&)     = default;
    allocator_inliner(allocator_inliner&&) noexcept = default;
    //    allocator_inliner& operator= (const allocator_inliner&)     = impl;
    //    allocator_inliner& operator= (allocator_inliner&&) noexcept = impl;
    ~allocator_inliner() = default;

    constexpr explicit allocator_inliner(const Allocator& alloc) noexcept
        : Allocator(alloc)
    {
    }

    constexpr auto operator=(const allocator_inliner& other) noexcept(
        noexcept(std::declval<allocator_inliner&>().maybe_assign(other))) -> allocator_inliner&
    {
        assert(&other != this &&
               "`allocator_inliner` should not participate in self-copy-assignment.");
        maybe_assign(other);
        return *this;
    }

    constexpr auto operator=(allocator_inliner&& other) noexcept(noexcept(
        std::declval<allocator_inliner&>().maybe_assign(std::move(other)))) -> allocator_inliner&
    {
        assert(&other != this &&
               "`allocator_inliner` should not participate in self-move-assignment.");
        maybe_assign(std::move(other));
        return *this;
    }

    constexpr auto allocator_ref() noexcept -> Allocator&
    {
        return *this;
    }

    constexpr auto allocator_ref() const noexcept -> const Allocator&
    {
        return *this;
    }

    template <bool IsNoOp = swap_is_noop, std::enable_if_t<IsNoOp, bool> = true>
    constexpr void swap(allocator_inliner&)
    {
    }

    template <bool IsNoOp = swap_is_noop, std::enable_if_t<!IsNoOp, bool> = false>
    constexpr void swap(allocator_inliner& other)
    {
        using std::swap;
        swap(static_cast<Allocator&>(*this), static_cast<Allocator&>(other));
    }
};

template <typename Allocator>
class allocator_inliner<Allocator, false>
{
    using alloc_traits = std::allocator_traits<Allocator>;

    static constexpr bool copy_assign_is_noop =
        !alloc_traits::propagate_on_container_copy_assignment::value;

    static constexpr bool move_assign_is_noop =
        !alloc_traits::propagate_on_container_move_assignment::value;

    static constexpr bool swap_is_noop = !alloc_traits::propagate_on_container_swap::value;

    template <bool IsNoOp = copy_assign_is_noop, std::enable_if_t<IsNoOp, bool> = true>
    constexpr void maybe_assign(const allocator_inliner&) noexcept
    {
    }

    template <bool IsNoOp = copy_assign_is_noop, std::enable_if_t<!IsNoOp, bool> = false>
    constexpr void maybe_assign(const allocator_inliner& other) noexcept(
        noexcept(std::declval<decltype(other.m_alloc)&>() = other.m_alloc))
    {
        m_alloc = other.m_alloc;
    }

    template <bool IsNoOp = move_assign_is_noop, std::enable_if_t<IsNoOp, bool> = true>
    constexpr void maybe_assign(allocator_inliner&&) noexcept
    {
    }

    template <bool IsNoOp = move_assign_is_noop, std::enable_if_t<!IsNoOp, bool> = false>
    constexpr void maybe_assign(allocator_inliner&& other) noexcept(
        noexcept(std::declval<decltype(other.m_alloc)&>() = std::move(other.m_alloc)))
    {
        m_alloc = std::move(other.m_alloc);
    }

  public:
    allocator_inliner()                             = default;
    allocator_inliner(const allocator_inliner&)     = default;
    allocator_inliner(allocator_inliner&&) noexcept = default;
    //    allocator_inliner& operator= (const allocator_inliner&)     = impl;
    //    allocator_inliner& operator= (allocator_inliner&&) noexcept = impl;
    ~allocator_inliner() = default;

    constexpr explicit allocator_inliner(const Allocator& alloc) noexcept
        : m_alloc(alloc)
    {
    }

    constexpr auto operator=(const allocator_inliner& other) noexcept(
        noexcept(std::declval<allocator_inliner&>().maybe_assign(other))) -> allocator_inliner&
    {
        assert(&other != this &&
               "`allocator_inliner` should not participate in self-copy-assignment.");
        maybe_assign(other);
        return *this;
    }

    constexpr auto operator=(allocator_inliner&& other) noexcept(noexcept(
        std::declval<allocator_inliner&>().maybe_assign(std::move(other)))) -> allocator_inliner&
    {
        assert(&other != this &&
               "`allocator_inliner` should not participate in self-move-assignment.");
        maybe_assign(std::move(other));
        return *this;
    }

    constexpr auto allocator_ref() noexcept -> Allocator&
    {
        return m_alloc;
    }

    constexpr auto allocator_ref() const noexcept -> const Allocator&
    {
        return m_alloc;
    }

    template <bool IsNoOp = swap_is_noop, std::enable_if_t<IsNoOp, bool> = true>
    constexpr void swap(allocator_inliner&)
    {
    }

    template <bool IsNoOp = swap_is_noop, std::enable_if_t<!IsNoOp, bool> = false>
    constexpr void swap(allocator_inliner& other)
    {
        using std::swap;
        swap(m_alloc, other.m_alloc);
    }

  private:
    Allocator m_alloc;
};

template <typename Allocator>
class
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023918L
    __declspec(empty_bases)
#endif
        allocator_interface : public allocator_inliner<Allocator>
{
  public:
    template <typename, typename = void>
    struct is_complete : std::false_type
    {
    };

    template <typename U>
    struct is_complete<U, decltype(static_cast<void>(sizeof(U)))> : std::true_type
    {
    };

    using size_type = typename std::allocator_traits<Allocator>::size_type;

    // If difference_type is larger than size_type then we need
    // to rectify that problem.
    using difference_type = std::conditional_t<
        (static_cast<std::size_t>((std::numeric_limits<size_type>::max)()) < // less-than
         static_cast<std::size_t>((std::numeric_limits<typename std::allocator_traits<
                                       Allocator>::difference_type>::max)())),
        std::make_signed_t<size_type>,
        typename std::allocator_traits<Allocator>::difference_type>;

  private:
    using alloc_base = allocator_inliner<Allocator>;

  protected:
    using alloc_ty     = Allocator;
    using alloc_traits = std::allocator_traits<alloc_ty>;
    using value_ty     = typename alloc_traits::value_type;
    using ptr          = typename alloc_traits::pointer;
    using cptr         = typename alloc_traits::const_pointer;
    using vptr         = typename alloc_traits::void_pointer;
    using cvptr        = typename alloc_traits::const_void_pointer;

    // Select the fastest types larger than the user-facing types. These are only intended for
    // internal computations, and should not have any memory footprint visible to consumers.
    using size_ty = std::conditional_t<
        (sizeof(size_type) <= sizeof(std::uint8_t)),
        std::uint_fast8_t,
        std::conditional_t<
            (sizeof(size_type) <= sizeof(std::uint16_t)),
            std::uint_fast16_t,
            std::conditional_t<(sizeof(size_type) <= sizeof(std::uint32_t)),
                               std::uint_fast32_t,
                               std::conditional_t<(sizeof(size_type) <= sizeof(std::uint64_t)),
                                                  std::uint_fast64_t,
                                                  size_type>>>>;

    using diff_ty = std::conditional_t<
        (sizeof(difference_type) <= sizeof(std::int8_t)),
        std::int_fast8_t,
        std::conditional_t<
            (sizeof(difference_type) <= sizeof(std::int16_t)),
            std::int_fast16_t,
            std::conditional_t<(sizeof(difference_type) <= sizeof(std::int32_t)),
                               std::int_fast32_t,
                               std::conditional_t<(sizeof(difference_type) <= sizeof(std::int64_t)),
                                                  std::int_fast64_t,
                                                  difference_type>>>>;

    using alloc_base::allocator_ref;

  private:
    template <typename...>
    using void_t = void;

    template <bool B>
    using bool_constant = std::integral_constant<bool, B>;

    template <typename V, typename Enable = void>
    struct is_trivially_destructible : std::false_type
    {
    };

    template <typename V>
    struct is_trivially_destructible<V, std::enable_if_t<is_complete<V>::value>>
        : std::is_trivially_destructible<V>
    {
    };

    template <typename Void, typename T, typename... Args>
    struct is_trivially_constructible_impl : std::false_type
    {
    };

    template <typename V, typename... Args>
    struct is_trivially_constructible_impl<std::enable_if_t<is_complete<V>::value>, V, Args...>
        : std::is_trivially_constructible<V, Args...>
    {
    };

    template <typename V, typename... Args>
    struct is_trivially_constructible : is_trivially_constructible_impl<void, V, Args...>
    {
    };

    template <typename T, typename Enable = void>
    struct underlying_if_enum
    {
        using type = T;
    };

    template <typename T>
    struct underlying_if_enum<T, std::enable_if_t<std::is_enum_v<T>>> : std::underlying_type<T>
    {
    };

    template <typename T>
    using underlying_if_enum_t = typename underlying_if_enum<T>::type;

    template <typename, typename = void>
    struct has_ptr_traits_to_address : std::false_type
    {
    };

    template <typename P>
    struct has_ptr_traits_to_address<
        P,
        void_t<decltype(std::pointer_traits<P>::to_address(std::declval<P>()))>> : std::true_type
    {
    };

    template <typename Void, typename A, typename V, typename... Args>
    struct has_alloc_construct_check : std::false_type
    {
    };

    template <typename A, typename V, typename... Args>
    struct has_alloc_construct_check<
        void_t<decltype(std::declval<A&>().construct(std::declval<V*>(), std::declval<Args>()...))>,
        A,
        V,
        Args...> : std::true_type
    {
    };

    template <typename Void, typename A, typename V, typename... Args>
    struct has_alloc_construct_impl : std::false_type
    {
    };

    template <typename A, typename V, typename... Args>
    struct has_alloc_construct_impl<typename std::enable_if<is_complete<V>::value>::type,
                                    A,
                                    V,
                                    Args...> : has_alloc_construct_check<void, A, V, Args...>
    {
    };

    template <typename A, typename V, typename... Args>
    struct has_alloc_construct : has_alloc_construct_impl<void, A, V, Args...>
    {
    };

    template <typename A, typename V, typename... Args>
    struct must_use_alloc_construct : bool_constant<!std::is_same<A, std::allocator<V>>::value &&
                                                    has_alloc_construct<A, V, Args...>::value>
    {
    };

    template <typename Void, typename A, typename V>
    struct has_alloc_destroy_impl : std::false_type
    {
    };

    template <typename A, typename V>
    struct has_alloc_destroy_impl<void_t<decltype(std::declval<A&>().destroy(std::declval<V*>()))>,
                                  A,
                                  V> : std::true_type
    {
    };

    template <typename A, typename V, typename Enable = void>
    struct has_alloc_destroy : std::false_type
    {
    };

    template <typename A, typename V>
    struct has_alloc_destroy<A, V, std::enable_if_t<is_complete<V>::value>>
        : has_alloc_destroy_impl<void, A, V>
    {
    };

    template <typename A, typename V>
    struct must_use_alloc_destroy
        : bool_constant<!std::is_same_v<A, std::allocator<V>> && has_alloc_destroy<A, V>::value>
    {
    };

  public:
    allocator_interface() = default;
    //    allocator_interface (const allocator_interface&)     = impl;
    allocator_interface(allocator_interface&&) noexcept = default;

    constexpr auto operator=(const allocator_interface&) -> allocator_interface& = default;

    constexpr auto operator=(allocator_interface&&) noexcept -> allocator_interface& = default;

    ~allocator_interface() = default;

    constexpr allocator_interface(const allocator_interface& other) noexcept
        : alloc_base(alloc_traits::select_on_container_copy_construction(other.allocator_ref()))
    {
    }

    constexpr explicit allocator_interface(const alloc_ty& alloc) noexcept
        : alloc_base(alloc)
    {
    }

    template <typename T>
    constexpr explicit allocator_interface(T&&, const alloc_ty& alloc) noexcept
        : allocator_interface(alloc)
    {
    }

    template <typename, typename, typename = void>
    struct is_memcpyable_integral : std::false_type
    {
    };

    template <typename From, typename To>
    struct is_memcpyable_integral<From, To, std::enable_if_t<is_complete<From>::value>>
    {
        using from = underlying_if_enum_t<From>;
        using to   = underlying_if_enum_t<To>;

        static constexpr bool value =
            (sizeof(from) == sizeof(to)) &&
            (std::is_same_v<bool, from> == std::is_same_v<bool, to>)&&std::is_integral_v<from> &&
            std::is_integral_v<to>;
    };

    template <typename From, typename To>
    struct is_convertible_pointer
        : bool_constant<std::is_pointer_v<From> && std::is_pointer_v<To> &&
                        std::is_convertible_v<From, To>>
    {
    };

    // Memcpyable assignment.
    template <typename QualifiedFrom, typename QualifiedTo = value_ty, typename Enable = void>
    struct is_memcpyable : std::false_type
    {
    };

    template <typename QualifiedFrom, typename QualifiedTo>
    struct is_memcpyable<QualifiedFrom,
                         QualifiedTo,
                         std::enable_if_t<is_complete<QualifiedFrom>::value>>
    {
        static_assert(!std::is_reference_v<QualifiedTo>, "QualifiedTo must not be a reference.");

        using from = std::remove_reference_t<std::remove_cv_t<QualifiedFrom>>;

        using to = std::remove_cv_t<QualifiedTo>;

        static constexpr bool value =
            std::is_trivially_assignable_v<QualifiedTo&, QualifiedFrom> &&
            std::is_trivially_copyable_v<to> &&
            (std::is_same_v<std::remove_cv_t<from>, to> ||
             is_memcpyable_integral<from, to>::value || is_convertible_pointer<from, to>::value);
    };

    // Memcpyable construction.
    template <typename QualifiedFrom, typename QualifiedTo>
    struct is_uninitialized_memcpyable_impl
    {
        static_assert(!std::is_reference_v<QualifiedTo>, "QualifiedTo must not be a reference.");

        using from = std::remove_reference_t<std::remove_cv_t<QualifiedFrom>>;

        using to = std::remove_cv_t<QualifiedTo>;

        static constexpr bool value =
            std::is_trivially_constructible_v<QualifiedTo, QualifiedFrom> &&
            std::is_trivially_copyable_v<to> &&
            (std::is_same_v<std::remove_cv_t<from>, to> ||
             is_memcpyable_integral<from, to>::value || is_convertible_pointer<from, to>::value) &&
            (!must_use_alloc_construct<alloc_ty, value_ty, from>::value &&
             !must_use_alloc_destroy<alloc_ty, value_ty>::value);
    };

    template <typename To, typename... Args>
    struct is_uninitialized_memcpyable : std::false_type
    {
    };

    template <typename To, typename From>
    struct is_uninitialized_memcpyable<To, From> : is_uninitialized_memcpyable_impl<From, To>
    {
    };

    template <typename Iterator>
    struct IsListIterator : std::false_type
    {
    };

    template <typename... Ts>
    struct IsListIterator<ListIterator<Ts...>> : std::true_type
    {
    };

    template <typename InputIt>
    struct is_contiguous_iterator
        : bool_constant<std::is_same_v<InputIt, ptr> || std::is_same_v<InputIt, cptr> ||
                        IsListIterator<InputIt>::value || std::contiguous_iterator<InputIt>>
    {
    };

    template <typename InputIt>
    struct is_memcpyable_iterator
        : bool_constant<is_memcpyable<decltype(*std::declval<InputIt>())>::value &&
                        is_contiguous_iterator<InputIt>::value>
    {
    };

    // Unwrap `move_iterator`s.
    template <typename InputIt>
    struct is_memcpyable_iterator<std::move_iterator<InputIt>> : is_memcpyable_iterator<InputIt>
    {
    };

    template <typename InputIt, typename V = value_ty>
    struct is_uninitialized_memcpyable_iterator
        : bool_constant<is_uninitialized_memcpyable<V, decltype(*std::declval<InputIt>())>::value &&
                        is_contiguous_iterator<InputIt>::value>
    {
    };

    // unwrap move_iterators
    template <typename U, typename V>
    struct is_uninitialized_memcpyable_iterator<std::move_iterator<U>, V>
        : is_uninitialized_memcpyable_iterator<U, V>
    {
    };

    [[noreturn]] static constexpr void throw_range_length_error()
    {
        throw std::length_error("The specified range is too long.");
    }

    static constexpr auto to_address(value_ty* p) noexcept -> value_ty*
    {
        static_assert(!std::is_function_v<value_ty>, "value_ty is a function pointer.");
        return p;
    }

    static constexpr auto to_address(const value_ty* p) noexcept -> const value_ty*
    {
        static_assert(!std::is_function_v<value_ty>, "value_ty is a function pointer.");
        return p;
    }

    template <typename Pointer,
              std::enable_if_t<has_ptr_traits_to_address<Pointer>::value>* = nullptr>
    static constexpr auto to_address(const Pointer& p) noexcept
        -> decltype(std::pointer_traits<Pointer>::to_address(p))
    {
        return std::pointer_traits<Pointer>::to_address(p);
    }

    template <typename Pointer,
              std::enable_if_t<!has_ptr_traits_to_address<Pointer>::value>* = nullptr>
    static constexpr auto to_address(const Pointer& p) noexcept
        -> decltype(to_address(p.operator->()))
    {
        return to_address(p.operator->());
    }

    template <typename Integer>
    [[nodiscard]] static consteval auto numeric_max() noexcept -> std::size_t
    {
        static_assert(0 <= (std::numeric_limits<Integer>::max)(), "Integer is nonpositive.");
        return static_cast<std::size_t>((std::numeric_limits<Integer>::max)());
    }

    [[nodiscard]] static constexpr auto internal_range_length(cptr first, cptr last) noexcept
        -> size_ty
    {
        // This is guaranteed to be less than or equal to max size_ty.
        return static_cast<size_ty>(last - first);
    }

    template <typename RandomIt>
    [[nodiscard]] static constexpr auto external_range_length_impl(RandomIt first,
                                                                   RandomIt last,
                                                                   std::random_access_iterator_tag)
        -> size_ty
    {
        assert(0 <= (last - first) && "Invalid range.");
        const auto len = static_cast<std::size_t>(last - first);
#ifndef NDEBUG
        if (numeric_max<size_ty>() < len)
        {
            throw_range_length_error();
        }
#endif
        return static_cast<size_ty>(len);
    }

    template <typename ForwardIt>
    [[nodiscard]] static constexpr auto external_range_length_impl(ForwardIt first,
                                                                   ForwardIt last,
                                                                   std::forward_iterator_tag)
        -> size_ty
    {
        if (std::is_constant_evaluated())
        {
            // Make sure constexpr doesn't get broken by `using namespace std::rel_ops`.
            typename std::iterator_traits<ForwardIt>::difference_type len = 0;
            for (; !(first == last); ++first)
            {
                ++len;
            }
            assert(static_cast<std::size_t>(len) <= numeric_max<size_ty>());
            return static_cast<size_ty>(len);
        }

        const auto len = static_cast<std::size_t>(std::distance(first, last));
#ifndef NDEBUG
        if (numeric_max<size_ty>() < len)
        {
            throw_range_length_error();
        }
#endif
        return static_cast<size_ty>(len);
    }

    template <typename ForwardIt,
              typename ItDiffT = typename std::iterator_traits<ForwardIt>::difference_type,
              std::enable_if_t<(numeric_max<size_ty>() < numeric_max<ItDiffT>()), bool> = true>
    [[nodiscard]] static constexpr auto external_range_length(ForwardIt first, ForwardIt last)
        -> size_ty
    {
        using iterator_cat = typename std::iterator_traits<ForwardIt>::iterator_category;
        return external_range_length_impl(first, last, iterator_cat{});
    }

    template <typename ForwardIt,
              typename ItDiffT = typename std::iterator_traits<ForwardIt>::difference_type,
              std::enable_if_t<!(numeric_max<size_ty>() < numeric_max<ItDiffT>()), bool> = false>
    [[nodiscard]] static constexpr auto external_range_length(ForwardIt first,
                                                              ForwardIt last) noexcept -> size_ty
    {
        if (std::is_constant_evaluated())
        {
            // Make sure constexpr doesn't get broken by `using namespace std::rel_ops`.
            size_ty len = 0;
            for (; !(first == last); ++first)
            {
                ++len;
            }
            return len;
        }

        return static_cast<size_ty>(std::distance(first, last));
    }

    template <typename Iterator,
              typename IteratorDiffT = typename std::iterator_traits<Iterator>::difference_type,
              typename Integer       = IteratorDiffT>
    [[nodiscard]] static constexpr auto unchecked_next(Iterator pos, Integer n = 1) noexcept
        -> Iterator
    {
        unchecked_advance(pos, static_cast<IteratorDiffT>(n));
        return pos;
    }

    template <typename Iterator,
              typename IteratorDiffT = typename std::iterator_traits<Iterator>::difference_type,
              typename Integer       = IteratorDiffT>
    [[nodiscard]] static constexpr auto unchecked_prev(Iterator pos, Integer n = 1) noexcept
        -> Iterator
    {
        unchecked_advance(pos, -static_cast<IteratorDiffT>(n));
        return pos;
    }

    template <typename Iterator,
              typename IteratorDiffT = typename std::iterator_traits<Iterator>::difference_type,
              typename Integer       = IteratorDiffT>
    static constexpr void unchecked_advance(Iterator& pos, Integer n) noexcept
    {
        std::advance(pos, static_cast<IteratorDiffT>(n));
    }

    [[nodiscard]] constexpr auto get_max_size() const noexcept -> size_ty
    {
        // This is protected from max/min macros.
        return (std::min)(static_cast<size_ty>(alloc_traits::max_size(allocator_ref())),
                          static_cast<size_ty>(numeric_max<difference_type>()));
    }

    [[nodiscard]] constexpr auto allocate(size_ty n) -> ptr
    {
        return alloc_traits::allocate(allocator_ref(), static_cast<size_type>(n));
    }

    [[nodiscard]] constexpr auto allocate_with_hint(size_ty n, cptr hint) -> ptr
    {
        return alloc_traits::allocate(allocator_ref(), static_cast<size_type>(n), hint);
    }

    constexpr void deallocate(ptr p, size_ty n)
    {
        alloc_traits::deallocate(allocator_ref(), to_address(p), static_cast<size_type>(n));
    }

    template <typename U,
              std::enable_if_t<is_uninitialized_memcpyable<value_ty, U>::value>* = nullptr>
    constexpr void construct(ptr p, U&& val) noexcept
    {
        if (std::is_constant_evaluated())
        {
            alloc_traits::construct(allocator_ref(), to_address(p), std::forward<U>(val));
            return;
        }
        std::memcpy(to_address(p), &val, sizeof(value_ty));
    }

    // basically alloc_traits::construct
    // all this is so we can replicate C++20 behavior in the other overload
    template <typename A = alloc_ty,
              typename V = value_ty,
              typename... Args,
              std::enable_if_t<(sizeof...(Args) != 1 ||
                                !is_uninitialized_memcpyable<V, Args...>::value) &&
                               has_alloc_construct<A, V, Args...>::value>* = nullptr>
    constexpr void construct(ptr p, Args&&... args) noexcept(noexcept(alloc_traits::construct(
        std::declval<alloc_ty&>(), std::declval<value_ty*>(), std::forward<Args>(args)...)))
    {
        alloc_traits::construct(allocator_ref(), to_address(p), std::forward<Args>(args)...);
    }

    template <typename A = alloc_ty,
              typename V = value_ty,
              typename... Args,
              void_t<std::enable_if_t<(sizeof...(Args) != 1 ||
                                       !is_uninitialized_memcpyable<V, Args...>::value) &&
                                      !has_alloc_construct<A, V, Args...>::value>,
                     decltype(::new(std::declval<void*>()) V(std::declval<Args>()...))>* = nullptr>
    constexpr void construct(ptr p, Args&&... args) noexcept(
        noexcept(::new(std::declval<void*>()) value_ty(std::declval<Args>()...)))
    {
        construct_at(to_address(p), std::forward<Args>(args)...);
    }

    template <typename A                                              = alloc_ty,
              typename V                                              = value_ty,
              std::enable_if_t<is_trivially_destructible<V>::value &&
                               !must_use_alloc_destroy<A, V>::value>* = nullptr>
    constexpr void destroy(ptr) const noexcept
    {
    }

    template <typename A                                        = alloc_ty,
              typename V                                        = value_ty,
              std::enable_if_t<(!is_trivially_destructible<V>::value ||
                                must_use_alloc_destroy<A, V>::value) &&
                               has_alloc_destroy<A, V>::value>* = nullptr>
    constexpr void destroy(ptr p) noexcept
    {
        alloc_traits::destroy(allocator_ref(), to_address(p));
    }

    // defined so we match C++20 behavior in all cases.
    template <typename A                                         = alloc_ty,
              typename V                                         = value_ty,
              std::enable_if_t<(!is_trivially_destructible<V>::value ||
                                must_use_alloc_destroy<A, V>::value) &&
                               !has_alloc_destroy<A, V>::value>* = nullptr>
    constexpr void destroy(ptr p) noexcept
    {
        destroy_at(to_address(p));
    }

    template <typename A                                              = alloc_ty,
              typename V                                              = value_ty,
              std::enable_if_t<is_trivially_destructible<V>::value &&
                               !must_use_alloc_destroy<A, V>::value>* = nullptr>
    constexpr void destroy_range(ptr, ptr) const noexcept
    {
    }

    template <typename A                                             = alloc_ty,
              typename V                                             = value_ty,
              std::enable_if_t<!is_trivially_destructible<V>::value ||
                               must_use_alloc_destroy<A, V>::value>* = nullptr>
    constexpr void destroy_range(ptr first, ptr last) noexcept
    {
        for (; !(first == last); ++first)
        {
            destroy(first);
        }
    }

    // allowed if trivially copyable and we use the standard allocator
    // and InputIt is a contiguous iterator
    template <typename ForwardIt,
              std::enable_if_t<is_uninitialized_memcpyable_iterator<ForwardIt>::value, bool> = true>
    constexpr auto uninitialized_copy(ForwardIt first, ForwardIt last, ptr dest) noexcept -> ptr
    {
        static_assert(std::is_constructible_v<value_ty, decltype(*first)>,
                      "`value_type` must be copy constructible.");

        if (std::is_constant_evaluated())
        {
            return default_uninitialized_copy(first, last, dest);
        }

        const size_ty num_copy = external_range_length(first, last);
        if (num_copy != 0)
        {
            std::memcpy(to_address(dest), to_address(first), num_copy * sizeof(value_ty));
        }
        return unchecked_next(dest, num_copy);
    }

    template <typename ForwardIt,
              std::enable_if_t<is_uninitialized_memcpyable_iterator<ForwardIt>::value, bool> = true>
    constexpr auto uninitialized_copy(std::move_iterator<ForwardIt> first,
                                      std::move_iterator<ForwardIt> last,
                                      ptr                           dest) noexcept -> ptr
    {
        return uninitialized_copy(first.base(), last.base(), dest);
    }

    template <typename InputIt,
              std::enable_if_t<!is_uninitialized_memcpyable_iterator<InputIt>::value, bool> = false>
    constexpr auto uninitialized_copy(InputIt first, InputIt last, ptr d_first) -> ptr
    {
        return default_uninitialized_copy(first, last, d_first);
    }

    template <typename InputIt>
    constexpr auto default_uninitialized_copy(InputIt first, InputIt last, ptr d_first) -> ptr
    {
        ptr d_last = d_first;
        try
        {
            // Note: Not != because `using namespace std::rel_ops` can break constexpr.
            for (; !(first == last); ++first, static_cast<void>(++d_last))
            {
                construct(d_last, *first);
            }
            return d_last;
        }
        catch (...)
        {
            destroy_range(d_first, d_last);
            throw;
        }
    }

    template <typename A                                                = alloc_ty,
              typename V                                                = value_ty,
              std::enable_if_t<is_trivially_constructible<V>::value &&
                               !must_use_alloc_construct<A, V>::value>* = nullptr>
    constexpr auto uninitialized_value_construct(ptr first, ptr last) -> ptr
    {
        if (std::is_constant_evaluated())
        {
            return default_uninitialized_value_construct(first, last);
        }
        std::fill(first, last, value_ty());
        return last;
    }

    template <typename A                                               = alloc_ty,
              typename V                                               = value_ty,
              std::enable_if_t<!is_trivially_constructible<V>::value ||
                               must_use_alloc_construct<A, V>::value>* = nullptr>
    constexpr auto uninitialized_value_construct(ptr first, ptr last) -> ptr
    {
        return default_uninitialized_value_construct(first, last);
    }

    constexpr auto default_uninitialized_value_construct(ptr first, ptr last) -> ptr
    {
        ptr curr = first;
        try
        {
            for (; !(curr == last); ++curr)
                construct(curr);
            return curr;
        }
        catch (...)
        {
            destroy_range(first, curr);
            throw;
        }
    }

    constexpr auto uninitialized_fill(ptr first, ptr last) -> ptr
    {
        return uninitialized_value_construct(first, last);
    }

    constexpr auto uninitialized_fill(ptr first, ptr last, const value_ty& val) -> ptr
    {
        ptr curr = first;
        try
        {
            for (; !(curr == last); ++curr)
            {
                construct(curr, val);
            }
            return curr;
        }
        catch (...)
        {
            destroy_range(first, curr);
            throw;
        }
    }

  private:
    // If value_ty is an array, replicate C++20 behavior (I don't think that value_ty can
    // actually be an array because of the Erasable requirement, but there shouldn't
    // be any runtime cost for being defensive here).
    template <typename V = value_ty, std::enable_if_t<std::is_array_v<V>, bool> = true>
    static constexpr void destroy_at(value_ty* p) noexcept
    {
        for (auto& e : *p)
        {
            destroy_at(std::addressof(e));
        }
    }

    template <typename V = value_ty, std::enable_if_t<!std::is_array_v<V>, bool> = false>
    static constexpr void destroy_at(value_ty* p) noexcept
    {
        p->~value_ty();
    }

    template <typename V = value_ty, typename... Args>
    static constexpr auto construct_at(value_ty* p, Args&&... args) noexcept(
        noexcept(::new(std::declval<void*>()) V(std::declval<Args>()...)))
        -> decltype(::new(std::declval<void*>()) V(std::declval<Args>()...))
    {
        if (std::is_constant_evaluated())
        {
            return std::construct_at(p, std::forward<Args>(args)...);
        }
        auto* vp = const_cast<void*>(static_cast<const volatile void*>(p));
        return ::new (vp) value_ty(std::forward<Args>(args)...);
    }
};

template <typename Pointer, typename SizeT>
class ListDataBase
{
  public:
    using ptr     = Pointer;
    using size_ty = SizeT;

         ListDataBase()                                      = default;
         ListDataBase(const ListDataBase&)                   = default;
         ListDataBase(ListDataBase&&) noexcept               = default;
    auto operator=(const ListDataBase&) -> ListDataBase&     = default;
    auto operator=(ListDataBase&&) noexcept -> ListDataBase& = default;
    ~    ListDataBase()                                      = default;

    constexpr auto data_ptr() const noexcept -> ptr
    {
        return m_data_ptr;
    }
    constexpr auto capacity() const noexcept -> size_ty
    {
        return m_capacity;
    }
    constexpr auto size() const noexcept -> size_ty
    {
        return m_size;
    }

    constexpr void set_data_ptr(ptr data_ptr) noexcept
    {
        m_data_ptr = data_ptr;
    }
    constexpr void set_capacity(size_ty capacity) noexcept
    {
        m_capacity = capacity;
    }
    constexpr void set_size(size_ty size) noexcept
    {
        m_size = size;
    }

    constexpr void set(ptr data_ptr, size_ty capacity, size_ty size)
    {
        m_data_ptr = data_ptr;
        m_capacity = capacity;
        m_size     = size;
    }

    constexpr void swap_data_ptr(ListDataBase& other) noexcept
    {
        using std::swap;
        swap(m_data_ptr, other.m_data_ptr);
    }

    constexpr void swap_capacity(ListDataBase& other) noexcept
    {
        using std::swap;
        swap(m_capacity, other.m_capacity);
    }

    constexpr void swap_size(ListDataBase& other) noexcept
    {
        using std::swap;
        swap(m_size, other.m_size);
    }

    constexpr void swap(ListDataBase& other) noexcept
    {
        using std::swap;
        swap(m_data_ptr, other.m_data_ptr);
        swap(m_capacity, other.m_capacity);
        swap(m_size, other.m_size);
    }

  private:
    ptr     m_data_ptr;
    size_ty m_capacity;
    size_ty m_size;
};

template <typename Pointer, typename SizeT, typename T, unsigned InlineCapacity>
class ListData : public ListDataBase<Pointer, SizeT>
{
  public:
         ListData()                                  = default;
         ListData(const ListData&)                   = delete;
         ListData(ListData&&) noexcept               = delete;
    auto operator=(const ListData&) -> ListData&     = delete;
    auto operator=(ListData&&) noexcept -> ListData& = delete;
    ~    ListData()                                  = default;

    constexpr auto storage() noexcept -> T*
    {
        return m_storage.get_inline_ptr();
    }

  private:
    inline_storage<T, InlineCapacity> m_storage;
};

template <typename Pointer, typename SizeT, typename T>
class
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023918L
    __declspec(empty_bases)
#endif
        ListData<Pointer, SizeT, T, 0> : public ListDataBase<Pointer, SizeT>
{
  public:
         ListData()                                  = default;
         ListData(const ListData&)                   = delete;
         ListData(ListData&&) noexcept               = delete;
    auto operator=(const ListData&) -> ListData&     = delete;
    auto operator=(ListData&&) noexcept -> ListData& = delete;
    ~    ListData()                                  = default;

    constexpr auto storage() noexcept -> T*
    {
        return nullptr;
    }
};

template <typename Allocator, unsigned InlineCapacity>
class ListBase : public allocator_interface<Allocator>
{
  public:
    using size_type       = typename allocator_interface<Allocator>::size_type;
    using difference_type = typename allocator_interface<Allocator>::difference_type;

    template <typename SameAllocator, unsigned DifferentInlineCapacity>
    friend class ListBase;

  protected:
    using alloc_interface = allocator_interface<Allocator>;
    using alloc_traits    = typename alloc_interface::alloc_traits;
    using alloc_ty        = Allocator;

    using value_ty = typename alloc_interface::value_ty;
    using ptr      = typename alloc_interface::ptr;
    using cptr     = typename alloc_interface::cptr;
    using size_ty  = typename alloc_interface::size_ty;
    using diff_ty  = typename alloc_interface::diff_ty;

    static_assert(alloc_interface::template is_complete<value_ty>::value || InlineCapacity == 0,
                  "`value_type` must be complete for instantiation of a non-zero number "
                  "of inline elements.");

    template <typename T>
    using is_complete = typename alloc_interface::template is_complete<T>;

    using alloc_interface::allocator_ref;
    using alloc_interface::construct;
    using alloc_interface::deallocate;
    using alloc_interface::destroy;
    using alloc_interface::destroy_range;
    using alloc_interface::external_range_length;
    using alloc_interface::get_max_size;
    using alloc_interface::internal_range_length;
    using alloc_interface::to_address;
    using alloc_interface::unchecked_advance;
    using alloc_interface::unchecked_next;
    using alloc_interface::unchecked_prev;
    using alloc_interface::uninitialized_copy;
    using alloc_interface::uninitialized_fill;
    using alloc_interface::uninitialized_value_construct;

    template <typename Integer>
    [[nodiscard]] static consteval auto numeric_max() noexcept -> std::size_t
    {
        return alloc_interface::template numeric_max<Integer>();
    }

    [[nodiscard]] static consteval auto get_inline_capacity() noexcept -> size_ty
    {
        return static_cast<size_ty>(InlineCapacity);
    }

    template <typename...>
    using void_t = void;

    template <bool B>
    using bool_constant = std::integral_constant<bool, B>;

    template <typename Void, typename AI, typename V, typename... Args>
    struct is_emplace_constructible_impl : std::false_type
    {
        using nothrow = std::false_type;
    };

    template <typename AI, typename V, typename... Args>
    struct is_emplace_constructible_impl<void_t<std::enable_if_t<is_complete<V>::value>,
                                                decltype(std::declval<AI&>().construct(
                                                    std::declval<V*>(), std::declval<Args>()...))>,
                                         AI,
                                         V,
                                         Args...> : std::true_type
    {
        using nothrow = bool_constant<noexcept(
            std::declval<AI&>().construct(std::declval<V*>(), std::declval<Args>()...))>;
    };

    template <typename... Args>
    struct is_emplace_constructible
        : is_emplace_constructible_impl<void, alloc_interface, value_ty, Args...>
    {
    };

    template <typename... Args>
    struct is_nothrow_emplace_constructible
        : is_emplace_constructible_impl<void, alloc_interface, value_ty, Args...>::nothrow
    {
    };

    template <typename V = value_ty>
    struct is_explicitly_move_insertable : is_emplace_constructible<V&&>
    {
    };

    template <typename V = value_ty>
    struct is_explicitly_nothrow_move_insertable : is_nothrow_emplace_constructible<V&&>
    {
    };

    template <typename V = value_ty>
    struct is_explicitly_copy_insertable
        : std::integral_constant<bool,
                                 is_emplace_constructible<V&>::value &&
                                     is_emplace_constructible<const V&>::value>
    {
    };

    template <typename V = value_ty>
    struct is_explicitly_nothrow_copy_insertable
        : std::integral_constant<bool,
                                 is_nothrow_emplace_constructible<V&>::value &&
                                     is_nothrow_emplace_constructible<const V&>::value>
    {
    };

    template <typename AI, typename Enable = void>
    struct is_eraseable : std::false_type
    {
    };

    template <typename AI>
    struct is_eraseable<AI,
                        void_t<decltype(std::declval<AI&>().destroy(std::declval<value_ty*>()))>>
        : std::true_type
    {
    };

    template <typename V>
    struct relocate_with_move : bool_constant<std::is_nothrow_move_constructible_v<V> ||
                                              !is_explicitly_copy_insertable<V>::value>
    {
    };

    template <typename A>
    struct allocations_are_movable
        : bool_constant<std::is_same_v<std::allocator<value_ty>, A> ||
                        std::allocator_traits<A>::propagate_on_container_move_assignment::value ||
                        std::allocator_traits<A>::is_always_equal::value>
    {
    };

    template <typename A>
    struct allocations_are_swappable
        : bool_constant<std::is_same_v<std::allocator<value_ty>, A> ||
                        std::allocator_traits<A>::propagate_on_container_swap::value ||
                        std::allocator_traits<A>::is_always_equal::value>
    {
    };

    template <typename... Args>
    using is_memcpyable = typename alloc_interface::template is_memcpyable<Args...>;

    template <typename... Args>
    using is_memcpyable_iterator =
        typename alloc_interface::template is_memcpyable_iterator<Args...>;

    [[noreturn]] static constexpr void throw_overflow_error()
    {
        throw std::overflow_error("The requested conversion would overflow.");
    }

    [[noreturn]] static constexpr void throw_index_error()
    {
        throw std::out_of_range("The requested index was out of range.");
    }

    [[noreturn]] static constexpr void throw_increment_error()
    {
        throw std::domain_error("The requested increment was outside of the allowed range.");
    }

    [[noreturn]] static constexpr void throw_allocation_size_error()
    {
        throw std::length_error("The required allocation exceeds the maximum size.");
    }

    [[nodiscard]] constexpr auto ptr_cast(const ListIterator<cptr, diff_ty>& it) noexcept -> ptr
    {
        return unchecked_next(begin_ptr(), it.base() - begin_ptr());
    }

  private:
    class stack_temporary
    {
      public:
             stack_temporary()                                         = delete;
             stack_temporary(const stack_temporary&)                   = delete;
             stack_temporary(stack_temporary&&) noexcept               = delete;
        auto operator=(const stack_temporary&) -> stack_temporary&     = delete;
        auto operator=(stack_temporary&&) noexcept -> stack_temporary& = delete;
        //      ~stack_temporary           ()                       = impl;

        template <typename... Args>
        constexpr explicit stack_temporary(alloc_interface& alloc_iface, Args&&... args)
            : m_interface(alloc_iface)
        {
            m_interface.construct(get_pointer(), std::forward<Args>(args)...);
        }

        constexpr ~stack_temporary()
        {
            m_interface.destroy(get_pointer());
        }

        [[nodiscard]] constexpr auto get() const noexcept -> const value_ty&
        {
            return *get_pointer();
        }

        [[nodiscard]] constexpr auto release() noexcept -> value_ty&&
        {
            return std::move(*get_pointer());
        }

      private:
        [[nodiscard]] constexpr auto get_pointer() const noexcept -> cptr
        {
            return static_cast<cptr>(static_cast<const void*>(std::addressof(m_data)));
        }

        [[nodiscard]] constexpr auto get_pointer() noexcept -> ptr
        {
            return static_cast<ptr>(static_cast<void*>(std::addressof(m_data)));
        }

        alloc_interface& m_interface;
        alignas(alignof(value_ty)) unsigned char m_data[sizeof(value_ty)];
    };

    class heap_temporary
    {
      public:
             heap_temporary()                                        = delete;
             heap_temporary(const heap_temporary&)                   = delete;
             heap_temporary(heap_temporary&&) noexcept               = delete;
        auto operator=(const heap_temporary&) -> heap_temporary&     = delete;
        auto operator=(heap_temporary&&) noexcept -> heap_temporary& = delete;
        //      ~heap_temporary           ()                      = impl;

        template <typename... Args>
        constexpr explicit heap_temporary(alloc_interface& alloc_iface, Args&&... args)
            : m_interface(alloc_iface)
            , m_data_ptr(alloc_iface.allocate(sizeof(value_ty)))
        {
            try
            {
                m_interface.construct(m_data_ptr, std::forward<Args>(args)...);
            }
            catch (...)
            {
                m_interface.deallocate(m_data_ptr, sizeof(value_ty));
                throw;
            }
        }

        constexpr ~heap_temporary()
        {
            m_interface.destroy(m_data_ptr);
            m_interface.deallocate(m_data_ptr, sizeof(value_ty));
        }

        [[nodiscard]] constexpr auto get() const noexcept -> const value_ty&
        {
            return *m_data_ptr;
        }

        [[nodiscard]] constexpr auto release() noexcept -> value_ty&&
        {
            return std::move(*m_data_ptr);
        }

      private:
        alloc_interface& m_interface;
        ptr              m_data_ptr;
    };

    constexpr void wipe()
    {
        destroy_range(begin_ptr(), end_ptr());
        if (has_allocation())
        {
            deallocate(data_ptr(), get_capacity());
        }
    }

    constexpr void set_data_ptr(ptr data_ptr) noexcept
    {
        m_data.set_data_ptr(data_ptr);
    }

    constexpr void set_capacity(size_ty capacity) noexcept
    {
        m_data.set_capacity(static_cast<size_type>(capacity));
    }

    constexpr void set_size(size_ty size) noexcept
    {
        m_data.set_size(static_cast<size_type>(size));
    }

    constexpr void set_data(ptr data_ptr, size_ty capacity, size_ty size) noexcept
    {
        m_data.set(data_ptr, static_cast<size_type>(capacity), static_cast<size_type>(size));
    }

    constexpr void swap_data_ptr(ListBase& other) noexcept
    {
        m_data.swap_data_ptr(other.m_data);
    }

    constexpr void swap_capacity(ListBase& other) noexcept
    {
        m_data.swap_capacity(other.m_data);
    }

    constexpr void swap_size(ListBase& other) noexcept
    {
        m_data.swap_size(other.m_data);
    }

    constexpr void swap_allocation(ListBase& other) noexcept
    {
        m_data.swap(other.m_data);
    }

    constexpr void reset_data(ptr data_ptr, size_ty capacity, size_ty size)
    {
        wipe();
        m_data.set(data_ptr, static_cast<size_type>(capacity), static_cast<size_type>(size));
    }

    constexpr void increase_size(size_ty n) noexcept
    {
        m_data.set_size(get_size() + n);
    }

    constexpr void decrease_size(size_ty n) noexcept
    {
        m_data.set_size(get_size() - n);
    }

    constexpr auto unchecked_allocate(size_ty n) -> ptr
    {
        assert(InlineCapacity < n && "Allocated capacity should be greater than InlineCapacity.");
        return alloc_interface::allocate(n);
    }

    constexpr auto unchecked_allocate(size_ty n, cptr hint) -> ptr
    {
        assert(InlineCapacity < n && "Allocated capacity should be greater than InlineCapacity.");
        return alloc_interface::allocate_with_hint(n, hint);
    }

    constexpr auto checked_allocate(size_ty n) -> ptr
    {
        if (get_max_size() < n)
        {
            throw_allocation_size_error();
        }
        return unchecked_allocate(n);
    }

  protected:
    [[nodiscard]] constexpr auto unchecked_calculate_new_capacity(
        const size_ty minimum_required_capacity) const noexcept -> size_ty
    {
        const size_ty current_capacity = get_capacity();

        assert(current_capacity < minimum_required_capacity);

        if (get_max_size() - current_capacity <= current_capacity)
            return get_max_size();

        // Note: This growth factor might be theoretically superior, but in testing it falls flat:
        // size_ty new_capacity = current_capacity + (current_capacity / 2);

        const size_ty new_capacity = 2 * current_capacity;
        if (new_capacity < minimum_required_capacity)
            return minimum_required_capacity;
        return new_capacity;
    }

    [[nodiscard]] constexpr auto checked_calculate_new_capacity(
        const size_ty minimum_required_capacity) const -> size_ty
    {
        if (get_max_size() < minimum_required_capacity)
        {
            throw_allocation_size_error();
        }
        return unchecked_calculate_new_capacity(minimum_required_capacity);
    }

    template <unsigned I>
    constexpr auto copy_assign_default(const ListBase<Allocator, I>& other) -> ListBase&
    {
        if (get_capacity() < other.get_size())
        {
            // Reallocate.
            size_ty new_capacity = unchecked_calculate_new_capacity(other.get_size());
            ptr     new_data_ptr = unchecked_allocate(new_capacity, other.allocation_end_ptr());

            try
            {
                uninitialized_copy(other.begin_ptr(), other.end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, other.get_size());
        }
        else
        {
            if (get_size() < other.get_size())
            {
                // No reallocation, partially in uninitialized space.
                std::copy_n(other.begin_ptr(), get_size(), begin_ptr());
                uninitialized_copy(unchecked_next(other.begin_ptr(), get_size()),
                                   other.end_ptr(),
                                   end_ptr());
            }
            else
            {
                destroy_range(copy_range(other.begin_ptr(), other.end_ptr(), begin_ptr()),
                              end_ptr());
            }

            // data_ptr and capacity do not change in this case.
            set_size(other.get_size());
        }

        alloc_interface::operator=(other);
        return *this;
    }

    template <unsigned I,
              typename AT                                    = alloc_traits,
              std::enable_if_t<AT::propagate_on_container_copy_assignment::value &&
                               !AT::is_always_equal::value>* = nullptr>
    constexpr auto copy_assign(const ListBase<Allocator, I>& other) -> ListBase&
    {
        if (other.allocator_ref() == allocator_ref())
        {
            return copy_assign_default(other);
        }

        if (InlineCapacity < other.get_size())
        {
            alloc_interface new_alloc(other);

            const size_ty new_capacity = other.get_size();
            const ptr     new_data_ptr =
                new_alloc.allocate_with_hint(new_capacity, other.allocation_end_ptr());

            try
            {
                uninitialized_copy(other.begin_ptr(), other.end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                new_alloc.deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, other.get_size());
            alloc_interface::operator=(new_alloc);
        }
        else
        {
            if (has_allocation())
            {
                ptr new_data_ptr;
                if (std::is_constant_evaluated())
                {
                    alloc_interface new_alloc(other);
                    new_data_ptr = new_alloc.allocate(InlineCapacity);
                }
                else
                {
                    new_data_ptr = storage_ptr();
                }

                uninitialized_copy(other.begin_ptr(), other.end_ptr(), new_data_ptr);
                destroy_range(begin_ptr(), end_ptr());
                deallocate(data_ptr(), get_capacity());
                set_data_ptr(new_data_ptr);
                set_capacity(InlineCapacity);
            }
            else if (get_size() < other.get_size())
            {
                std::copy_n(other.begin_ptr(), get_size(), begin_ptr());
                uninitialized_copy(unchecked_next(other.begin_ptr(), get_size()),
                                   other.end_ptr(),
                                   end_ptr());
            }
            else
            {
                destroy_range(copy_range(other.begin_ptr(), other.end_ptr(), begin_ptr()),
                              end_ptr());
            }
            set_size(other.get_size());
            alloc_interface::operator=(other);
        }

        return *this;
    }

    template <unsigned I,
              typename AT                                   = alloc_traits,
              std::enable_if_t<!AT::propagate_on_container_copy_assignment::value ||
                               AT::is_always_equal::value>* = nullptr>
    constexpr auto copy_assign(const ListBase<Allocator, I>& other) -> ListBase&
    {
        return copy_assign_default(other);
    }

    template <unsigned I>
    constexpr void move_allocation_pointer(ListBase<alloc_ty, I>&& other) noexcept
    {
        reset_data(other.data_ptr(), other.get_capacity(), other.get_size());
        other.set_default();
    }

    template <unsigned N = InlineCapacity, std::enable_if_t<N == 0>* = nullptr>
    constexpr auto move_assign_default(ListBase&& other) noexcept -> ListBase&
    {
        move_allocation_pointer(std::move(other));
        alloc_interface::operator=(std::move(other));
        return *this;
    }

    template <unsigned LessEqualI, std::enable_if_t<(LessEqualI <= InlineCapacity)>* = nullptr>
    constexpr auto move_assign_default(ListBase<Allocator, LessEqualI>&& other) noexcept(
        std::is_nothrow_move_assignable_v<value_ty> &&
        std::is_nothrow_move_constructible_v<value_ty>) -> ListBase&
    {
        // We only move the allocation pointer over if it has strictly greater capacity than
        // the inline capacity of `*this` because allocations can never have a smaller capacity
        // than the inline capacity.
        if (InlineCapacity < other.get_capacity())
        {
            move_allocation_pointer(std::move(other));
        }
        else
        {
            // We are guaranteed to have sufficient capacity to store the elements.
            if (InlineCapacity < get_capacity())
            {
                ptr new_data_ptr;
                if (std::is_constant_evaluated())
                {
                    new_data_ptr = other.allocate(InlineCapacity);
                }
                else
                {
                    new_data_ptr = storage_ptr();
                }

                uninitialized_move(other.begin_ptr(), other.end_ptr(), new_data_ptr);
                destroy_range(begin_ptr(), end_ptr());
                deallocate(data_ptr(), get_capacity());
                set_data_ptr(new_data_ptr);
                set_capacity(InlineCapacity);
            }
            else if (get_size() < other.get_size())
            {
                // There are more elements in `other`.
                // Overwrite the existing range and uninitialized move the rest.
                ptr other_pivot = unchecked_next(other.begin_ptr(), get_size());
                std::move(other.begin_ptr(), other_pivot, begin_ptr());
                uninitialized_move(other_pivot, other.end_ptr(), end_ptr());
            }
            else
            {
                // There are the same number or fewer elements in `other`.
                // Overwrite part of the existing range and destroy the rest.
                ptr new_end = std::move(other.begin_ptr(), other.end_ptr(), begin_ptr());
                destroy_range(new_end, end_ptr());
            }

            set_size(other.get_size());

            // Note: We do not need to deallocate any allocations in `other` because the value of
            //       an object meeting the Allocator named requirements does not change value after
            //       a move.
        }

        alloc_interface::operator=(std::move(other));
        return *this;
    }

    template <unsigned GreaterI, std::enable_if_t<(InlineCapacity < GreaterI)>* = nullptr>
    constexpr auto move_assign_default(ListBase<Allocator, GreaterI>&& other) -> ListBase&
    {
        if (other.has_allocation())
        {
            move_allocation_pointer(std::move(other));
        }
        else if (get_capacity() < other.get_size() ||
                 (has_allocation() && !(other.allocator_ref() == allocator_ref())))
        {
            // Reallocate.

            // The compiler should be able to optimize this.
            size_ty new_capacity = get_capacity() < other.get_size()
                                       ? unchecked_calculate_new_capacity(other.get_size())
                                       : get_capacity();

            ptr new_data_ptr = other.allocate_with_hint(new_capacity, other.allocation_end_ptr());

            try
            {
                uninitialized_move(other.begin_ptr(), other.end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                other.deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, other.get_size());
        }
        else
        {
            if (get_size() < other.get_size())
            {
                // There are more elements in `other`.
                // Overwrite the existing range and uninitialized move the rest.
                ptr other_pivot = unchecked_next(other.begin_ptr(), get_size());
                std::move(other.begin_ptr(), other_pivot, begin_ptr());
                uninitialized_move(other_pivot, other.end_ptr(), end_ptr());
            }
            else
            {
                // fewer elements in other
                // overwrite part of the existing range and destroy the rest
                ptr new_end = std::move(other.begin_ptr(), other.end_ptr(), begin_ptr());
                destroy_range(new_end, end_ptr());
            }

            // `data_ptr` and `capacity` do not change in this case.
            set_size(other.get_size());
        }

        alloc_interface::operator=(std::move(other));
        return *this;
    }

    template <unsigned I>
    constexpr auto move_assign_unequal_no_propagate(ListBase<Allocator, I>&& other) -> ListBase&
    {
        if (get_capacity() < other.get_size())
        {
            // Reallocate.
            size_ty new_capacity = unchecked_calculate_new_capacity(other.get_size());
            ptr     new_data_ptr = unchecked_allocate(new_capacity, other.allocation_end_ptr());

            try
            {
                uninitialized_move(other.begin_ptr(), other.end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, other.get_size());
        }
        else
        {
            if (get_size() < other.get_size())
            {
                // There are more elements in `other`.
                // Overwrite the existing range and uninitialized move the rest.
                ptr other_pivot = unchecked_next(other.begin_ptr(), get_size());
                std::move(other.begin_ptr(), other_pivot, begin_ptr());
                uninitialized_move(other_pivot, other.end_ptr(), end_ptr());
            }
            else
            {
                // There are fewer elements in `other`.
                // Overwrite part of the existing range and destroy the rest.
                destroy_range(std::move(other.begin_ptr(), other.end_ptr(), begin_ptr()),
                              end_ptr());
            }

            // data_ptr and capacity do not change in this case
            set_size(other.get_size());
        }

        alloc_interface::operator=(std::move(other));
        return *this;
    }

    template <unsigned I,
              typename A                                           = alloc_ty,
              std::enable_if_t<allocations_are_movable<A>::value>* = nullptr>
    constexpr auto move_assign(ListBase<Allocator, I>&& other) noexcept(
        noexcept(std::declval<ListBase&>().move_assign_default(std::move(other)))) -> ListBase&
    {
        return move_assign_default(std::move(other));
    }

    template <unsigned I,
              typename A                                            = alloc_ty,
              std::enable_if_t<!allocations_are_movable<A>::value>* = nullptr>
    constexpr auto move_assign(ListBase<Allocator, I>&& other) -> ListBase&
    {
        if (other.allocator_ref() == allocator_ref())
            return move_assign_default(std::move(other));
        return move_assign_unequal_no_propagate(std::move(other));
    }

    template <unsigned I = InlineCapacity, std::enable_if_t<I == 0>* = nullptr>
    constexpr void move_initialize(ListBase&& other) noexcept
    {
        set_data(other.data_ptr(), other.get_capacity(), other.get_size());
        other.set_default();
    }

    template <unsigned LessEqualI, std::enable_if_t<(LessEqualI <= InlineCapacity)>* = nullptr>
    constexpr void move_initialize(ListBase<Allocator, LessEqualI>&& other) noexcept(
        std::is_nothrow_move_constructible_v<value_ty>)
    {
        if (InlineCapacity < other.get_capacity())
        {
            set_data(other.data_ptr(), other.get_capacity(), other.get_size());
            other.set_default();
        }
        else
        {
            set_to_inline_storage();
            uninitialized_move(other.begin_ptr(), other.end_ptr(), data_ptr());
            set_size(other.get_size());
        }
    }

    template <unsigned GreaterI,
              typename std::enable_if<(InlineCapacity < GreaterI)>::type* = nullptr>
    constexpr void move_initialize(ListBase<Allocator, GreaterI>&& other)
    {
        if (other.has_allocation())
        {
            set_data(other.data_ptr(), other.get_capacity(), other.get_size());
            other.set_default();
        }
        else
        {
            if (InlineCapacity < other.get_size())
            {
                // We may throw in this case.
                set_data_ptr(unchecked_allocate(other.get_size(), other.allocation_end_ptr()));
                set_capacity(other.get_size());

                try
                {
                    uninitialized_move(other.begin_ptr(), other.end_ptr(), data_ptr());
                }
                catch (...)
                {
                    deallocate(data_ptr(), get_capacity());
                    throw;
                }
            }
            else
            {
                set_to_inline_storage();
                uninitialized_move(other.begin_ptr(), other.end_ptr(), data_ptr());
            }

            set_size(other.get_size());
        }
    }

  public:
         ListBase(const ListBase&)                   = delete;
         ListBase(ListBase&&) noexcept               = delete;
    auto operator=(const ListBase&) -> ListBase&     = delete;
    auto operator=(ListBase&&) noexcept -> ListBase& = delete;

    constexpr ListBase() noexcept
    {
        set_default();
    }

    static constexpr struct bypass_tag
    {
    } bypass{};

    template <unsigned I, typename... MaybeAlloc>
    constexpr ListBase(bypass_tag, const ListBase<Allocator, I>& other, const MaybeAlloc&... alloc)
        : alloc_interface(other, alloc...)
    {
        if (InlineCapacity < other.get_size())
        {
            set_data_ptr(unchecked_allocate(other.get_size(), other.allocation_end_ptr()));
            set_capacity(other.get_size());

            try
            {
                uninitialized_copy(other.begin_ptr(), other.end_ptr(), data_ptr());
            }
            catch (...)
            {
                deallocate(data_ptr(), get_capacity());
                throw;
            }
        }
        else
        {
            set_to_inline_storage();
            uninitialized_copy(other.begin_ptr(), other.end_ptr(), data_ptr());
        }

        set_size(other.get_size());
    }

    template <unsigned I>
    constexpr ListBase(bypass_tag, ListBase<Allocator, I>&& other) noexcept(
        std::is_nothrow_move_constructible_v<value_ty> || (I == 0 && I == InlineCapacity))
        : alloc_interface(std::move(other))
    {
        move_initialize(std::move(other));
    }

    template <unsigned I,
              typename A                                                          = alloc_ty,
              std::enable_if_t<std::is_same_v<std::allocator<value_ty>, A> ||
                               std::allocator_traits<A>::is_always_equal::value>* = nullptr>
    constexpr ListBase(bypass_tag,
                       ListBase<Allocator, I>&& other,
                       const alloc_ty&) noexcept(noexcept(ListBase(bypass, std::move(other))))
        : ListBase(bypass, std::move(other))
    {
    }

    template <unsigned I,
              typename A                                                             = alloc_ty,
              std::enable_if_t<!(std::is_same_v<std::allocator<value_ty>, A> ||
                                 std::allocator_traits<A>::is_always_equal::value)>* = nullptr>
    constexpr ListBase(bypass_tag, ListBase<Allocator, I>&& other, const alloc_ty& alloc)
        : alloc_interface(alloc)
    {
        if (other.allocator_ref() == alloc)
        {
            move_initialize(std::move(other));
            return;
        }

        if (InlineCapacity < other.get_size())
        {
            // We may throw in this case.
            set_data_ptr(unchecked_allocate(other.get_size(), other.allocation_end_ptr()));
            set_capacity(other.get_size());

            try
            {
                uninitialized_move(other.begin_ptr(), other.end_ptr(), data_ptr());
            }
            catch (...)
            {
                deallocate(data_ptr(), get_capacity());
                throw;
            }
        }
        else
        {
            set_to_inline_storage();
            uninitialized_move(other.begin_ptr(), other.end_ptr(), data_ptr());
        }

        set_size(other.get_size());
    }

    constexpr explicit ListBase(const alloc_ty& alloc) noexcept
        : alloc_interface(alloc)
    {
        set_default();
    }

    constexpr ListBase(size_ty count, const alloc_ty& alloc)
        : alloc_interface(alloc)
    {
        if (InlineCapacity < count)
        {
            set_data_ptr(checked_allocate(count));
            set_capacity(count);
        }
        else
        {
            set_to_inline_storage();
        }

        try
        {
            uninitialized_value_construct(begin_ptr(), unchecked_next(begin_ptr(), count));
        }
        catch (...)
        {
            if (has_allocation())
            {
                deallocate(data_ptr(), get_capacity());
            }
            throw;
        }
        set_size(count);
    }

    constexpr ListBase(size_ty count, const value_ty& val, const alloc_ty& alloc)
        : alloc_interface(alloc)
    {
        if (InlineCapacity < count)
        {
            set_data_ptr(checked_allocate(count));
            set_capacity(count);
        }
        else
        {
            set_to_inline_storage();
        }

        try
        {
            uninitialized_fill(begin_ptr(), unchecked_next(begin_ptr(), count), val);
        }
        catch (...)
        {
            if (has_allocation())
            {
                deallocate(data_ptr(), get_capacity());
            }
            throw;
        }
        set_size(count);
    }

    template <typename Generator>
    constexpr ListBase(size_ty count, Generator& g, const alloc_ty& alloc)
        : alloc_interface(alloc)
    {
        if (InlineCapacity < count)
        {
            set_data_ptr(checked_allocate(count));
            set_capacity(count);
        }
        else
        {
            set_to_inline_storage();
        }

        ptr       curr    = begin_ptr();
        const ptr new_end = unchecked_next(begin_ptr(), count);
        try
        {
            for (; !(curr == new_end); ++curr)
            {
                construct(curr, g());
            }
        }
        catch (...)
        {
            destroy_range(begin_ptr(), curr);
            if (has_allocation())
            {
                deallocate(data_ptr(), get_capacity());
            }
            throw;
        }
        set_size(count);
    }

    template <std::input_iterator InputIt>
    constexpr ListBase(InputIt first, InputIt last, std::input_iterator_tag, const alloc_ty& alloc)
        : ListBase(alloc)
    {
        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;
        append_range(first, last, iterator_cat{});
    }

    template <std::forward_iterator ForwardIt>
    constexpr ListBase(ForwardIt first,
                       ForwardIt last,
                       std::forward_iterator_tag,
                       const alloc_ty& alloc)
        : alloc_interface(alloc)
    {
        size_ty count = external_range_length(first, last);
        if (InlineCapacity < count)
        {
            set_data_ptr(unchecked_allocate(count));
            set_capacity(count);
            try
            {
                uninitialized_copy(first, last, begin_ptr());
            }
            catch (...)
            {
                deallocate(data_ptr(), get_capacity());
                throw;
            }
        }
        else
        {
            set_to_inline_storage();
            uninitialized_copy(first, last, begin_ptr());
        }

        set_size(count);
    }

    constexpr ~ListBase() noexcept
    {
        assert(InlineCapacity <= get_capacity() && "Invalid capacity.");
        wipe();
    }

  protected:
    constexpr void set_to_inline_storage()
    {
        set_capacity(InlineCapacity);
        if (std::is_constant_evaluated())
        {
            return set_data_ptr(alloc_interface::allocate(InlineCapacity));
        }
        set_data_ptr(storage_ptr());
    }

    constexpr void assign_with_copies(size_ty count, const value_ty& val)
    {
        if (get_capacity() < count)
        {
            size_ty new_capacity = checked_calculate_new_capacity(count);
            ptr     new_begin    = unchecked_allocate(new_capacity);

            try
            {
                uninitialized_fill(new_begin, unchecked_next(new_begin, count), val);
            }
            catch (...)
            {
                deallocate(new_begin, new_capacity);
                throw;
            }

            reset_data(new_begin, new_capacity, count);
        }
        else if (get_size() < count)
        {
            std::fill(begin_ptr(), end_ptr(), val);
            uninitialized_fill(end_ptr(), unchecked_next(begin_ptr(), count), val);
            set_size(count);
        }
        else
        {
            erase_range(std::fill_n(begin_ptr(), count, val), end_ptr());
        }
    }

    template <typename InputIt,
              std::enable_if_t<
                  std::is_assignable_v<value_ty&, decltype(*std::declval<InputIt>())>>* = nullptr>
    constexpr void assign_with_range(InputIt first, InputIt last, std::input_iterator_tag)
    {
        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;

        ptr curr = begin_ptr();
        for (; !(end_ptr() == curr || first == last); ++curr, static_cast<void>(++first))
        {
            *curr = *first;
        }

        if (first == last)
        {
            erase_to_end(curr);
        }
        else
        {
            append_range(first, last, iterator_cat{});
        }
    }

    template <typename ForwardIt,
              std::enable_if_t<
                  std::is_assignable_v<value_ty&, decltype(*std::declval<ForwardIt>())>>* = nullptr>
    constexpr void assign_with_range(ForwardIt first, ForwardIt last, std::forward_iterator_tag)
    {
        const size_ty count = external_range_length(first, last);
        if (get_capacity() < count)
        {
            size_ty new_capacity = checked_calculate_new_capacity(count);
            ptr     new_begin    = unchecked_allocate(new_capacity);

            try
            {
                uninitialized_copy(first, last, new_begin);
            }
            catch (...)
            {
                deallocate(new_begin, new_capacity);
                throw;
            }

            reset_data(new_begin, new_capacity, count);
        }
        else if (get_size() < count)
        {
            ForwardIt pivot = copy_n_return_in(first, get_size(), begin_ptr());
            uninitialized_copy(pivot, last, end_ptr());
            set_size(count);
        }
        else
        {
            erase_range(copy_range(first, last, begin_ptr()), end_ptr());
        }
    }

    template <typename InputIt,
              std::enable_if_t<
                  !std::is_assignable_v<value_ty&, decltype(*std::declval<InputIt>())>>* = nullptr>
    constexpr void assign_with_range(InputIt first, InputIt last, std::input_iterator_tag)
    {
        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;

        // If not assignable then destroy all elements and append.
        erase_all();
        append_range(first, last, iterator_cat{});
    }

    // Ie. move-if-noexcept.
    struct strong_exception_policy
    {
    };

    template <typename Policy        = void,
              typename V             = value_ty,
              std::enable_if_t<is_explicitly_move_insertable<V>::value &&
                                   (!std::is_same_v<Policy, strong_exception_policy> ||
                                    relocate_with_move<V>::value),
                               bool> = true>
    constexpr auto uninitialized_move(ptr first, ptr last, ptr d_first) noexcept(
        std::is_nothrow_move_constructible_v<value_ty>) -> ptr
    {
        return uninitialized_copy(std::make_move_iterator(first),
                                  std::make_move_iterator(last),
                                  d_first);
    }

    template <typename Policy        = void,
              typename V             = value_ty,
              std::enable_if_t<!is_explicitly_move_insertable<V>::value ||
                                   (std::is_same_v<Policy, strong_exception_policy> &&
                                    !relocate_with_move<V>::value),
                               bool> = false>
    constexpr auto uninitialized_move(ptr first, ptr last, ptr d_first) noexcept(
        alloc_interface::template is_uninitialized_memcpyable_iterator<ptr>::value) -> ptr
    {
        return uninitialized_copy(first, last, d_first);
    }

    constexpr auto shift_into_uninitialized(ptr pos, size_ty n_shift) -> ptr
    {
        // Shift elements over to the right into uninitialized space.
        // Returns the start of the shifted range.
        // Precondition: shift < end_ptr () - pos
        assert(n_shift != 0 && "The value of `n_shift` should not be 0.");

        const ptr original_end = end_ptr();
        const ptr pivot        = unchecked_prev(original_end, n_shift);

        uninitialized_move(pivot, original_end, original_end);
        increase_size(n_shift);
        return move_right(pos, pivot, original_end);
    }

    template <typename... Args>
    constexpr auto append_element(Args&&... args) -> ptr
    {
        if (get_size() < get_capacity())
        {
            return emplace_into_current_end(std::forward<Args>(args)...);
        }
        return emplace_into_reallocation_end(std::forward<Args>(args)...);
    }

    constexpr auto append_copies(size_ty count, const value_ty& val) -> ptr
    {
        if (num_uninitialized() < count)
        {
            // Reallocate.
            if (get_max_size() - get_size() < count)
            {
                throw_allocation_size_error();
            }

            size_ty original_size = get_size();
            size_ty new_size      = get_size() + count;

            // The check is handled by the if-guard.
            size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
            ptr     new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
            ptr     new_last     = unchecked_next(new_data_ptr, original_size);

            try
            {
                new_last = uninitialized_fill(new_last, unchecked_next(new_last, count), val);
                uninitialized_move(begin_ptr(), end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                destroy_range(unchecked_next(new_data_ptr, original_size), new_last);
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, new_size);
            return unchecked_next(new_data_ptr, original_size);
        }

        const ptr ret = end_ptr();
        uninitialized_fill(ret, unchecked_next(ret, count), val);
        increase_size(count);
        return ret;
    }

    template <typename MovePolicy,
              typename InputIt,
              std::enable_if_t<std::is_same_v<MovePolicy, strong_exception_policy>, bool> = true>
    constexpr auto append_range(InputIt first, InputIt last, std::input_iterator_tag) -> ptr
    {
        // Append with a strong exception guarantee.
        size_ty original_size = get_size();
        for (; !(first == last); ++first)
        {
            try
            {
                append_element(*first);
            }
            catch (...)
            {
                erase_range(unchecked_next(begin_ptr(), original_size), end_ptr());
                throw;
            }
        }
        return unchecked_next(begin_ptr(), original_size);
    }

    template <typename MovePolicy = void,
              typename InputIt,
              std::enable_if_t<!std::is_same_v<MovePolicy, strong_exception_policy>, bool> = false>
    constexpr auto append_range(InputIt first, InputIt last, std::input_iterator_tag) -> ptr
    {
        size_ty original_size = get_size();
        for (; !(first == last); ++first)
        {
            append_element(*first);
        }
        return unchecked_next(begin_ptr(), original_size);
    }

    template <typename MovePolicy = void, typename ForwardIt>
    constexpr auto append_range(ForwardIt first, ForwardIt last, std::forward_iterator_tag) -> ptr
    {
        const size_ty num_insert = external_range_length(first, last);

        if (num_uninitialized() < num_insert)
        {
            // Reallocate.
            if (get_max_size() - get_size() < num_insert)
            {
                throw_allocation_size_error();
            }

            size_ty original_size = get_size();
            size_ty new_size      = get_size() + num_insert;

            // The check is handled by the if-guard.
            size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
            ptr     new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
            ptr     new_last     = unchecked_next(new_data_ptr, original_size);

            try
            {
                new_last = uninitialized_copy(first, last, new_last);
                uninitialized_move<MovePolicy>(begin_ptr(), end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                destroy_range(unchecked_next(new_data_ptr, original_size), new_last);
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, new_size);
            return unchecked_next(new_data_ptr, original_size);
        }

        ptr ret = end_ptr();
        uninitialized_copy(first, last, ret);
        increase_size(num_insert);
        return ret;
    }

    template <typename... Args>
    constexpr auto emplace_at(ptr pos, Args&&... args) -> ptr
    {
        assert(get_size() <= get_capacity() && "size was greater than capacity");

        if (get_size() < get_capacity())
        {
            return emplace_into_current(pos, std::forward<Args>(args)...);
        }
        return emplace_into_reallocation(pos, std::forward<Args>(args)...);
    }

    constexpr auto insert_copies(ptr pos, size_ty count, const value_ty& val) -> ptr
    {
        if (0 == count)
        {
            return pos;
        }

        if (end_ptr() == pos)
        {
            if (1 == count)
            {
                return append_element(val);
            }
            return append_copies(count, val);
        }

        if (num_uninitialized() < count)
        {
            // Reallocate.
            if (get_max_size() - get_size() < count)
            {
                throw_allocation_size_error();
            }

            const size_ty offset = internal_range_length(begin_ptr(), pos);

            const size_ty new_size = get_size() + count;

            // The check is handled by the if-guard.
            const size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
            ptr           new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
            ptr           new_first    = unchecked_next(new_data_ptr, offset);
            ptr           new_last     = new_first;

            try
            {
                uninitialized_fill(new_first, unchecked_next(new_first, count), val);
                unchecked_advance(new_last, count);

                uninitialized_move(begin_ptr(), pos, new_data_ptr);
                new_first = new_data_ptr;
                uninitialized_move(pos, end_ptr(), new_last);
            }
            catch (...)
            {
                destroy_range(new_first, new_last);
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, new_size);
            return unchecked_next(begin_ptr(), offset);
        }

        // If we have fewer to insert than tailing elements after `pos`, we shift into
        // uninitialized and then copy over.

        const size_ty tail_size = internal_range_length(pos, end_ptr());
        if (tail_size < count)
        {
            // The number inserted is larger than the number after `pos`,
            // so part of the input will be used to construct new elements,
            // and another part of it will assign existing ones.
            // In order:
            //   Construct new elements immediately after end_ptr () using the input.
            //   Move-construct existing elements over to the tail.
            //   Assign existing elements using the input.

            ptr original_end = end_ptr();

            // Place a portion of the input into the uninitialized section.
            size_ty num_val_tail = count - tail_size;

            if (std::is_constant_evaluated())
            {
                uninitialized_fill(end_ptr(), unchecked_next(end_ptr(), num_val_tail), val);
                increase_size(num_val_tail);

                const heap_temporary tmp(*this, val);

                uninitialized_move(pos, original_end, end_ptr());
                increase_size(tail_size);

                std::fill_n(pos, tail_size, tmp.get());

                return pos;
            }

            uninitialized_fill(end_ptr(), unchecked_next(end_ptr(), num_val_tail), val);
            increase_size(num_val_tail);

            try
            {
                // We need to handle possible aliasing here.
                const stack_temporary tmp(*this, val);

                // Now, move the tail to the end.
                uninitialized_move(pos, original_end, end_ptr());
                increase_size(tail_size);

                try
                {
                    // Finally, try to copy the rest of the elements over.
                    std::fill_n(pos, tail_size, tmp.get());
                }
                catch (...)
                {
                    // Attempt to roll back and destroy the tail if we fail.
                    ptr inserted_end = unchecked_prev(end_ptr(), tail_size);
                    move_left(inserted_end, end_ptr(), pos);
                    destroy_range(inserted_end, end_ptr());
                    decrease_size(tail_size);
                    throw;
                }
            }
            catch (...)
            {
                // Destroy the elements constructed from the input.
                destroy_range(original_end, end_ptr());
                decrease_size(internal_range_length(original_end, end_ptr()));
                throw;
            }
        }
        else
        {
            if (std::is_constant_evaluated())
            {
                const heap_temporary tmp(*this, val);

                ptr inserted_end = shift_into_uninitialized(pos, count);
                std::fill(pos, inserted_end, tmp.get());

                return pos;
            }

            const stack_temporary tmp(*this, val);

            ptr inserted_end = shift_into_uninitialized(pos, count);

            // Attempt to copy over the elements.
            // If we fail we'll attempt a full roll-back.
            try
            {
                std::fill(pos, inserted_end, tmp.get());
            }
            catch (...)
            {
                ptr original_end = move_left(inserted_end, end_ptr(), pos);
                destroy_range(original_end, end_ptr());
                decrease_size(count);
                throw;
            }
        }
        return pos;
    }

    template <typename ForwardIt>
    constexpr auto insert_range_helper(ptr pos, ForwardIt first, ForwardIt last) -> ptr
    {
        assert(!(first == last) && "The range should not be empty.");
        assert(!(end_ptr() == pos) && "`pos` should not be at the end.");

        const size_ty num_insert = external_range_length(first, last);
        if (num_uninitialized() < num_insert)
        {
            // Reallocate.
            if (get_max_size() - get_size() < num_insert)
            {
                throw_allocation_size_error();
            }

            const size_ty offset   = internal_range_length(begin_ptr(), pos);
            const size_ty new_size = get_size() + num_insert;

            // The check is handled by the if-guard.
            const size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
            const ptr     new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
            ptr           new_first    = unchecked_next(new_data_ptr, offset);
            ptr           new_last     = new_first;

            try
            {
                uninitialized_copy(first, last, new_first);
                unchecked_advance(new_last, num_insert);

                uninitialized_move(begin_ptr(), pos, new_data_ptr);
                new_first = new_data_ptr;
                uninitialized_move(pos, end_ptr(), new_last);
            }
            catch (...)
            {
                destroy_range(new_first, new_last);
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, new_size);
            return unchecked_next(begin_ptr(), offset);
        }

        // if we have fewer to insert than tailing elements after
        // `pos` we shift into uninitialized and then copy over
        const size_ty tail_size = internal_range_length(pos, end_ptr());
        if (tail_size < num_insert)
        {
            // Use the same method as insert_copies.
            ptr       original_end = end_ptr();
            ForwardIt pivot        = unchecked_next(first, tail_size);

            // Place a portion of the input into the uninitialized section.
            uninitialized_copy(pivot, last, end_ptr());
            increase_size(num_insert - tail_size);

            try
            {
                // Now move the tail to the end.
                uninitialized_move(pos, original_end, end_ptr());
                increase_size(tail_size);

                try
                {
                    // Finally, try to copy the rest of the elements over.
                    copy_range(first, pivot, pos);
                }
                catch (...)
                {
                    // Attempt to roll back and destroy the tail if we fail.
                    ptr inserted_end = unchecked_prev(end_ptr(), tail_size);
                    move_left(inserted_end, end_ptr(), pos);
                    destroy_range(inserted_end, end_ptr());
                    decrease_size(tail_size);
                    throw;
                }
            }
            catch (...)
            {
                // If we throw, destroy the first copy we made.
                destroy_range(original_end, end_ptr());
                decrease_size(internal_range_length(original_end, end_ptr()));
                throw;
            }
        }
        else
        {
            shift_into_uninitialized(pos, num_insert);

            // Attempt to copy over the elements.
            // If we fail we'll attempt a full roll-back.
            try
            {
                copy_range(first, last, pos);
            }
            catch (...)
            {
                ptr inserted_end = unchecked_next(pos, num_insert);
                ptr original_end = move_left(inserted_end, end_ptr(), pos);
                destroy_range(original_end, end_ptr());
                decrease_size(num_insert);
                throw;
            }
        }
        return pos;
    }

    template <typename InputIt>
    constexpr auto insert_range(ptr pos, InputIt first, InputIt last, std::input_iterator_tag)
        -> ptr
    {
        assert(!(first == last) && "The range should not be empty.");

        // Ensure we use this specific overload to give a strong exception guarantee for 1 element.
        if (end_ptr() == pos)
        {
            return append_range(first, last, std::input_iterator_tag{});
        }

        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;
        ListBase tmp(first, last, iterator_cat{}, allocator_ref());

        return insert_range_helper(pos,
                                   std::make_move_iterator(tmp.begin_ptr()),
                                   std::make_move_iterator(tmp.end_ptr()));
    }

    template <typename ForwardIt>
    constexpr auto insert_range(ptr pos, ForwardIt first, ForwardIt last, std::forward_iterator_tag)
        -> ptr
    {
        if (!(end_ptr() == pos))
        {
            return insert_range_helper(pos, first, last);
        }

        if (unchecked_next(first) == last)
        {
            return append_element(*first);
        }

        using iterator_cat = typename std::iterator_traits<ForwardIt>::iterator_category;
        return append_range(first, last, iterator_cat{});
    }

    template <typename... Args>
    constexpr auto emplace_into_current_end(Args&&... args) -> ptr
    {
        construct(end_ptr(), std::forward<Args>(args)...);
        increase_size(1);
        return unchecked_prev(end_ptr());
    }

    template <typename V                                                 = value_ty,
              std::enable_if_t<std::is_nothrow_move_constructible_v<V>>* = nullptr>
    constexpr auto emplace_into_current(ptr pos, value_ty&& val) -> ptr
    {
        if (pos == end_ptr())
        {
            return emplace_into_current_end(std::move(val));
        }

        // In the special case of value_ty&& we don't make a copy because behavior is unspecified
        // when it is an internal element. Hence, we'll take the opportunity to optimize and assume
        // that it isn't an internal element.
        shift_into_uninitialized(pos, 1);
        destroy(pos);
        construct(pos, std::move(val));
        return pos;
    }

    template <typename... Args>
    constexpr auto emplace_into_current(ptr pos, Args&&... args) -> ptr
    {
        if (pos == end_ptr())
        {
            return emplace_into_current_end(std::forward<Args>(args)...);
        }

        if (std::is_constant_evaluated())
        {
            heap_temporary tmp(*this, std::forward<Args>(args)...);
            shift_into_uninitialized(pos, 1);
            *pos = tmp.release();
            return pos;
        }

        // This is necessary because of possible aliasing.
        stack_temporary tmp(*this, std::forward<Args>(args)...);
        shift_into_uninitialized(pos, 1);
        *pos = tmp.release();
        return pos;
    }

    template <typename... Args>
    constexpr auto emplace_into_reallocation_end(Args&&... args) -> ptr
    {
        // Appending; strong exception guarantee.
        if (get_max_size() == get_size())
        {
            throw_allocation_size_error();
        }

        const size_ty new_size = get_size() + 1;

        // The check is handled by the if-guard.
        const size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
        const ptr     new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
        const ptr     emplace_pos  = unchecked_next(new_data_ptr, get_size());

        try
        {
            construct(emplace_pos, std::forward<Args>(args)...);
            try
            {
                uninitialized_move<strong_exception_policy>(begin_ptr(), end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                destroy(emplace_pos);
                throw;
            }
        }
        catch (...)
        {
            deallocate(new_data_ptr, new_capacity);
            throw;
        }

        reset_data(new_data_ptr, new_capacity, new_size);
        return emplace_pos;
    }

    template <typename... Args>
    constexpr auto emplace_into_reallocation(ptr pos, Args&&... args) -> ptr
    {
        const size_ty offset = internal_range_length(begin_ptr(), pos);
        if (offset == get_size())
        {
            return emplace_into_reallocation_end(std::forward<Args>(args)...);
        }

        if (get_max_size() == get_size())
        {
            throw_allocation_size_error();
        }

        const size_ty new_size = get_size() + 1;

        // The check is handled by the if-guard.
        const size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
        const ptr     new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
        ptr           new_first    = unchecked_next(new_data_ptr, offset);
        ptr           new_last     = new_first;

        try
        {
            construct(new_first, std::forward<Args>(args)...);
            unchecked_advance(new_last, 1);

            uninitialized_move(begin_ptr(), pos, new_data_ptr);
            new_first = new_data_ptr;
            uninitialized_move(pos, end_ptr(), new_last);
        }
        catch (...)
        {
            destroy_range(new_first, new_last);
            deallocate(new_data_ptr, new_capacity);
            throw;
        }

        reset_data(new_data_ptr, new_capacity, new_size);
        return unchecked_next(begin_ptr(), offset);
    }

    constexpr ptr shrink_to_size()
    {
        if (!has_allocation() || get_size() == get_capacity())
            return begin_ptr();

        // The rest runs only if allocated.

        size_ty new_capacity;
        ptr     new_data_ptr;

        if (InlineCapacity < get_size())
        {
            new_capacity = get_size();
            new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
        }
        else
        {
            // We move to inline storage.
            new_capacity = InlineCapacity;
            if (std::is_constant_evaluated())
            {
                new_data_ptr = alloc_interface::allocate(InlineCapacity);
            }
            else
            {
                new_data_ptr = storage_ptr();
            }
        }

        uninitialized_move(begin_ptr(), end_ptr(), new_data_ptr);

        destroy_range(begin_ptr(), end_ptr());
        deallocate(data_ptr(), get_capacity());

        set_data_ptr(new_data_ptr);
        set_capacity(new_capacity);

        return begin_ptr();
    }

    template <typename... ValueT>
    constexpr void resize_with(size_ty new_size, const ValueT&... val)
    {
        // ValueT... should either be value_ty or empty.

        if (new_size == 0)
        {
            erase_all();
        }

        if (get_capacity() < new_size)
        {
            // Reallocate.

            if (get_max_size() < new_size)
            {
                throw_allocation_size_error();
            }

            const size_ty original_size = get_size();

            // The check is handled by the if-guard.
            const size_ty new_capacity = unchecked_calculate_new_capacity(new_size);
            ptr           new_data_ptr = unchecked_allocate(new_capacity, allocation_end_ptr());
            ptr           new_last     = unchecked_next(new_data_ptr, original_size);

            try
            {
                new_last =
                    uninitialized_fill(new_last, unchecked_next(new_data_ptr, new_size), val...);

                // Strong exception guarantee.
                uninitialized_move<strong_exception_policy>(begin_ptr(), end_ptr(), new_data_ptr);
            }
            catch (...)
            {
                destroy_range(unchecked_next(new_data_ptr, original_size), new_last);
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            reset_data(new_data_ptr, new_capacity, new_size);
        }
        else if (get_size() < new_size)
        {
            // Construct in the uninitialized section.
            uninitialized_fill(end_ptr(), unchecked_next(begin_ptr(), new_size), val...);
            set_size(new_size);
        }
        else
        {
            erase_range(unchecked_next(begin_ptr(), new_size), end_ptr());
        }

        // Do nothing if the count is the same as the current size.
    }

    constexpr void request_capacity(size_ty request)
    {
        if (request <= get_capacity())
        {
            return;
        }

        size_ty new_capacity = checked_calculate_new_capacity(request);
        ptr     new_begin    = unchecked_allocate(new_capacity);

        try
        {
            uninitialized_move<strong_exception_policy>(begin_ptr(), end_ptr(), new_begin);
        }
        catch (...)
        {
            deallocate(new_begin, new_capacity);
            throw;
        }

        wipe();

        set_data_ptr(new_begin);
        set_capacity(new_capacity);
    }

    constexpr auto erase_at(ptr pos) -> ptr
    {
        move_left(unchecked_next(pos), end_ptr(), pos);
        erase_last();
        return pos;
    }

    constexpr void erase_last()
    {
        decrease_size(1);

        // The element located at end_ptr is still alive since the size decreased.
        destroy(end_ptr());
    }

    constexpr auto erase_range(ptr first, ptr last) -> ptr
    {
        if (!(first == last))
        {
            erase_to_end(move_left(last, end_ptr(), first));
        }
        return first;
    }

    constexpr void erase_to_end(ptr pos)
    {
        assert(0 <= (end_ptr() - pos) && "`pos` was in the uninitialized range");
        if (size_ty change = internal_range_length(pos, end_ptr()))
        {
            decrease_size(change);
            destroy_range(pos, unchecked_next(pos, change));
        }
    }

    constexpr void erase_all()
    {
        ptr curr_end = end_ptr();
        set_size(0);
        destroy_range(begin_ptr(), curr_end);
    }

    constexpr void swap_elements(ListBase& other) noexcept(
        std::is_nothrow_move_constructible<value_ty>::value &&
        std::is_nothrow_swappable<value_ty>::value)
    {
        assert(get_size() <= other.get_size());

        const ptr other_tail = std::swap_ranges(begin_ptr(), end_ptr(), other.begin_ptr());
        uninitialized_move(other_tail, other.end_ptr(), end_ptr());
        destroy_range(other_tail, other.end_ptr());

        swap_size(other);
    }

    constexpr void swap_default(ListBase& other) noexcept(
        std::is_nothrow_move_constructible_v<value_ty> && std::is_nothrow_swappable_v<value_ty>)
    {
        // This function is used when:
        //   We are using the standard allocator.
        //   The allocators propagate and are equal.
        //   The allocators are always equal.
        //   The allocators do not propagate and are equal.
        //   The allocators propagate and are not equal.

        // Not handled:
        //   The allocators do not propagate and are not equal.

        assert(get_capacity() <= other.get_capacity());

        if (has_allocation())
        { // Implies that `other` also has an allocation.
            swap_allocation(other);
        }
        else if (other.has_allocation())
        {
            // Note: This will never be constant evaluated because both are always allocated.
            uninitialized_move(begin_ptr(), end_ptr(), other.storage_ptr());
            destroy_range(begin_ptr(), end_ptr());

            set_data_ptr(other.data_ptr());
            set_capacity(other.get_capacity());

            other.set_data_ptr(other.storage_ptr());
            other.set_capacity(InlineCapacity);

            swap_size(other);
        }
        else if (get_size() < other.get_size())
        {
            swap_elements(other);
        }
        else
        {
            other.swap_elements(*this);
        }

        alloc_interface::swap(other);
    }

    constexpr void swap_unequal_no_propagate(ListBase& other)
    {
        assert(get_capacity() <= other.get_capacity());

        if (get_capacity() < other.get_size())
        {
            // Reallocation required.
            // We should always be able to reuse the allocation of `other`.
            const size_ty new_capacity = unchecked_calculate_new_capacity(other.get_size());
            const ptr     new_data_ptr = unchecked_allocate(new_capacity, end_ptr());

            try
            {
                uninitialized_move(other.begin_ptr(), other.end_ptr(), new_data_ptr);
                try
                {
                    destroy_range(std::move(begin_ptr(), end_ptr(), other.begin_ptr()),
                                  other.end_ptr());
                }
                catch (...)
                {
                    destroy_range(new_data_ptr, unchecked_next(new_data_ptr, other.get_size()));
                    throw;
                }
            }
            catch (...)
            {
                deallocate(new_data_ptr, new_capacity);
                throw;
            }

            destroy_range(begin_ptr(), end_ptr());
            if (has_allocation())
            {
                deallocate(data_ptr(), get_capacity());
            }

            set_data_ptr(new_data_ptr);
            set_capacity(new_capacity);
            swap_size(other);
        }
        else if (get_size() < other.get_size())
        {
            swap_elements(other);
        }
        else
        {
            other.swap_elements(*this);
        }

        // This should have no effect.
        alloc_interface::swap(other);
    }

    template <
        typename A                                                                    = alloc_ty,
        std::enable_if_t<allocations_are_swappable<A>::value && InlineCapacity == 0>* = nullptr>
    constexpr void swap(ListBase& other) noexcept
    {
        swap_allocation(other);
        alloc_interface::swap(other);
    }

    template <
        typename A                                                                    = alloc_ty,
        std::enable_if_t<allocations_are_swappable<A>::value && InlineCapacity != 0>* = nullptr>
    constexpr void swap(ListBase& other) noexcept(std::is_nothrow_move_constructible_v<value_ty> &&
                                                  std::is_nothrow_swappable_v<value_ty>)
    {
        if (get_capacity() < other.get_capacity())
        {
            swap_default(other);
        }
        else
        {
            other.swap_default(*this);
        }
    }

    template <typename A                                              = alloc_ty,
              std::enable_if_t<!allocations_are_swappable<A>::value>* = nullptr>
    constexpr void swap(ListBase& other) noexcept
    {
        if (get_capacity() < other.get_capacity())
        {
            if (other.allocator_ref() == allocator_ref())
            {
                swap_default(other);
            }
            else
            {
                swap_unequal_no_propagate(other);
            }
        }
        else
        {
            if (other.allocator_ref() == allocator_ref())
            {
                other.swap_default(*this);
            }
            else
            {
                other.swap_unequal_no_propagate(*this);
            }
        }
    }

#ifdef __GLIBCXX__

    // These are compatibility fixes for libstdc++ because std::copy doesn't work for
    // `move_iterator`s when constant evaluated.

    template <typename InputIt>
    static constexpr InputIt unmove_iterator(InputIt it)
    {
        return it;
    }

    template <typename InputIt>
    static constexpr auto unmove_iterator(std::move_iterator<InputIt> it)
        -> decltype(unmove_iterator(it.base()))
    {
        return unmove_iterator(it.base());
    }

    template <typename InputIt>
    static constexpr auto unmove_iterator(std::reverse_iterator<InputIt> it)
        -> std::reverse_iterator<decltype(unmove_iterator(it.base()))>
    {
        return std::reverse_iterator<decltype(unmove_iterator(it.base()))>(
            unmove_iterator(it.base()));
    }

#endif

    template <typename InputIt>
    constexpr auto copy_range(InputIt first, InputIt last, ptr dest) -> ptr
    {
#if defined(__GLIBCXX__)
        if (std::is_constant_evaluated() &&
            !std::is_same<decltype(unmove_iterator(std::declval<InputIt>())), InputIt>::value)
        {
            return std::move(unmove_iterator(first), unmove_iterator(last), dest);
        }
#endif


        return std::copy(first, last, dest);
    }

    template <typename InputIt, std::enable_if_t<is_memcpyable_iterator<InputIt>::value>* = nullptr>
    constexpr auto copy_n_return_in(InputIt first, size_ty count, ptr dest) noexcept -> InputIt
    {
        if (std::is_constant_evaluated())
        {
            std::copy_n(first, count, dest);
            return unchecked_next(first, count);
        }

        if (count != 0)
        {
            std::memcpy(to_address(dest), to_address(first), count * sizeof(value_ty));
        }

        // Note: The unsafe cast here should be proven to be safe in the caller function.
        return unchecked_next(first, count);
    }

    template <typename InputIt, std::enable_if_t<is_memcpyable_iterator<InputIt>::value>* = nullptr>
    constexpr auto copy_n_return_in(std::move_iterator<InputIt> first,
                                    size_ty                     count,
                                    ptr dest) noexcept -> std::move_iterator<InputIt>
    {
        return std::move_iterator<InputIt>(copy_n_return_in(first.base(), count, dest));
    }

    template <typename RandomIt,
              std::enable_if_t<
                  !is_memcpyable_iterator<RandomIt>::value &&
                  std::is_base_of_v<std::random_access_iterator_tag,
                                    typename std::iterator_traits<RandomIt>::iterator_category>>* =
                  nullptr>
    constexpr auto copy_n_return_in(RandomIt first, size_ty count, ptr dest) -> RandomIt
    {
#if defined(__GLIBCXX__)
        if (std::is_constant_evaluated() &&
            !std::is_same<decltype(unmove_iterator(std::declval<RandomIt>())), RandomIt>::value)
        {
            auto bfirst = unmove_iterator(first);
            auto blast  = unchecked_next(bfirst, count);
            std::move(bfirst, blast, dest);
            return unchecked_next(first, count);
        }
#endif

        std::copy_n(first, count, dest);
        // Note: This unsafe cast should be proven safe in the caller function.
        return unchecked_next(first, count);
    }

    template <typename InputIt,
              std::enable_if_t<
                  !is_memcpyable_iterator<InputIt>::value &&
                  !std::is_base_of_v<std::random_access_iterator_tag,
                                     typename std::iterator_traits<InputIt>::iterator_category>>* =
                  nullptr>
    constexpr auto copy_n_return_in(InputIt first, size_ty count, ptr dest) -> InputIt
    {

        for (; count != 0; --count, static_cast<void>(++dest), static_cast<void>(++first))
            *dest = *first;
        return first;
    }

    template <typename V = value_ty, std::enable_if_t<is_memcpyable<V>::value>* = nullptr>
    constexpr auto move_left(ptr first, ptr last, ptr d_first) -> ptr
    {
        // Shift initialized elements to the left.

        if (std::is_constant_evaluated())
        {
            return std::move(first, last, d_first);
        }

        const size_ty num_moved = internal_range_length(first, last);
        if (num_moved != 0)
        {
            std::memmove(to_address(d_first), to_address(first), num_moved * sizeof(value_ty));
        }
        return unchecked_next(d_first, num_moved);
    }

    template <typename V = value_ty, std::enable_if_t<!is_memcpyable<V>::value>* = nullptr>
    constexpr auto move_left(ptr first, ptr last, ptr d_first) -> ptr
    {
        // Shift initialized elements to the left.
        return std::move(first, last, d_first);
    }

    template <typename V = value_ty, std::enable_if_t<is_memcpyable<V>::value, bool> = true>
    constexpr auto move_right(ptr first, ptr last, ptr d_last) -> ptr
    {
        // Move initialized elements to the right.

        if (std::is_constant_evaluated())
        {
            return std::move_backward(first, last, d_last);
        }

        const size_ty num_moved = internal_range_length(first, last);
        const ptr     dest      = unchecked_prev(d_last, num_moved);
        if (num_moved != 0)
        {
            std::memmove(to_address(dest), to_address(first), num_moved * sizeof(value_ty));
        }
        return dest;
    }

    template <typename V = value_ty, std::enable_if_t<!is_memcpyable<V>::value, bool> = false>
    constexpr auto move_right(ptr first, ptr last, ptr d_last) -> ptr
    {
        // move initialized elements to the right
        // n should not be 0
        return std::move_backward(first, last, d_last);
    }

  public:
    constexpr void set_default()
    {
        set_to_inline_storage();
        set_size(0);
    }

    [[nodiscard]] constexpr auto data_ptr() noexcept -> ptr
    {
        return m_data.data_ptr();
    }

    [[nodiscard]] constexpr auto data_ptr() const noexcept -> cptr
    {
        return m_data.data_ptr();
    }

    [[nodiscard]] constexpr auto get_capacity() const noexcept -> size_ty
    {
        return m_data.capacity();
    }

    [[nodiscard]] constexpr auto get_size() const noexcept -> size_ty
    {
        return m_data.size();
    }

    [[nodiscard]] constexpr auto num_uninitialized() const noexcept -> size_ty
    {
        return get_capacity() - get_size();
    }

    [[nodiscard]] constexpr auto begin_ptr() noexcept -> ptr
    {
        return data_ptr();
    }

    [[nodiscard]] constexpr auto begin_ptr() const noexcept -> cptr
    {
        return data_ptr();
    }

    [[nodiscard]] constexpr auto end_ptr() noexcept -> ptr
    {
        return unchecked_next(begin_ptr(), get_size());
    }

    [[nodiscard]] constexpr auto end_ptr() const noexcept -> cptr
    {
        return unchecked_next(begin_ptr(), get_size());
    }

    [[nodiscard]] constexpr auto allocation_end_ptr() noexcept -> ptr
    {
        return unchecked_next(begin_ptr(), get_capacity());
    }

    [[nodiscard]] constexpr auto allocation_end_ptr() const noexcept -> cptr
    {
        return unchecked_next(begin_ptr(), get_capacity());
    }

    [[nodiscard]] constexpr auto copy_allocator() const noexcept -> alloc_ty
    {
        return alloc_ty(allocator_ref());
    }

    [[nodiscard]] constexpr auto storage_ptr() noexcept -> ptr
    {
        return m_data.storage();
    }

    [[nodiscard]] constexpr auto has_allocation() const noexcept -> bool
    {
        if (std::is_constant_evaluated())
        {
            return true;
        }

        return InlineCapacity < get_capacity();
    }

    [[nodiscard]] constexpr auto is_inlinable() const noexcept -> bool
    {
        return get_size() <= InlineCapacity;
    }

  private:
    ListData<ptr, size_type, value_ty, InlineCapacity> m_data;
};

} // namespace detail

template <typename T, unsigned InlineCapacity, typename Allocator>
    requires concepts::List::AllocatorFor<Allocator, T>
class List : detail::ListBase<Allocator, InlineCapacity>
{
    using base = detail::ListBase<Allocator, InlineCapacity>;

  public:
    static_assert(std::is_same_v<T, typename Allocator::value_type>,
                  "`Allocator::value_type` must be the same as `T`.");

    template <typename SameT, unsigned DifferentInlineCapacity, typename SameAllocator>
        requires concepts::List::AllocatorFor<SameAllocator, SameT>
    friend class List;

    using value_type      = T;
    using allocator_type  = Allocator;
    using size_type       = typename base::size_type;
    using difference_type = typename base::difference_type;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer   = typename std::allocator_traits<allocator_type>::const_pointer;

    using iterator               = ListIterator<pointer, difference_type>;
    using const_iterator         = ListIterator<const_pointer, difference_type>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_assert(InlineCapacity <= (std::numeric_limits<size_type>::max)(),
                  "InlineCapacity must be less than or equal to the maximum value of size_type.");

    static constexpr unsigned inline_capacity_v = InlineCapacity;

  private:
    static constexpr bool Destructible = concepts::List::Destructible<value_type>;

    static constexpr bool MoveAssignable = concepts::List::MoveAssignable<value_type>;

    static constexpr bool CopyAssignable = concepts::List::CopyAssignable<value_type>;

    static constexpr bool MoveConstructible = concepts::List::MoveConstructible<value_type>;

    static constexpr bool CopyConstructible = concepts::List::CopyConstructible<value_type>;

    static constexpr bool Swappable = concepts::List::Swappable<value_type>;

    static constexpr bool DefaultInsertable =
        concepts::List::DefaultInsertable<value_type, List, allocator_type>;

    static constexpr bool MoveInsertable =
        concepts::List::MoveInsertable<value_type, List, allocator_type>;

    static constexpr bool CopyInsertable =
        concepts::List::CopyInsertable<value_type, List, allocator_type>;

    static constexpr bool Erasable = concepts::List::Erasable<value_type, List, allocator_type>;

    template <typename... Args>
    struct EmplaceConstructible
    {
        static constexpr bool value =
            concepts::List::EmplaceConstructible<value_type, List, allocator_type, Args...>;
    };

  public:
    constexpr List() noexcept(noexcept(allocator_type()))
        requires concepts::DefaultConstructible<allocator_type>
    = default;

    constexpr List(const List& other)
        requires CopyInsertable
        : base(base::bypass, other)
    {
    }

    constexpr List(List&& other) noexcept(std::is_nothrow_move_constructible_v<value_type> ||
                                          InlineCapacity == 0)
        requires MoveInsertable
        : base(base::bypass, std::move(other))
    {
    }

    constexpr explicit List(const allocator_type& alloc) noexcept
        : base(alloc)
    {
    }

    constexpr List(const List& other, const allocator_type& alloc)
        requires CopyInsertable
        : base(base::bypass, other, alloc)
    {
    }

    constexpr List(List&& other, const allocator_type& alloc)
        requires MoveInsertable
        : base(base::bypass, std::move(other), alloc)
    {
    }

    constexpr explicit List(size_type count, const allocator_type& alloc = allocator_type())
        requires DefaultInsertable
        : base(count, alloc)
    {
    }

    constexpr List(size_type             count,
                   const_reference       value,
                   const allocator_type& alloc = allocator_type())
        requires CopyInsertable
        : base(count, value, alloc)
    {
    }

    template <typename Generator>
        requires std::invocable<Generator&> &&
                 EmplaceConstructible<std::invoke_result_t<Generator&>>::value
    constexpr List(size_type count, Generator g, const allocator_type& alloc = allocator_type())
        : base(count, g, alloc)
    {
    }

    template <std::input_iterator InputIt>
        requires EmplaceConstructible<std::iter_reference_t<InputIt>>::value &&
                 (std::forward_iterator<InputIt> || MoveInsertable)
    constexpr List(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
        : base(first, last, typename std::iterator_traits<InputIt>::iterator_category{}, alloc)
    {
    }

    constexpr List(std::initializer_list<value_type> init,
                   const allocator_type&             alloc = allocator_type())
        requires EmplaceConstructible<const_reference>::value
        : List(init.begin(), init.end(), alloc)
    {
    }

    template <unsigned I>
        requires CopyInsertable
    constexpr explicit List(const List<T, I, Allocator>& other)
        : base(base::bypass, other)
    {
    }

    template <unsigned I>
        requires MoveInsertable
    constexpr explicit List(List<T, I, Allocator>&& other) noexcept(
        std::is_nothrow_move_constructible_v<value_type> && I < InlineCapacity)
        : base(base::bypass, std::move(other))
    {
    }

    template <unsigned I>
        requires CopyInsertable
    constexpr List(const List<T, I, Allocator>& other, const allocator_type& alloc)
        : base(base::bypass, other, alloc)
    {
    }

    template <unsigned I>
        requires MoveInsertable
    constexpr List(List<T, I, Allocator>&& other, const allocator_type& alloc)
        : base(base::bypass, std::move(other), alloc)
    {
    }

    constexpr ~List()
        requires Erasable
    = default;

    constexpr auto operator=(const List& other) -> List&
        requires CopyInsertable && CopyAssignable
    {
        assign(other);
        return *this;
    }

    constexpr auto operator=(List&& other) noexcept(
        (std::is_same_v<std::allocator<value_type>, Allocator> ||
         std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
         std::allocator_traits<Allocator>::is_always_equal::value) &&
        ((std::is_nothrow_move_assignable_v<value_type> &&
          std::is_nothrow_move_constructible_v<value_type>) ||
         InlineCapacity == 0)) -> List&
        // Note: The standard says here that
        // std::allocator_traits<allocator_type>::propagate_on_container_move_assignment == false
        // implies MoveInsertable && MoveAssignable, but since we have inline storage we must always
        // require moves [tab:container.alloc.req].
        requires MoveInsertable && MoveAssignable
    {
        assign(std::move(other));
        return *this;
    }

    constexpr auto operator=(std::initializer_list<value_type> ilist) -> List&
        requires CopyInsertable && CopyAssignable
    {
        assign(ilist);
        return *this;
    }

    constexpr void assign(size_type count, const_reference value)
        requires CopyInsertable && CopyAssignable
    {
        base::assign_with_copies(count, value);
    }

    template <std::input_iterator InputIt>
        requires EmplaceConstructible<std::iter_reference_t<InputIt>>::value &&
                 (std::forward_iterator<InputIt> || MoveInsertable)
    constexpr void assign(InputIt first, InputIt last)
    {
        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;
        base::assign_with_range(first, last, iterator_cat{});
    }

    constexpr void assign(std::initializer_list<value_type> ilist)
        requires EmplaceConstructible<const_reference>::value
    {
        assign(ilist.begin(), ilist.end());
    }

    constexpr void assign(const List& other)
        requires CopyInsertable && CopyAssignable
    {
        if (&other != this)
        {
            base::copy_assign(other);
        }
    }

    template <unsigned I>
        requires CopyInsertable && CopyAssignable
    constexpr void assign(const List<T, I, Allocator>& other)
    {
        base::copy_assign(other);
    }

    constexpr void assign(List&& other) noexcept(
        (std::is_same_v<std::allocator<value_type>, Allocator> ||
         std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
         std::allocator_traits<Allocator>::is_always_equal::value) &&
        ((std::is_nothrow_move_assignable_v<value_type> &&
          std::is_nothrow_move_constructible_v<value_type>) ||
         InlineCapacity == 0))
        requires MoveInsertable && MoveAssignable
    {
        if (&other != this)
        {
            base::move_assign(std::move(other));
        }
    }

    template <unsigned I>
        requires MoveInsertable && MoveAssignable
    constexpr void assign(List<T, I, Allocator>&& other) noexcept(
        I <= InlineCapacity &&
        (std::is_same_v<std::allocator<value_type>, Allocator> ||
         std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
         std::allocator_traits<Allocator>::is_always_equal::value) &&
        std::is_nothrow_move_assignable_v<value_type> &&
        std::is_nothrow_move_constructible_v<value_type>)
    {
        base::move_assign(std::move(other));
    }

    constexpr void swap(List& other) noexcept(
        (std::is_same_v<std::allocator<value_type>, Allocator> ||
         std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
         std::allocator_traits<Allocator>::is_always_equal::value) &&
        ((std::is_nothrow_move_constructible_v<value_type> &&
          std::is_nothrow_move_assignable_v<value_type> &&
          std::is_nothrow_swappable_v<value_type>) ||
         InlineCapacity == 0))
        requires(MoveInsertable && MoveAssignable && Swappable) ||
                ((std::is_same_v<std::allocator<value_type>, Allocator> ||
                  std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
                  std::allocator_traits<Allocator>::is_always_equal::value) &&
                 InlineCapacity == 0)
    {
        base::swap(other);
    }

    constexpr auto begin() noexcept -> iterator
    {
        return iterator{base::begin_ptr()};
    }

    constexpr auto begin() const noexcept -> const_iterator
    {
        return const_iterator{base::begin_ptr()};
    }

    constexpr auto cbegin() const noexcept -> const_iterator
    {
        return begin();
    }

    constexpr auto end() noexcept -> iterator
    {
        return iterator{base::end_ptr()};
    }

    constexpr auto end() const noexcept -> const_iterator
    {
        return const_iterator{base::end_ptr()};
    }

    constexpr auto cend() const noexcept -> const_iterator
    {
        return end();
    }

    constexpr auto rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator{end()};
    }

    constexpr auto rbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator{end()};
    }

    constexpr auto crbegin() const noexcept -> const_reverse_iterator
    {
        return rbegin();
    }

    constexpr auto rend() noexcept -> reverse_iterator
    {
        return reverse_iterator{begin()};
    }

    constexpr auto rend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator{begin()};
    }

    constexpr auto crend() const noexcept -> const_reverse_iterator
    {
        return rend();
    }

    constexpr auto at(size_type pos) -> reference
    {
        if (size() <= pos)
        {
            base::throw_index_error();
        }
        return begin()[static_cast<difference_type>(pos)];
    }

    constexpr auto at(size_type pos) const -> const_reference
    {
        if (size() <= pos)
        {
            base::throw_index_error();
        }
        return begin()[static_cast<difference_type>(pos)];
    }

    constexpr auto operator[](size_type pos) -> reference
    {
        return begin()[static_cast<difference_type>(pos)];
    }

    constexpr auto operator[](size_type pos) const -> const_reference
    {
        return begin()[static_cast<difference_type>(pos)];
    }

    constexpr auto front() -> reference
    {
        return (*this)[0];
    }

    constexpr auto front() const -> const_reference
    {
        return (*this)[0];
    }

    constexpr auto back() -> reference
    {
        return (*this)[size() - 1];
    }

    constexpr auto back() const -> const_reference
    {
        return (*this)[size() - 1];
    }

    constexpr auto data() noexcept -> pointer
    {
        return base::begin_ptr();
    }

    constexpr auto data() const noexcept -> const_pointer
    {
        return base::begin_ptr();
    }

    constexpr auto size() const noexcept -> size_type
    {
        return static_cast<size_type>(base::get_size());
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return size() == 0;
    }

    constexpr auto max_size() const noexcept -> size_type
    {
        return static_cast<size_type>(base::get_max_size());
    }

    constexpr auto capacity() const noexcept -> size_type
    {
        return static_cast<size_type>(base::get_capacity());
    }

    constexpr auto get_allocator() const noexcept -> allocator_type
    {
        return base::copy_allocator();
    }

    constexpr auto insert(const_iterator pos, const_reference value) -> iterator
        requires CopyInsertable && CopyAssignable
    {
        return emplace(pos, value);
    }

    constexpr auto insert(const_iterator pos, value_type&& value) -> iterator
        requires MoveInsertable && MoveAssignable
    {
        return emplace(pos, std::move(value));
    }

    constexpr auto insert(const_iterator pos, size_type count, const_reference value) -> iterator
        requires CopyInsertable && CopyAssignable
    {
        return iterator(base::insert_copies(base::ptr_cast(pos), count, value));
    }

    // Note: Unlike std::vector, this does not require MoveConstructible because we
    //       don't use std::rotate (as was the reason for the change in C++17).
    //       Relevant: https://cplusplus.github.io/LWG/issue2266).
    template <std::input_iterator InputIt>
        requires EmplaceConstructible<std::iter_reference_t<InputIt>>::value && MoveInsertable &&
                 MoveAssignable
    constexpr auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator
    {
        if (first == last)
        {
            return iterator(base::ptr_cast(pos));
        }

        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;
        return iterator(base::insert_range(base::ptr_cast(pos), first, last, iterator_cat{}));
    }

    constexpr auto insert(const_iterator pos, std::initializer_list<value_type> ilist) -> iterator
        requires EmplaceConstructible<const_reference>::value && MoveInsertable && MoveAssignable
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
        requires EmplaceConstructible<Args...>::value && MoveInsertable && MoveAssignable
    constexpr auto emplace(const_iterator pos, Args&&... args) -> iterator
    {
        return iterator(base::emplace_at(base::ptr_cast(pos), std::forward<Args>(args)...));
    }

    constexpr auto erase(const_iterator pos) -> iterator
        requires MoveAssignable && Erasable
    {
        assert(0 <= (pos - begin()) && "`pos` is out of bounds (before `begin ()`).");
        assert(0 < (end() - pos) && "`pos` is out of bounds (at or after `end ()`).");

        return iterator(base::erase_at(base::ptr_cast(pos)));
    }

    constexpr auto erase(const_iterator first, const_iterator last) -> iterator
        requires MoveAssignable && Erasable
    {
        assert(0 <= (last - first) && "Invalid range.");
        assert(0 <= (first - begin()) && "`first` is out of bounds (before `begin ()`).");
        assert(0 <= (end() - last) && "`last` is out of bounds (after `end ()`).");

        return iterator(base::erase_range(base::ptr_cast(first), base::ptr_cast(last)));
    }

    constexpr void push_back(const_reference value)
        requires CopyInsertable
    {
        emplace_back(value);
    }

    constexpr void push_back(value_type&& value)
        requires MoveInsertable
    {
        emplace_back(std::move(value));
    }

    template <typename... Args>
        requires EmplaceConstructible<Args...>::value && MoveInsertable
    constexpr auto emplace_back(Args&&... args) -> reference
    {
        return *base::append_element(std::forward<Args>(args)...);
    }

    constexpr void pop_back()
        requires Erasable
    {
        assert(!empty() && "`pop_back ()` called on an empty List.");
        base::erase_last();
    }

    constexpr void reserve(size_type new_capacity)
        requires MoveInsertable
    {
        base::request_capacity(new_capacity);
    }

    constexpr void shrink_to_fit()
        requires MoveInsertable
    {
        base::shrink_to_size();
    }

    constexpr void clear() noexcept
        requires Erasable
    {
        base::erase_all();
    }

    constexpr void resize(size_type count)
        requires MoveInsertable && DefaultInsertable
    {
        base::resize_with(count);
    }

    constexpr void resize(size_type count, const_reference value)
        requires CopyInsertable
    {
        base::resize_with(count, value);
    }

    [[nodiscard]] constexpr auto inlined() const noexcept -> bool
    {
        return !base::has_allocation();
    }

    [[nodiscard]] constexpr auto inlinable() const noexcept -> bool
    {
        return base::is_inlinable();
    }

    [[nodiscard]] static consteval auto inline_capacity() noexcept -> size_type
    {
        return static_cast<size_type>(inline_capacity_v);
    }

    template <std::input_iterator InputIt>
        requires EmplaceConstructible<std::iter_reference_t<InputIt>>::value && MoveInsertable
    constexpr auto append(InputIt first, InputIt last) -> List&
    {
        using policy       = typename base::strong_exception_policy;
        using iterator_cat = typename std::iterator_traits<InputIt>::iterator_category;
        base::template append_range<policy>(first, last, iterator_cat{});
        return *this;
    }

    constexpr auto append(std::initializer_list<value_type> ilist) -> List&
        requires EmplaceConstructible<const_reference>::value && MoveInsertable
    {
        return append(ilist.begin(), ilist.end());
    }

    template <unsigned I>
    constexpr auto append(const List<T, I, Allocator>& other) -> List&
        requires CopyInsertable
    {
        return append(other.begin(), other.end());
    }

    template <unsigned I>
    constexpr auto append(List<T, I, Allocator>&& other) -> List&
        requires MoveInsertable
    {
        // Provide a strong exception guarantee for `other` as well.
        using move_iter_type =
            std::conditional_t<base::template relocate_with_move<value_type>::value,
                               std::move_iterator<iterator>,
                               iterator>;

        append(move_iter_type{other.begin()}, move_iter_type{other.end()});
        other.clear();
        return *this;
    }
};

template <typename T, unsigned InlineCapacityLHS, unsigned InlineCapacityRHS, typename Allocator>
constexpr auto operator==(const List<T, InlineCapacityLHS, Allocator>& lhs,
                          const List<T, InlineCapacityRHS, Allocator>& rhs) -> bool
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto operator==(const List<T, InlineCapacity, Allocator>& lhs,
                          const List<T, InlineCapacity, Allocator>& rhs) -> bool
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, unsigned InlineCapacityLHS, unsigned InlineCapacityRHS, typename Allocator>
constexpr auto operator!=(const List<T, InlineCapacityLHS, Allocator>& lhs,
                          const List<T, InlineCapacityRHS, Allocator>& rhs) -> bool
{
    return !(lhs == rhs);
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto operator!=(const List<T, InlineCapacity, Allocator>& lhs,
                          const List<T, InlineCapacity, Allocator>& rhs) -> bool
{
    return !(lhs == rhs);
}

template <typename T, unsigned InlineCapacityLHS, unsigned InlineCapacityRHS, typename Allocator>
constexpr auto operator<(const List<T, InlineCapacityLHS, Allocator>& lhs,
                         const List<T, InlineCapacityRHS, Allocator>& rhs) -> bool
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto operator<(const List<T, InlineCapacity, Allocator>& lhs,
                         const List<T, InlineCapacity, Allocator>& rhs) -> bool
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, unsigned InlineCapacityLHS, unsigned InlineCapacityRHS, typename Allocator>
constexpr auto operator>=(const List<T, InlineCapacityLHS, Allocator>& lhs,
                          const List<T, InlineCapacityRHS, Allocator>& rhs) -> bool
{
    return !(lhs < rhs);
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto operator>=(const List<T, InlineCapacity, Allocator>& lhs,
                          const List<T, InlineCapacity, Allocator>& rhs) -> bool
{
    return !(lhs < rhs);
}

template <typename T, unsigned InlineCapacityLHS, unsigned InlineCapacityRHS, typename Allocator>
constexpr auto operator>(const List<T, InlineCapacityLHS, Allocator>& lhs,
                         const List<T, InlineCapacityRHS, Allocator>& rhs) -> bool
{
    return rhs < lhs;
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto operator>(const List<T, InlineCapacity, Allocator>& lhs,
                         const List<T, InlineCapacity, Allocator>& rhs) -> bool
{
    return rhs < lhs;
}

template <typename T, unsigned InlineCapacityLHS, unsigned InlineCapacityRHS, typename Allocator>
constexpr auto operator<=(const List<T, InlineCapacityLHS, Allocator>& lhs,
                          const List<T, InlineCapacityRHS, Allocator>& rhs) -> bool
{
    return rhs >= lhs;
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto operator<=(const List<T, InlineCapacity, Allocator>& lhs,
                          const List<T, InlineCapacity, Allocator>& rhs) -> bool
{
    return rhs >= lhs;
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr void swap(List<T, InlineCapacity, Allocator>& lhs,
                    List<T, InlineCapacity, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    requires concepts::MoveInsertable<T, List<T, InlineCapacity, Allocator>, Allocator> &&
             concepts::Swappable<T>
{
    lhs.swap(rhs);
}

template <typename T, unsigned InlineCapacity, typename Allocator, typename U>
constexpr auto erase(List<T, InlineCapacity, Allocator>& v, const U& value) ->
    typename List<T, InlineCapacity, Allocator>::size_type
{
    const auto original_size = v.size();
    v.erase(std::remove(v.begin(), v.end(), value), v.end());
    return original_size - v.size();
}

template <typename T, unsigned InlineCapacity, typename Allocator, typename Pred>
constexpr auto erase_if(List<T, InlineCapacity, Allocator>& v, Pred pred) ->
    typename List<T, InlineCapacity, Allocator>::size_type
{
    const auto original_size = v.size();
    v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
    return original_size - v.size();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto begin(List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::iterator
{
    return v.begin();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto begin(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_iterator
{
    return v.begin();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto cbegin(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_iterator
{
    return begin(v);
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto end(List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::iterator
{
    return v.end();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto end(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_iterator
{
    return v.end();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto cend(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_iterator
{
    return end(v);
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto rbegin(List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::reverse_iterator
{
    return v.rbegin();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto rbegin(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_reverse_iterator
{
    return v.rbegin();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto crbegin(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_reverse_iterator
{
    return rbegin(v);
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto rend(List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::reverse_iterator
{
    return v.rend();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto rend(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_reverse_iterator
{
    return v.rend();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto crend(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_reverse_iterator
{
    return rend(v);
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto size(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::size_type
{
    return v.size();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto ssize(const List<T, InlineCapacity, Allocator>& v) noexcept -> std::common_type_t<
    std::ptrdiff_t,
    std::make_signed_t<typename List<T, InlineCapacity, Allocator>::size_type>>
{
    using ret_type = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(v.size())>>;
    return static_cast<ret_type>(v.size());
}

template <typename T, unsigned InlineCapacity, typename Allocator>
[[nodiscard]] constexpr auto empty(const List<T, InlineCapacity, Allocator>& v) noexcept -> bool
{
    return v.empty();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto data(List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::pointer
{
    return v.data();
}

template <typename T, unsigned InlineCapacity, typename Allocator>
constexpr auto data(const List<T, InlineCapacity, Allocator>& v) noexcept ->
    typename List<T, InlineCapacity, Allocator>::const_pointer
{
    return v.data();
}

template <typename InputIt,
          unsigned InlineCapacity = default_buffer_size<
              std::allocator<typename std::iterator_traits<InputIt>::value_type>>::value,
          typename Allocator = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
List(InputIt, InputIt, Allocator = Allocator())
    -> List<typename std::iterator_traits<InputIt>::value_type, InlineCapacity, Allocator>;

template <typename T,
          unsigned InlineCapacity =
              default_buffer_size<std::allocator<std::reference_wrapper<T>>>::value,
          typename Allocator = std::allocator<std::reference_wrapper<T>>>
using RefList = List<std::reference_wrapper<T>, InlineCapacity, Allocator>;

template <typename T,
          unsigned InlineCapacity = default_buffer_size<std::allocator<std::unique_ptr<T>>>::value,
          typename Allocator      = std::allocator<std::unique_ptr<T>>>
using UniquePtrList = List<std::unique_ptr<T>, InlineCapacity, Allocator>;

template <typename First,
          typename Second,
          unsigned InlineCapacity =
              default_buffer_size<std::allocator<std::pair<First, Second>>>::value,
          typename Allocator = std::allocator<std::pair<First, Second>>>
using PairList = List<std::pair<First, Second>, InlineCapacity, Allocator>;

} // namespace cer
