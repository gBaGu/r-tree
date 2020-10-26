#pragma once
#include <stdexcept>


namespace rtree
{
    class DuplicateEntryException : public std::runtime_error
    {
    public:
        DuplicateEntryException(const std::string& msg)
            : std::runtime_error(msg) {}
    };

    class EmptyBoundingBoxException : public std::runtime_error
    {
    public:
        EmptyBoundingBoxException(const std::string& msg)
            : std::runtime_error(msg) {}
    };
} // namespace rtree