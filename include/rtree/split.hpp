#pragma once
#include "node.hpp"
#include "settings.h"


namespace rtree {


    template<typename T>
    class Node;

    template<typename T>
    struct Entry;

    template<typename T>
    using node_ptr = std::shared_ptr<Node<T>>;

    template<typename T>
    using split_result = std::pair<node_ptr<T>, node_ptr<T>>;

    class LinearSplit
    {
    public:
        LinearSplit(size_t minEntries, size_t maxEntries)
            : _minEntries(minEntries), _maxEntries(maxEntries) {}

        size_t getMinEntries() const { return _minEntries; }
        size_t getMaxEntries() const { return _maxEntries; }

        template<typename T>
        bool needSplit(node_ptr<T> node) const
        {
            return node->getEntries().size() > _maxEntries;
        }

        template<typename T>
        split_result<T> split(node_ptr<T> node) const;

    private:
        size_t _minEntries;
        size_t _maxEntries;

        template<typename T>
        split_result<T> splitInner(node_ptr<T> node) const;

        template<typename T>
        split_result<T> splitLeaf(node_ptr<T> node) const;
    };


    template<typename T>
    split_result<T> LinearSplit::split(node_ptr<T> node) const
    {
        if (node->size() <= 1) {
            return std::make_pair(nullptr, nullptr);
        }

        if (node->isLeaf()) {
            return splitLeaf(node);
        }
        else { // not leaf
            return splitInner(node);
        }
    }

    template<typename T>
    split_result<T> LinearSplit::splitInner(node_ptr<T> node) const
    {
        // Choose two child nodes with the biggest distance
        std::pair<node_ptr<T>, node_ptr<T>> firstChildNodes = std::make_pair(nullptr, nullptr);
        double maxDistance = -1.0;
        const auto& children = node->getChildren();
        for (auto it1 = children.begin(); it1 != children.end(); it1++) {
            for (auto it2 = std::next(it1); it2 != children.end(); it2++) {
                const auto d = (*it1)->getBoundingBox().distance((*it2)->getBoundingBox());
                if (maxDistance == -1.0 || d > maxDistance) {
                    firstChildNodes = std::make_pair(*it1, *it2);
                    maxDistance = d;
                }
            }
        }

        std::vector<node_ptr<T>> otherChildren(children.size() - 2);
        std::remove_copy_if(children.begin(), children.end(),
                            otherChildren.begin(),
                            [&](const auto& child) {
                                return child == firstChildNodes.first || child == firstChildNodes.second;
                            });

        auto ret = std::make_pair(Node<T>::makeNode(firstChildNodes.first), Node<T>::makeNode(firstChildNodes.second));
        ret.first->setParent(node->getParent());
        ret.second->setParent(node->getParent());
        std::random_shuffle(std::begin(otherChildren), std::end(otherChildren));
        for (const auto& child: otherChildren) {
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

    template<typename T>
    split_result<T> LinearSplit::splitLeaf(node_ptr<T> node) const
    {
        std::pair<Entry<T>, Entry<T>> firstEntries;
        double maxDistance = -1.0;
        const auto& entries = node->getEntries();
        for (auto it1 = entries.begin(); it1 != entries.end(); it1++) {
            for (auto it2 = std::next(it1); it2 != entries.end(); it2++) {
                const auto d = it1->box.distance(it2->box);
                if (maxDistance == -1.0 || d > maxDistance) {
                    firstEntries = std::make_pair(*it1, *it2);
                    maxDistance = d;
                }
            }
        }

        std::vector<Entry<T>> otherEntries(entries.size() - 2);
        std::remove_copy_if(entries.begin(), entries.end(),
                            otherEntries.begin(),
                            [&](const auto& entry) {
                                return entry == firstEntries.first || entry == firstEntries.second;
                            });
        
        auto ret = std::make_pair(Node<T>::makeNode(firstEntries.first), Node<T>::makeNode(firstEntries.second));
        ret.first->setParent(node->getParent());
        ret.second->setParent(node->getParent());
        std::random_shuffle(std::begin(otherEntries), std::end(otherEntries));
        for (const auto& entry: otherEntries) {
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
} // namespace rtree