#pragma once
#include <iterator>
#include <type_traits>
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


    template <typename Strategy>
    class Split
    {
    public:
        Split(size_t minEntries, size_t maxEntries)
            : _minEntries(minEntries), _maxEntries(maxEntries) {}

        size_t getMinEntries() const { return _minEntries; }
        size_t getMaxEntries() const { return _maxEntries; }

        template <typename T>
        bool needSplit(node_ptr<T> node) const
        {
            return node->getEntries().size() > _maxEntries;
        }

        template <typename T>
        split_result<T> split(node_ptr<T> node) const;

    private:
        size_t _minEntries;
        size_t _maxEntries;


        template <typename T>
        split_result<T> splitInner(node_ptr<T> node) const;

        template <typename T>
        split_result<T> splitLeaf(node_ptr<T> node) const;
    };


    template <typename Strategy>
    template <typename T>
    split_result<T> Split<Strategy>::split(node_ptr<T> node) const
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

    template <typename Strategy>
    template <typename T>
    split_result<T> Split<Strategy>::splitInner(node_ptr<T> node) const
    {
        const auto& children = node->getChildren();
        const auto seeds = Strategy::pickSeeds(children.begin(), children.end());

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
            Strategy::insertToBest(child, ret.first, ret.second);
        }
        return ret;
    }

    template <typename Strategy>
    template <typename T>
    split_result<T> Split<Strategy>::splitLeaf(node_ptr<T> node) const
    {
        const auto& entries = node->getEntries();
        const auto seeds = Strategy::pickSeeds(entries.begin(), entries.end());

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
            Strategy::insertToBest(entry, ret.first, ret.second);
        }
        return ret;
    }


    class LinearSplit
    {
    public:
        template <typename Iter>
        static std::pair<typename std::iterator_traits<Iter>::value_type,
                         typename std::iterator_traits<Iter>::value_type>
            pickSeeds(Iter begin, Iter end)
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

        template <typename Bounded, typename T>
        static std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Bounded>()->getBoundingBox())>>, BoundingBox>, void>
            insertToBest(const Bounded& bounded, node_ptr<T> node1, node_ptr<T> node2)
        {
            if ((node1->getBoundingBox() & bounded->getBoundingBox()).area() <
                (node2->getBoundingBox() & bounded->getBoundingBox()).area()) {
                node1->insertChild(bounded);
            }
            else {
                node2->insertChild(bounded);
            }
        }

        template <typename Bounded, typename T>
        static std::enable_if_t<std::is_same_v<decltype(std::declval<Bounded>().box), BoundingBox>, void>
            insertToBest(const Bounded& bounded, node_ptr<T> node1, node_ptr<T> node2)
        {
            if ((node1->getBoundingBox() & bounded.box).area() <
                (node2->getBoundingBox() & bounded.box).area()) {
                node1->insert(bounded);
            }
            else {
                node2->insert(bounded);
            }
        }

    private:
        template <typename Bounded>
        static decltype(std::declval<Bounded>()->getBoundingBox().distance(std::declval<BoundingBox>()))
            distance(const Bounded& l, const Bounded& r)
        {
            return l->getBoundingBox().distance(r->getBoundingBox());
        }

        template <typename Bounded>
        static decltype(std::declval<Bounded>().box.distance(std::declval<BoundingBox>()))
            distance(const Bounded& l, const Bounded& r)
        {
            return l.box.distance(r.box);
        }
    };


    class QuadraticSplit
    {
    public:
        template <typename Iter>
        static std::pair<typename std::iterator_traits<Iter>::value_type,
                         typename std::iterator_traits<Iter>::value_type>
            pickSeeds(Iter begin, Iter end)
        {
            decltype(std::declval<LinearSplit>().pickSeeds(begin, end)) seeds;
            double maxDeadSpace = -1.0;
            for (auto it1 = begin; it1 != end; it1++) {
                for (auto it2 = std::next(it1); it2 != end; it2++) {
                    const auto ds = deadSpace(*it1, *it2);
                    if (maxDeadSpace == -1.0 || ds > maxDeadSpace) {
                        seeds = std::make_pair(*it1, *it2);
                        maxDeadSpace = ds;
                    }
                }
            }
            return seeds;
        }

        template <typename Bounded, typename T>
        static std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Bounded>()->getBoundingBox())>>, BoundingBox>, void>
            insertToBest(const Bounded& bounded, node_ptr<T> node1, node_ptr<T> node2)
        {
            // TODO: implement
        }

        template <typename Bounded, typename T>
        static std::enable_if_t<std::is_same_v<decltype(std::declval<Bounded>().box), BoundingBox>, void>
            insertToBest(const Bounded& bounded, node_ptr<T> node1, node_ptr<T> node2)
        {
            // TODO: implement
        }

    private:
        template <typename Bounded>
        static decltype(std::declval<Bounded>()->getBoundingBox().area())
            deadSpace(const Bounded& l, const Bounded& r)
        {
            return (l->getBoundingBox() & r->getBoundingBox()).area() +
                (l->getBoundingBox() | r->getBoundingBox()).area() -
                l->getBoundingBox().area() -
                r->getBoundingBox().area();
        }

        template <typename Bounded>
        static decltype(std::declval<Bounded>().box.area())
            deadSpace(const Bounded& l, const Bounded& r)
        {
            return (l.box & r.box).area() +
                (l.box | r.box).area() -
                l.box.area() -
                r.box.area();
        }
    };
} // namespace rtree
