#include <boost/test/unit_test.hpp>

#include <rtree/rtree.hpp>

#include <algorithm>
#include <iterator>
#include <vector>


BOOST_AUTO_TEST_SUITE(exponential_split)

BOOST_AUTO_TEST_CASE(insert_into_root_and_split)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree(2, 4);
    for (size_t i = 0; i < tree.getMaxEntries() + 1; i++) {
        const rtree::BoundingBox box(i*0.1, i*0.1, 0.2, 0.2);
        tree.insert(box, i);
    }

    const rtree::BoundingBox root(0, 0, 1.2, 1.2);
    // Check that root node is not a leaf now
    auto nodeIt = tree.begin();
    BOOST_CHECK(not nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 2);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
    //BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == root, "Root node bounding box is incorrect");

    // Check that root node has two children after split
    nodeIt = std::next(nodeIt);
    const auto firstChildSize = nodeIt->size();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->depth(), 1);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), tree.begin().get());

    nodeIt = std::next(nodeIt);
    const auto secondChildSize = nodeIt->size();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->depth(), 1);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), tree.begin().get());

    // Check that overall size increased by 1
    BOOST_CHECK_EQUAL(firstChildSize + secondChildSize, tree.getMaxEntries() + 1);
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end());
}

BOOST_AUTO_TEST_CASE(remove_with_condense_followed_with_split)
{
    int indexCounter = 0;
    rtree::Tree<int, rtree::ExponentialSplit> tree(2, 4);

    // Insert entries to fill up root node
    for (size_t i = 0; i < tree.getMinEntries(); i++) {
        const rtree::BoundingBox box(5.0+i, 5.0+i, 5, 5);
        tree.insert(box, indexCounter++);
    }
    for (size_t i = 0; i < tree.getMaxEntries() - tree.getMinEntries(); i++) {
        const rtree::BoundingBox box(100.0+i, 100.0+i, 5, 5);
        tree.insert(box, indexCounter++);
    }
    // Insert one more to trigger split
    tree.insert({ 100, 100, 1, 1 }, indexCounter++);
    // Insert one more to have total number of entries equal to tree.getMaxEntries() + 2
    //  so everything won`t fit into one node after we remove one entry
    tree.insert({ 101, 101, 1, 1 }, indexCounter++);

    // Remove from node with minimum number of entries to trigger compaction and split
    tree.remove(0);

    auto nodeIt = tree.begin();
    const auto root = nodeIt.get();
    BOOST_CHECK(not nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 2);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);

    nodeIt = std::next(nodeIt);
    const auto firstChildSize = nodeIt->size();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->depth(), 1);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), root);

    nodeIt = std::next(nodeIt);
    const auto secondChildSize = nodeIt->size();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->depth(), 1);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), root);

    BOOST_CHECK_EQUAL(firstChildSize + secondChildSize, tree.getMaxEntries() + 1);
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end());
}

BOOST_AUTO_TEST_SUITE_END()
