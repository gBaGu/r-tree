#pragma once
#include <cstddef>


namespace rtree
{
    constexpr size_t DefaultMinEntries = 2;
    constexpr size_t DefaultMaxEntries = 10;

    static_assert(DefaultMinEntries <= DefaultMaxEntries / 2,
        "Minimum number of node entries must be less or equal to maximum number divided by 2.");
}