#pragma once
#include <algorithm>
#include <map>
#include <optional>
#include <stack>
#include <stdexcept>
#include <vector>

#include "iterator.hpp"
#include "node.hpp"
#include "settings.h"


namespace rtree
{
    template<typename DataType>
    class Tree
    {
    public:
        void remove(DataType data);
        void insert(BoundingBox b, DataType data);

        /**
         * Find all entries whose bounding boxes are intersected by b
         */
        std::vector<Entry<DataType>> find(BoundingBox b) const;

        Iterator<DataType> begin() const { return Iterator<DataType>(_root); }
        Iterator<DataType> end() const { return Iterator<DataType>(); }

    private:
        node_ptr<DataType> findInsertCandidate(BoundingBox b) const;
        node_ptr<DataType> findContaining(Entry<DataType> e) const;

        std::optional<BoundingBox> getFromCache(DataType data) const;
        void removeFromCache(DataType data);
        void saveToCache(DataType data, BoundingBox b);

        node_ptr<DataType> _root;
        std::map<DataType, BoundingBox> _cache;
    };


    template<typename DataType>
    void Tree<DataType>::remove(DataType data)
    {
        const auto cachedBox = getFromCache(data);
        removeFromCache(data);

        BoundingBox boxToDelete;
        if (cachedBox.has_value()) {
            boxToDelete = cachedBox.value();
        }
        else {
            // TODO: iterate through entries and find
        }

        const auto overlapped = findOverlappingLeafs(boxToDelete)
    }

    template<typename DataType>
    void Tree<DataType>::insert(BoundingBox b, DataType data)
    {
        saveToCache(data, b);
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
    std::vector<Entry<DataType>> Tree<DataType>::findIntersected(BoundingBox b) const
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
                const auto area = child->getBoundingBox().area();
                if (!bestChild) {
                    bestChild = child;
                    minArea = area;
                }
                else if (area < minArea) {
                    bestChild = child;
                    minArea = area;
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
            return std::find(_root->getEntries().begin(), _root->getEntries().end(), e) != _root->getEntries().end() ?
                _root :
                nullptr;
        }

        std::vector<node_ptr<DataType>> overlapped;
        std::stack<node_ptr<DataType>> stack { { _root } };
        while (!stack.empty()) {
            const auto node = stack.top();
            stack.pop();
            if (!node->isLeaf()) {
                const auto& children = node->getChildren();
                std::for_each(children.begin(), children.end(), [&](const auto& child) {
                    if (child->getBoundingBox().overlaps(b)) {
                        if (child->isLeaf()) {
                            overlapped.push_back(child);
                        }
                        else {
                            stack.emplace(child);
                        }
                    }
                });
            }
        }
        return overlapped;
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
        _cache.insert(std::make_pair(data, b));
    }
} // namespace rtree