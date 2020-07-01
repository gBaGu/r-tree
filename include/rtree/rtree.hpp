#pragma once
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <vector>

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
        std::vector<Entry<DataType>> findIntersected(BoundingBox b) const;
        void print() const;

    private:
        node_ptr<DataType> findInsertCandidate(BoundingBox b) const;

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
                for (const auto& child: node->getChildren()) {
                    stack.emplace(child);
                }
            }
        }
        return intersected;
    }

    template<typename DataType>
    void Tree<DataType>::print() const
    {
        std::cout << "Printing R-tree:\n";
        std::stack<std::pair<int, node_ptr<DataType>>> stack { { std::make_pair(0, _root) } };
        while (!stack.empty()) {
            const auto [indent, node] = stack.top();
            stack.pop();
            if (node->isLeaf()) {
                const auto box = node->getBoundingBox();
                std::cout << std::string(indent, ' ')
                    << "(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
                for (const auto& entry: node->getEntries()) {
                    const auto box = entry.box;
                    std::cout << std::string(indent, ' ')
                        << ".(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
                }
                std::cout << std::string(10, '-') << '\n';
            }
            else {
                const auto box = node->getBoundingBox();
                std::cout << std::string(indent, ' ')
                    << "(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
                const auto& children = node->getChildren();
                for (auto it = children.rbegin(); it != children.rend(); it++) {
                    stack.emplace(indent + 2, *it);
                }
            }
        }
        std::cout << std::endl;
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
        _cache.insert(data, b);
    }
} // namespace rtree