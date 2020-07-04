#pragma once
#include <functional>
#include <stack>

#include "node.hpp"


namespace rtree
{
    template<typename T>
    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Node<T>;
        using difference_type = int;
        using pointer = node_ptr<T>;
        using reference = Node<T>&;

        Iterator(const Iterator<T>& other) = default;
        Iterator(Iterator<T>&& other) = default;
        Iterator(pointer ptr = nullptr);
        ~Iterator() {}

        Iterator& operator=(const Iterator<T>& other) = default;
        Iterator& operator=(Iterator<T>&& other) = default;
        Iterator& operator=(pointer ptr);

        operator bool() const { return !_stack.empty(); }
        bool operator==(const Iterator<T>& other) const { return _stack.size() == other._stack.size() && _stack == other._stack; }
        bool operator!=(const Iterator<T>& other) const { return !(*this == other); }
        Iterator& operator++();
        Iterator operator++(int);
        reference operator*() { return *_stack.top(); }
        const reference operator*() const { return *_stack.top(); }
        pointer operator->() { return _stack.top(); }


    private:
        std::stack<pointer> _stack;
    };


    template<typename T>
    Iterator<T>::Iterator(pointer ptr)
    {
        if (ptr) {
            _stack.push(ptr);
        }
    }

    template<typename T>
    Iterator<T>& Iterator<T>::operator=(pointer ptr)
    {
        _stack.clear();
        if (ptr) {
            _stack.push(ptr);
        }
    }

    template<typename T>
    Iterator<T>& Iterator<T>::operator++()
    {
        const auto node = _stack.top();
        _stack.pop();
        if (!node->isLeaf()) {
            for (auto it = node->getChildren().rbegin(); it != node->getChildren().rend(); it++) {
                _stack.emplace(*it);
            }
        }
        return *this;
    }

    template<typename T>
    Iterator<T> Iterator<T>::operator++(int)
    {
        auto tmp = *this;
        operator++();
        return tmp;
    }
} // namespace rtree