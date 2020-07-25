#pragma once
#include <algorithm>
#include <memory>
#include <vector>

#include "bounding_box.h"


namespace rtree
{
    template<typename DataType>
    class Node;

    template<typename DataType>
    using node_ptr = std::shared_ptr<Node<DataType>>;


    template<typename DataType>
    struct Entry
    {
        BoundingBox box;
        DataType data;

        bool operator==(const Entry& other) const { return box == other.box && data == other.data; }
    };


    template<typename DataType>
    class Node
    {
        using split_result = std::pair<node_ptr<DataType>, node_ptr<DataType>>;
    public:
        //TODO: create more constructors
        explicit Node(node_ptr<DataType> child); // TODO: rework because this can be thought of as a copy constructor
        explicit Node(Entry<DataType> entry);

        static node_ptr<DataType> makeNode(node_ptr<DataType> child) { return std::make_shared<Node<DataType>>(child); }
        static node_ptr<DataType> makeNode(Entry<DataType> entry) { return std::make_shared<Node<DataType>>(entry); }

        void expandBoundingBox(BoundingBox b);
        void insert(const Entry<DataType>& e);
        bool remove(const Entry<DataType>& e);
        bool remove(DataType data);
        void insertChild(node_ptr<DataType> node);
        void removeChild(node_ptr<DataType> node);
        void setParent(node_ptr<DataType> node) { _parent = node; }
        split_result split();
        void updateBoundingBoxes();

        size_t                                 depth() const;
        const BoundingBox&                     getBoundingBox() const { return _boundingBox; }
        const std::vector<node_ptr<DataType>>& getChildren() const { return _children; }
        const std::vector<Entry<DataType>>&    getEntries() const { return _entries; }
        node_ptr<DataType>                     getParent() const { return _parent; }
        bool                                   isLeaf() const { return !_entries.empty(); }
        size_t                                 size() const { return isLeaf() ? _entries.size() : _children.size(); }

    private:
        BoundingBox _boundingBox;
        node_ptr<DataType> _parent;
        std::vector<node_ptr<DataType>> _children;
        std::vector<Entry<DataType>> _entries;

        split_result splitInner();
        split_result splitLeaf();
        void updateBoundingBox();
    };


    template<typename DataType>
    Node<DataType>::Node(node_ptr<DataType> child)
        : _boundingBox(child->getBoundingBox()), _children({ child })
    {
    }

    template<typename DataType>
    Node<DataType>::Node(Entry<DataType> entry)
        : _boundingBox(entry.box), _entries({ entry })
    {
    }

    template<typename DataType>
    void Node<DataType>::expandBoundingBox(BoundingBox b)
    {
        _boundingBox = _boundingBox & b;
    }

    template<typename DataType>
    void Node<DataType>::insert(const Entry<DataType>& e)
    {
        _entries.push_back(e);
        expandBoundingBox(e.box);
        auto node = _parent;
        while (node) {
            node->expandBoundingBox(e.box);
            node = node->getParent();
        }
    }

    template<typename DataType>
    bool Node<DataType>::remove(const Entry<DataType>& e)
    {
        const auto toErase = std::remove(_entries.begin(), _entries.end(), e);
        bool removed = toErase != _entries.end();
        _entries.erase(toErase, _entries.end());
        updateBoundingBoxes();
        return removed;
    }

    template<typename DataType>
    bool Node<DataType>::remove(DataType data)
    {
        const auto toErase = std::remove_if(_entries.begin(), _entries.end(),
            [&data](const auto& entry) { return entry.data == data; });
        bool removed = toErase != _entries.end();
        _entries.erase(toErase, _entries.end());
        updateBoundingBoxes();
        return removed;
    }

    template<typename DataType>
    void Node<DataType>::insertChild(node_ptr<DataType> n)
    {
        _children.push_back(n);
        expandBoundingBox(n->getBoundingBox());
        auto node = _parent;
        while (node) {
            node->expandBoundingBox(n->getBoundingBox());
            node = node->getParent();
        }
    }

    template<typename DataType>
    void Node<DataType>::removeChild(node_ptr<DataType> n)
    {
        _children.erase(std::remove(_children.begin(), _children.end(), n), _children.end());
        updateBoundingBoxes();
    }

    template<typename DataType>
    typename Node<DataType>::split_result Node<DataType>::split()
    {
        if (_entries.size() <= 1 && _children.size() <= 1) {
            return std::make_pair(nullptr, nullptr);
        }

        if (isLeaf()) {
            return splitLeaf();
        }
        else { // not leaf
            return splitInner();
        }
    }

    template<typename DataType>
    void Node<DataType>::updateBoundingBoxes()
    {
        updateBoundingBox();
        auto node = _parent;
        while (node) {
            node->updateBoundingBox();
            node = node->getParent();
        }
    }

    template<typename DataType>
    size_t Node<DataType>::depth() const
    {
        size_t d = 0;
        auto parent = getParent();
        while (parent) {
            d++;
            parent = parent->getParent();
        }
        return d;
    }

    template<typename DataType>
    typename Node<DataType>::split_result Node<DataType>::splitInner()
    {
        std::pair<node_ptr<DataType>, node_ptr<DataType>> firstChildNodes = std::make_pair(nullptr, nullptr);
        double maxDistance = -1.0;
        for (auto it1 = _children.begin(); it1 != _children.end(); it1++) {
            for (auto it2 = std::next(it1); it2 != _children.end(); it2++) {
                const auto d = (*it1)->getBoundingBox().distance((*it2)->getBoundingBox());
                if (maxDistance == -1.0 || d > maxDistance) {
                    firstChildNodes = std::make_pair(*it1, *it2);
                    maxDistance = d;
                }
            }
        }
        _children.erase(std::remove(_children.begin(), _children.end(), firstChildNodes.first), _children.end());
        _children.erase(std::remove(_children.begin(), _children.end(), firstChildNodes.second), _children.end());
        // This causes compilation error "declared using local type ... is used but never defined [-fpermissive]"
        // _children.erase(std::remove_if(_children.begin(), _children.end(),
        //         [&](const node_ptr<DataType>& child) { return child == firstChildNodes.first || child == firstChildNodes.second; }),
        //     _children.end()); // maybe make a copy of _children to be able to revert easily

        split_result ret = std::make_pair(makeNode(firstChildNodes.first), makeNode(firstChildNodes.second));
        ret.first->setParent(getParent());
        ret.second->setParent(getParent());
        std::random_shuffle(std::begin(_children), std::end(_children));
        for (const auto& child: _children) {
            if ((ret.first->getBoundingBox() & child->getBoundingBox()).area() <
                (ret.second->getBoundingBox() & child->getBoundingBox()).area()) {
                ret.first->insertChild(child);
            }
            else {
                ret.second->insertChild(child);
            }
        }
        return ret;
    }

    template<typename DataType>
    typename Node<DataType>::split_result Node<DataType>::splitLeaf()
    {
        std::pair<Entry<DataType>, Entry<DataType>> firstEntries;
        double maxDistance = -1.0;
        for (auto it1 = _entries.begin(); it1 != _entries.end(); it1++) {
            for (auto it2 = std::next(it1); it2 != _entries.end(); it2++) {
                const auto d = it1->box.distance(it2->box);
                if (maxDistance == -1.0 || d > maxDistance) {
                    firstEntries = std::make_pair(*it1, *it2);
                    maxDistance = d;
                }
            }
        }
        _entries.erase(std::remove(_entries.begin(), _entries.end(), firstEntries.first), _entries.end());
        _entries.erase(std::remove(_entries.begin(), _entries.end(), firstEntries.second), _entries.end());
        // This causes compilation error "declared using local type ... is used but never defined [-fpermissive]"
        // _entries.erase(std::remove_if(_entries.begin(), _entries.end(),
        //         [&](const Entry<DataType>& entry) { return entry == firstEntries.first || entry == firstEntries.second; }),
        //     _entries.end());
        
        split_result ret = std::make_pair(makeNode(firstEntries.first), makeNode(firstEntries.second));
        ret.first->setParent(getParent());
        ret.second->setParent(getParent());
        std::random_shuffle(std::begin(_entries), std::end(_entries));
        for (const auto& entry: _entries) {
            if ((ret.first->getBoundingBox() & entry.box).area() <
                (ret.second->getBoundingBox() & entry.box).area()) {
                ret.first->insert(entry);
            }
            else {
                ret.second->insert(entry);
            }
        }
        return ret;
    }

    template<typename DataType>
    void Node<DataType>::updateBoundingBox()
    {
        if (_entries.empty() && _children.empty()) {
            return;
        }

        if (isLeaf()) {
            auto box = _entries.front().box;
            for (const auto& entry: _entries) {
                box = box & entry.box;
            }
            _boundingBox = box;
        }
        else {
            auto box = _children.front()->getBoundingBox();
            for (const auto& child: _children) {
                box = box & child->getBoundingBox();
            }
            _boundingBox = box;
        }
    }
} // namespace rtree