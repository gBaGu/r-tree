#pragma once
#include <algorithm>
#include <map>
#include <optional>
#include <stack>
#include <stdexcept>
#include <vector>

#include "exception.h"
#include "iterator.hpp"
#include "node.hpp"
#include "settings.h"


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


    template<typename DataType>
    class Tree
    {
    public:
        Tree() : minEntries(DefaultMinEntries), maxEntries(DefaultMaxEntries) {}
        void remove(DataType data);
        void insert(BoundingBox b, DataType data);

        bool empty() const { return begin() == end(); }
        /**
         * Find all entries whose bounding boxes are intersected by b
         */
        std::vector<Entry<DataType>> find(BoundingBox b) const;

        Iterator<DataType> begin() const { return Iterator<DataType>(_root); }
        Iterator<DataType> end() const { return Iterator<DataType>(); }

        size_t getMinEntries() const { return minEntries; }
        size_t getMaxEntries() const { return maxEntries; }

    private:
        void condense(node_ptr<DataType> node);
        void insertIgnoreCache(BoundingBox b, DataType data);
        /**
         * Find node whose bounding box area will be increased as little as possible
         * after insertion of entry represented by b
        */
        node_ptr<DataType> findInsertCandidate(BoundingBox b) const;
        /**
         * Find node that is containing entry e
        */
        node_ptr<DataType> findContaining(Entry<DataType> e) const;

        std::optional<BoundingBox> getFromCache(DataType data) const;
        void removeFromCache(DataType data);
        void saveToCache(DataType data, BoundingBox b);

        node_ptr<DataType> _root;
        std::map<DataType, BoundingBox> _cache;
        size_t minEntries;
        size_t maxEntries;
    };


    template<typename DataType>
    void Tree<DataType>::remove(DataType data)
    {
        const auto cachedBox = getFromCache(data);
        removeFromCache(data);

        // Find and remove entry by its id
        node_ptr<DataType> node = nullptr;
        if (cachedBox.has_value()) {
            const auto boxToDelete = cachedBox.value();
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

    template<typename DataType>
    void Tree<DataType>::insert(BoundingBox b, DataType data)
    {
        saveToCache(data, b);
        insertIgnoreCache(b, data);
    }

    template<typename DataType>
    std::vector<Entry<DataType>> Tree<DataType>::find(BoundingBox b) const
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

    template<typename DataType>
    void Tree<DataType>::condense(node_ptr<DataType> node)
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
                insertIgnoreCache(entry.box, entry.data);
            }
        }
        if (_root->size() == 0) {
            _root = nullptr;
        }
    }

    template<typename DataType>
    void Tree<DataType>::insertIgnoreCache(BoundingBox b, DataType data)
    {
        Entry<DataType> e = { .box=b, .data=data };
        if (!_root) {
            _root = Node<DataType>::makeNode(e);
            return;
        }

        auto nodeToInsert = findInsertCandidate(b);
        nodeToInsert->insert(e);
        auto node = nodeToInsert;
        while (node->getEntries().size() > DefaultMaxEntries) {
            auto parent = node->getParent();
            const auto splitnodes = node->split();
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

    template<typename DataType>
    node_ptr<DataType> Tree<DataType>::findInsertCandidate(BoundingBox b) const
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

    template<typename DataType>
    node_ptr<DataType> Tree<DataType>::findContaining(Entry<DataType> e) const
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


    template<typename DataType>
    std::optional<BoundingBox> Tree<DataType>::getFromCache(DataType data) const
    {
        const auto it = _cache.find(data);
        if (it != _cache.end()) {
            return it->second;
        }
        return {};
    }

    template<typename DataType>
    void Tree<DataType>::removeFromCache(DataType data)
    {
        _cache.erase(data);
    }

    template<typename DataType>
    void Tree<DataType>::saveToCache(DataType data, BoundingBox b)
    {
        const auto inserted = _cache.insert(std::make_pair(data, b));
        if (!inserted.second) {
            throw DuplicateEntryException("saveToCache() error: entry " + toString(data) + " is already exists");
        }
    }
} // namespace rtree