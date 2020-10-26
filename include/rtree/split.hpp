#pragma once
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
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


    class QuadraticSplit
    {
    public:
        template <typename T>
        static split_result<T> splitInner(node_ptr<T> node);

        template <typename T>
        static split_result<T> splitLeaf(node_ptr<T> node);

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

        template <typename Iter>
        static std::pair<typename std::iterator_traits<Iter>::value_type,
                         typename std::iterator_traits<Iter>::value_type>
            pickSeeds(Iter begin, Iter end)
        {
            decltype(std::declval<QuadraticSplit>().pickSeeds(begin, end)) seeds;
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
    };


    template <typename T>
    split_result<T> QuadraticSplit::splitInner(node_ptr<T> node)
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
        while (!otherChildren.empty()) {
            double maxDeadSpaceDiff = -1.0;
            size_t maxDeadSpaceId = -1;
            for (size_t i = 0; i < otherChildren.size(); i++) {
                const auto firstNodeDeadSpace = deadSpace(seeds.first, otherChildren[i]);
                const auto secondNodeDeadSpace = deadSpace(seeds.second, otherChildren[i]);
                const auto deadSpaceDiff = std::abs(firstNodeDeadSpace - secondNodeDeadSpace);
                if (maxDeadSpaceDiff == -1.0 ||
                    deadSpaceDiff > maxDeadSpaceDiff) {
                    maxDeadSpaceId = i;
                    maxDeadSpaceDiff = deadSpaceDiff;
                }
            }
            const auto& entry = otherChildren[maxDeadSpaceId];
            if ((ret.first->getBoundingBox() & entry->getBoundingBox()).area() <
                (ret.second->getBoundingBox() & entry->getBoundingBox()).area()) {
                ret.first->insertChild(entry);
            }
            else {
                ret.second->insertChild(entry);
            }
            if (maxDeadSpaceId != otherChildren.size() - 1) {
                std::swap(otherChildren[maxDeadSpaceId], otherChildren.back());
            }
            otherChildren.resize(otherChildren.size() - 1);
        }
        return ret;
    }

    template <typename T>
    split_result<T> QuadraticSplit::splitLeaf(node_ptr<T> node)
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
        while (!otherEntries.empty()) {
            double maxDeadSpaceDiff = -1.0;
            size_t maxDeadSpaceId = -1;
            for (size_t i = 0; i < otherEntries.size(); i++) {
                const auto firstNodeDeadSpace = deadSpace(seeds.first, otherEntries[i]);
                const auto secondNodeDeadSpace = deadSpace(seeds.second, otherEntries[i]);
                const auto deadSpaceDiff = std::abs(firstNodeDeadSpace - secondNodeDeadSpace);
                if (maxDeadSpaceDiff == -1.0 ||
                    deadSpaceDiff > maxDeadSpaceDiff) {
                    maxDeadSpaceId = i;
                    maxDeadSpaceDiff = deadSpaceDiff;
                }
            }
            const auto& entry = otherEntries[maxDeadSpaceId];
            if ((ret.first->getBoundingBox() & entry.box).area() <
                (ret.second->getBoundingBox() & entry.box).area()) {
                ret.first->insert(entry);
            }
            else {
                ret.second->insert(entry);
            }
            if (maxDeadSpaceId != otherEntries.size() - 1) {
                std::swap(otherEntries[maxDeadSpaceId], otherEntries.back());
            }
            otherEntries.resize(otherEntries.size() - 1);
        }
        return ret;
    }


    // Warning: not recommended for use when _maxEntries > 4
    class ExponentialSplit
    {
    public:
        template <typename T>
        static split_result<T> splitInner(node_ptr<T> node);

        template <typename T>
        static split_result<T> splitLeaf(node_ptr<T> node);

    private:
    };


    template <typename T>
    split_result<T> ExponentialSplit::splitInner(node_ptr<T> node)
    {
        const auto& children = node->getChildren();
        std::vector<size_t> perm(children.size());
        std::iota(perm.begin(), perm.end(), 0);
        double minTotalArea = -1.0;
        size_t bestSeparator = 0;
        std::vector<size_t> bestPermutation;
        do {
            for (size_t separator = 1; separator < children.size(); separator++) {
                BoundingBox firstbox;
                BoundingBox secondbox;
                std::for_each(perm.begin(), perm.begin() + separator,
                    [&](auto val) { firstbox = firstbox & children[val]->getBoundingBox(); });
                std::for_each(perm.begin() + separator, perm.end(),
                    [&](auto val) { secondbox = secondbox & children[val]->getBoundingBox(); });
                auto totalArea = firstbox.area() + secondbox.area();
                if (minTotalArea == -1.0 ||
                    totalArea < minTotalArea) {
                    minTotalArea = totalArea;
                    bestSeparator = separator;
                    bestPermutation = perm;
                }
            }
        } while (std::next_permutation(perm.begin(), perm.end()));
        std::vector<node_ptr<T>> firstNodeChildren(bestSeparator);
        std::vector<node_ptr<T>> secondNodeChildren(children.size() - bestSeparator);
        std::transform(bestPermutation.begin(), bestPermutation.begin() + bestSeparator,
                       firstNodeChildren.begin(),
                       [&](const auto& val) { return children[val]; });
        std::transform(bestPermutation.begin() + bestSeparator, bestPermutation.end(),
                       secondNodeChildren.begin(),
                       [&](const auto& val) { return children[val]; });
        return std::make_pair(Node<T>::makeInner(firstNodeChildren.begin(), firstNodeChildren.end()),
                              Node<T>::makeInner(secondNodeChildren.begin(), secondNodeChildren.end()));
    }

    template <typename T>
    split_result<T> ExponentialSplit::splitLeaf(node_ptr<T> node)
    {
        const auto& entries = node->getEntries();
        std::vector<size_t> perm(entries.size());
        std::iota(perm.begin(), perm.end(), 0);
        double minTotalArea = -1.0;
        size_t bestSeparator = 0;
        std::vector<size_t> bestPermutation;
        do {
            for (size_t separator = 1; separator < entries.size(); separator++) {
                BoundingBox firstbox;
                BoundingBox secondbox;
                std::for_each(perm.begin(), perm.begin() + separator,
                    [&](auto val) { firstbox = firstbox & entries[val].box; });
                std::for_each(perm.begin() + separator, perm.end(),
                    [&](auto val) { secondbox = secondbox & entries[val].box; });
                auto totalArea = firstbox.area() + secondbox.area();
                if (minTotalArea == -1.0 ||
                    totalArea < minTotalArea) {
                    minTotalArea = totalArea;
                    bestSeparator = separator;
                    bestPermutation = perm;
                }
            }
        } while (std::next_permutation(perm.begin(), perm.end()));
        std::vector<Entry<T>> firstNodeEntries(bestSeparator);
        std::vector<Entry<T>> secondNodeEntries(entries.size() - bestSeparator);
        std::transform(bestPermutation.begin(), bestPermutation.begin() + bestSeparator,
                       firstNodeEntries.begin(),
                       [&](const auto& val) { return entries[val]; });
        std::transform(bestPermutation.begin() + bestSeparator, bestPermutation.end(),
                       secondNodeEntries.begin(),
                       [&](const auto& val) { return entries[val]; });
        return std::make_pair(Node<T>::makeLeaf(firstNodeEntries.begin(), firstNodeEntries.end()),
                              Node<T>::makeLeaf(secondNodeEntries.begin(), secondNodeEntries.end()));
    }
} // namespace rtree
