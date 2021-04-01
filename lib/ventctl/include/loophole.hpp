#pragma once

/**
 * Slightly rewritten compile time typeid example by Sasha Sobol
 * http://coliru.stacked-crooked.com/a/6094c5aa5e75e240
 */

#include <type_traits>
#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"

#include <type-loophole.h>

namespace util
{
    template<typename T, typename TAG = void, int N = 0,
        bool b = (sizeof(loophole_ns::c_op<TAG, N>::template ins<T, N>(0)) == sizeof(char))>
    struct type_id {
        const static int value = type_id<T, TAG, N + 1>::value;
    };

    template <typename T, typename TAG, int N>
    struct type_id<T, TAG, N, false> {
        typedef decltype(static_cast<T*>(loophole_ns::c_op<TAG, N>{}), 0) X;
        const static int value = N;
    };

    template <int N, typename TAG=void>
    struct type_by_id {
        typedef decltype(loophole(loophole_ns::tag<TAG, N>())) type;
    };

    template<typename T>
    constexpr int type_id_v = type_id<T>::value;
}

#pragma GCC diagnostic pop