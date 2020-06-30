#pragma once
#include <stdexcept>

#include "iterator.hpp"
#include "node.hpp"
#include "settings.h"


namespace rtree
{
    template<typename DataType>
    class Tree
    {
    public:
        void insert(BoundingBox b, DataType data);

        Iterator<DataType> begin() const { return Iterator<DataType>(_root); }
        Iterator<DataType> end() const { return Iterator<DataType>(); }

    private:
        node_ptr<DataType> findInsertCandidate(BoundingBox b) const;

        // std::vector<node_ptr> _nodes; // Do I need this?
        node_ptr<DataType> _root;
    };


    template<typename DataType>
    void Tree<DataType>::insert(BoundingBox b, DataType data)
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
} // namespace rtree