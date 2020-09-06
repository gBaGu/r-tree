#include <boost/test/unit_test.hpp>

#include <rtree/rtree.hpp>

#include <algorithm>
#include <iterator>
#include <vector>


BOOST_AUTO_TEST_SUITE(exponential_split)

BOOST_AUTO_TEST_CASE(creation)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    BOOST_REQUIRE_EQUAL(tree.getMaxEntries(), rtree::DefaultMaxEntries);
    BOOST_REQUIRE_EQUAL(tree.getMinEntries(), rtree::DefaultMinEntries);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

BOOST_AUTO_TEST_CASE(insert_into_empty_tree)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    const rtree::BoundingBox box(0, 0, 10, 10);
    tree.insert(box, 0);
    auto nodeIt = tree.begin();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 1);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
    BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == box, "Root node bounding box is incorrect");
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
}

BOOST_AUTO_TEST_CASE(insert_into_nonempty_root)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    tree.insert({ 12, 34, 56, 78 }, 0);

    // Insert into a tree with a single node and single entry
    tree.insert({ 1, 2, 3, 4 }, 1);
    const rtree::BoundingBox root(1, 2, 67, 110);
    auto nodeIt = tree.begin();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 2);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
    BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == root, "Root node bounding box is incorrect");
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
}

BOOST_AUTO_TEST_CASE(insert_max_entries_into_root)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    for (size_t i = 0; i < tree.getMaxEntries(); i++) {
        const rtree::BoundingBox box(i*10.0, i*10.0, 10, 10);
        tree.insert(box, i);
    }
    const rtree::BoundingBox root(0, 0, 100, 100);
    auto nodeIt = tree.begin();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), tree.getMaxEntries());
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
    BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == root, "Root node bounding box is incorrect");
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
}

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

BOOST_AUTO_TEST_CASE(insert_duplicate_id)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    tree.insert({ 10, 10, 1, 1 }, 0);
    BOOST_CHECK_THROW(tree.insert({ 1, 10, 1, 1 }, 0), std::exception);
}

BOOST_AUTO_TEST_CASE(remove_from_empty_tree)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    tree.remove(0);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

BOOST_AUTO_TEST_CASE(remove_the_only_entry)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    tree.insert({ 10, 10, 1, 1 }, 0);
    tree.remove(0);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

BOOST_AUTO_TEST_CASE(remove_missing_entry)
{
    rtree::Tree<int, rtree::ExponentialSplit> tree;
    const auto box = rtree::BoundingBox(10, 10, 1, 1);
    tree.insert(box, 0);
    tree.remove(1);
    
    auto nodeIt = tree.begin();
    BOOST_CHECK(nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 1);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
    BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == box, "Root node bounding box is incorrect");
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
}

BOOST_AUTO_TEST_SUITE_END()
