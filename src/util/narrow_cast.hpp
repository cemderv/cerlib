///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <exception>

namespace cer
{
template <class T, class U>
constexpr auto narrow_cast(U&& u) noexcept -> T
{
    return static_cast<T>(std::forward<U>(u));
}

struct narrowing_error : std::exception
{
    auto what() const noexcept -> const char* override
    {
        return "narrowing_error";
    }
};

// narrow() : a checked version of narrow_cast() that throws if the cast changed the value
template <class T, class U, std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
constexpr auto narrow(U u) -> T
{
    constexpr bool is_different_signedness = (std::is_signed_v<T> != std::is_signed_v<U>);

    const T t = narrow_cast<T>(
        u); // While this is technically undefined behavior in some cases (i.e., if the source value
            // is of floating-point type and cannot fit into the destination integral type), the
            // resultant behavior is benign on the platforms that we target (i.e., no hardware trap
            // representations are hit).

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    // Note: NaN will always throw, since NaN != NaN
    if (static_cast<U>(t) != u || (is_different_signedness && ((t < T{}) != (u < U{}))))
    {
        throw narrowing_error{};
    }
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    return t;
}

template <class T, class U, std::enable_if_t<!std::is_arithmetic_v<T>>* = nullptr>
constexpr auto narrow(U u) -> T
{
    const T t = narrow_cast<T>(u);

    if (static_cast<U>(t) != u)
    {
        throw narrowing_error{};
    }

    return t;
}
} // namespace cer
