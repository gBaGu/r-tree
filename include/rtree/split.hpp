#pragma once
#include <iterator>
#include <utility>

#include "node.hpp"
#include "settings.h"


namespace rtree {


    template <typename T>
    class Node;

    template <typename T>
    struct Entry;

    template <typename T>
    using node_ptr = std::shared_ptr<Node<T>>;

    template <typename T>
    using split_result = std::pair<node_ptr<T>, node_ptr<T>>;


    class LinearSplit
    {
    public:
        template <typename T>
        static split_result<T> splitInner(node_ptr<T> node);

        template <typename T>
        static split_result<T> splitLeaf(node_ptr<T> node);

    private:
        template <typename T>
        static decltype(std::declval<T>()->getBoundingBox().distance(std::declval<BoundingBox>())) distance(const T& l, const T& r)
        {
            return l->getBoundingBox().distance(r->getBoundingBox());
        }

        template <typename T>
        static decltype(std::declval<T>().box.distance(std::declval<BoundingBox>())) distance(const T& l, const T& r)
        {
            return l.box.distance(r.box);
        }

        template <typename Iter>
        static std::pair <typename std::iterator_traits<Iter>::value_type,
                          typename std::iterator_traits<Iter>::value_type>
            pickSeeds(Iter begin, Iter end);
    };


    template <typename Iter>
    std::pair <typename std::iterator_traits<Iter>::value_type,
               typename std::iterator_traits<Iter>::value_type>
        LinearSplit::pickSeeds(Iter begin, Iter end)
    {
        decltype(std::declval<LinearSplit>().pickSeeds(begin, end)) seeds;
        double maxDistance = -1.0;
        for (auto it1 = begin; it1 != end; it1++) {
            for (auto it2 = std::next(it1); it2 != end; it2++) {
                const auto dist = distance(*it1, *it2);
                if (maxDistance == -1.0 || dist > maxDistance) {
                    seeds = std::make_pair(*it1, *it2);
                    maxDistance = dist;
                }
            }
        }
        return seeds;
    }

    template <typename T>
    split_result<T> LinearSplit::splitInner(node_ptr<T> node)
    {
        const auto& children = node->getChildren();
        const auto seeds = pickSeeds(children.begin(), children.end());

        std::vector<node_ptr<T>> otherChildren(children.size() - 2);
        std::remove_copy_if(children.begin(), children.end(),
                            otherChildren.begin(),
                            [&](const auto& child) {
                                return child == seeds.first || child == seeds.second;
                            });

        auto ret = std::make_pair(Node<T>::makeNode(seeds.first), Node<T>::makeNode(seeds.second));
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

    template <typename T>
    split_result<T> LinearSplit::splitLeaf(node_ptr<T> node)
    {
        const auto& entries = node->getEntries();
        const auto seeds = pickSeeds(entries.begin(), entries.end());

        std::vector<Entry<T>> otherEntries(entries.size() - 2);
        std::remove_copy_if(entries.begin(), entries.end(),
                            otherEntries.begin(),
                            [&](const auto& entry) {
                                return entry == seeds.first || entry == seeds.second;
                            });
        
        auto ret = std::make_pair(Node<T>::makeNode(seeds.first), Node<T>::makeNode(seeds.second));
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