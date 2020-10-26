#pragma once
#include <algorithm>
#include <limits>
#include <map>
#include <optional>
#include <stack>
#include <stdexcept>
#include <vector>

#include "exception.h"
#include "iterator.hpp"
#include "node.hpp"
#include "settings.h"
#include "split.hpp"


namespace rtree
{
    template<typename T,
             std::enable_if_t<std::is_integral<T>::value, int> = 0>
    std::string toString(const T& v)
    {
        return std::to_string(v);
    }

    template<typename T,
             std::enable_if_t<not std::is_integral<T>::value, int> = 0>
    std::string toString(const T& v)
    {
        return v.toString();
    }


    template<typename DataType, typename SplitStrategy = LinearSplit>
    class Tree
    {
    public:
        Tree()
            : _minEntries(DefaultMinEntries), _maxEntries(DefaultMaxEntries) {}
        Tree(size_t minEntries, size_t maxEntries);
        void remove(DataType data);
        void insert(BoundingBox b, DataType data);

        bool empty() const { return begin() == end(); }
        /**
         * Find all entries whose bounding boxes are intersected by b
         */
        std::vector<Entry<DataType>> find(BoundingBox b) const;

        Iterator<DataType> begin() const { return Iterator<DataType>(_root); }
        Iterator<DataType> end() const { return Iterator<DataType>(); }

        size_t getMinEntries() const { return _minEntries; }
        size_t getMaxEntries() const { return _maxEntries; }

    private:
        void condense(node_ptr<DataType> node);
        void insertIgnoreIndex(BoundingBox b, DataType data);
        /**
         * Find node whose bounding box area will be increased as little as possible
         * after insertion of entry represented by b
        */
        node_ptr<DataType> findInsertCandidate(BoundingBox b) const;
        /**
         * Find node that is containing entry e
        */
        node_ptr<DataType> findContaining(Entry<DataType> e) const;

        bool needSplit(node_ptr<DataType> node) const
        {
            return node->size() > getMaxEntries();
        }

        split_result<DataType> split(node_ptr<DataType> node) const;

        std::optional<BoundingBox> getFromIndex(DataType data) const;
        void removeFromIndex(DataType data);
        void saveToIndex(DataType data, BoundingBox b);

        node_ptr<DataType> _root;
        std::map<DataType, BoundingBox> _indexedBoxes;
        size_t _minEntries;
        size_t _maxEntries;
    };


    template<typename DataType, typename SplitStrategy>
    Tree<DataType, SplitStrategy>::Tree(size_t minEntries, size_t maxEntries)
        : _minEntries(minEntries), _maxEntries(maxEntries)
    {
        if (_minEntries == 0) {
            _minEntries = 1;
        }
        if (_minEntries * 2 > _maxEntries) {
            if (_maxEntries > std::numeric_limits<decltype(_maxEntries)>::max() / 2) {
                _minEntries = _maxEntries / 2;
            }
            else {
                _maxEntries = _minEntries * 2;
            }
        }
    }


    template<typename DataType, typename SplitStrategy>
    void Tree<DataType, SplitStrategy>::remove(DataType data)
    {
        const auto indexedBox = getFromIndex(data);
        removeFromIndex(data);

        // Find and remove entry by its id
        node_ptr<DataType> node = nullptr;
        if (indexedBox.has_value()) {
            const auto boxToDelete = indexedBox.value();
            Entry<DataType> target = { .box=boxToDelete, .data=data };
            node = findContaining(target);
            if (!node) {
                return;
            }
            node->remove(target);
        }
        else {
            const auto nodeIt = std::find_if(begin(), end(), [&data](auto& node) {
                if (!node.isLeaf()) {
                    return false;
                }
                return node.remove(data);
            });
            if (nodeIt == end()) {
                return;
            }
            node = nodeIt.get();
        }
        condense(node);
        if (!empty() && !_root->isLeaf() && _root->size() == 1) {
            _root = _root->getChildren()[0];
        }
    }

    template<typename DataType, typename SplitStrategy>
    void Tree<DataType, SplitStrategy>::insert(BoundingBox b, DataType data)
    {
        if (b.isEmpty()) {
            throw EmptyBoundingBoxException("insert() error: bounding box is empty");
        }
        saveToIndex(data, b);
        insertIgnoreIndex(b, data);
    }

    template<typename DataType, typename SplitStrategy>
    std::vector<Entry<DataType>> Tree<DataType, SplitStrategy>::find(BoundingBox b) const
    {
        std::vector<Entry<DataType>> intersected;
        std::stack<node_ptr<DataType>> stack { { _root } };
        while (!stack.empty()) {
            const auto node = stack.top();
            stack.pop();
            if (node->isLeaf()) {
                for (const auto& entry: node->getEntries()) {
                    const auto box = entry.box;
                    if (box.intersects(b)) {
                        intersected.push_back(entry);
                    }
                }
            }
            else {
                const auto& children = node->getChildren();
                for (const auto& child: children) {
                    stack.emplace(child); // TODO: emplace only if children is intersected
                }
            }
        }
        return intersected;
    }

    template<typename DataType, typename SplitStrategy>
    void Tree<DataType, SplitStrategy>::condense(node_ptr<DataType> node)
    {
        std::vector<node_ptr<DataType>> removed;
        auto current = node;
        // Go all the way up till we find node that doesn`t need to be reinserted
        while (current != _root) {
            const auto parent = current->getParent();
            if (current->size() < DefaultMinEntries) {
                parent->removeChild(current);
                removed.push_back(current);
            }
            else {
                current->updateBoundingBoxes();
                break;
            }
            current = parent;
        }
        for (const auto& node: removed) {
            for (const auto& entry: node->getEntries()) {
                insertIgnoreIndex(entry.box, entry.data);
            }
        }
        if (_root->size() == 0) {
            _root = nullptr;
        }
    }

    template<typename DataType, typename SplitStrategy>
    void Tree<DataType, SplitStrategy>::insertIgnoreIndex(BoundingBox b, DataType data)
    {
        Entry<DataType> e = { .box=b, .data=data };
        if (!_root) {
            _root = Node<DataType>::makeNode(e);
            return;
        }

        auto nodeToInsert = findInsertCandidate(b);
        nodeToInsert->insert(e);
        auto node = nodeToInsert;
        while (needSplit(node)) {
            auto parent = node->getParent();
            const auto splitnodes = split(node);
            if (splitnodes.first && splitnodes.second) {
                if (parent) {
                    parent->removeChild(node); // TODO: can optimize here by skipping updateBoundingBox() call
                    parent->insertChild(splitnodes.first);
                    parent->insertChild(splitnodes.second);
                    splitnodes.first->setParent(parent);
                    splitnodes.second->setParent(parent);
                }
                else { // node is a root
                    auto newRoot = Node<DataType>::makeNode(splitnodes.first);
                    newRoot->insertChild(splitnodes.second);
                    splitnodes.first->setParent(newRoot);
                    splitnodes.second->setParent(newRoot);
                    _root = newRoot;
                    break;
                }
            }
            node = parent;
        }
    }

    template<typename DataType, typename SplitStrategy>
    node_ptr<DataType> Tree<DataType, SplitStrategy>::findInsertCandidate(BoundingBox b) const
    {
        auto node = _root;
        while (node->getEntries().empty()) {
            double minArea = 0.0;
            node_ptr<DataType> bestChild = nullptr;
            const auto& children = node->getChildren();
            if (children.empty()) {
                throw std::logic_error("Node has no children. Tree is probably corrupted");
            }
            for (const auto child: children) {
                const auto areaAfterInsert = (child->getBoundingBox() & b).area();
                if (!bestChild) {
                    bestChild = child;
                    minArea = areaAfterInsert;
                }
                else if (areaAfterInsert < minArea) {
                    bestChild = child;
                    minArea = areaAfterInsert;
                }
                else if (areaAfterInsert == minArea &&
                         child->getBoundingBox().area() < bestChild->getBoundingBox().area()) {
                    bestChild = child;
                    minArea = areaAfterInsert;
                }
            }
            node = bestChild;
        }
        return node;
    }

    template<typename DataType, typename SplitStrategy>
    node_ptr<DataType> Tree<DataType, SplitStrategy>::findContaining(Entry<DataType> e) const
    {
        if (!_root->getBoundingBox().overlaps(e.box)) {
            return nullptr;
        }

        if (_root->isLeaf()) {
            return std::find(_root->getEntries().begin(), _root->getEntries().end(), e) != 
                   _root->getEntries().end() ?
                _root :
                nullptr;
        }

        std::stack<node_ptr<DataType>> stack { { _root } };
        while (!stack.empty()) {
            const auto node = stack.top();
            stack.pop();
            if (!node->isLeaf()) {
                const auto& children = node->getChildren();
                for (auto it = children.begin(); it != children.end(); it++) {
                    if ((*it)->getBoundingBox().overlaps(e.box)) {
                        if ((*it)->isLeaf()) {
                            if (std::find((*it)->getEntries().begin(), (*it)->getEntries().end(), e) !=
                                (*it)->getEntries().end()) {
                                return (*it);
                            }
                        }
                        else {
                            stack.emplace((*it));
                        }
                    }
                }
            }
        }
        return nullptr;
    }


    template<typename DataType, typename SplitStrategy>
    split_result<DataType> Tree<DataType, SplitStrategy>::split(node_ptr<DataType> node) const
    {
        if (node->size() <= 1) {
            return std::make_pair(nullptr, nullptr);
        }

        if (node->isLeaf()) {
            return SplitStrategy::splitLeaf(node);
        }
        else { // not leaf
            return SplitStrategy::splitInner(node);
        }
    }


    template<typename DataType, typename SplitStrategy>
    std::optional<BoundingBox> Tree<DataType, SplitStrategy>::getFromIndex(DataType data) const
    {
        const auto it = _indexedBoxes.find(data);
        if (it != _indexedBoxes.end()) {
            return it->second;
        }
        return {};
    }

    template<typename DataType, typename SplitStrategy>
    void Tree<DataType, SplitStrategy>::removeFromIndex(DataType data)
    {
        _indexedBoxes.erase(data);
    }

    template<typename DataType, typename SplitStrategy>
    void Tree<DataType, SplitStrategy>::saveToIndex(DataType data, BoundingBox b)
    {
        const auto inserted = _indexedBoxes.insert(std::make_pair(data, b));
        if (!inserted.second) {
            throw DuplicateEntryException("saveToIndex() error: entry " + toString(data) + " is already exists");
        }
    }
} // namespace rtree