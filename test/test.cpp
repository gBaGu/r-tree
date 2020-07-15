#define BOOST_TEST_MODULE rtree test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <rtree/rtree.hpp>


BOOST_AUTO_TEST_SUITE(Tree)

BOOST_AUTO_TEST_CASE(insert)
{
    int indexCounter = 0;
    rtree::Tree<int> tree;
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());

    rtree::BoundingBox rootbox;

    { // Insert to empty tree
        const rtree::BoundingBox box { .x=0, .y=0, .w=10, .h=10 };
        const int index = indexCounter++;
        tree.insert(box, index);
        rootbox = box;

        const auto& node = *tree.begin();
        BOOST_CHECK(node.isLeaf());
        BOOST_CHECK_EQUAL(node.size(), 1);
        BOOST_CHECK_EQUAL(node.depth(), 0);
        BOOST_CHECK_EQUAL(node.getParent(), nullptr);
        BOOST_CHECK_MESSAGE(node.getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        BOOST_CHECK_EQUAL(std::next(tree.begin()), tree.end()); // check that only one node is present
    }

    {
        const rtree::BoundingBox box { .x=0, .y=0, .w=30, .h=68 };
        const int index = indexCounter++;
        tree.insert(box, index);
        rootbox = rootbox & box;

        const auto& node = *tree.begin();
        BOOST_CHECK(node.isLeaf());
        BOOST_CHECK_EQUAL(node.size(), 2);
        BOOST_CHECK_EQUAL(node.depth(), 0);
        BOOST_CHECK_EQUAL(node.getParent(), nullptr);
        BOOST_CHECK_MESSAGE(node.getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        BOOST_CHECK_EQUAL(std::next(tree.begin()), tree.end()); // check that only one node is present
    }

    {
        const auto entriesToFullNode = tree.getMaxEntries() - 2;
        for (size_t i = 0; i < entriesToFullNode; i++) {
            const rtree::BoundingBox box { .x=i*10.0, .y=i*10.0, .w=10, .h=10 };
            const int index = indexCounter++;
            tree.insert(box, index);
            rootbox = rootbox & box;
        }

        const auto& node = *tree.begin();
        BOOST_CHECK(node.isLeaf());
        BOOST_CHECK_EQUAL(node.size(), tree.getMaxEntries()); // check that node is full
        BOOST_CHECK_EQUAL(node.depth(), 0);
        BOOST_CHECK_EQUAL(node.getParent(), nullptr);
        BOOST_CHECK_MESSAGE(node.getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        BOOST_CHECK_EQUAL(std::next(tree.begin()), tree.end()); // check that only one node is present
    }

    { // Insert to full root node
        const rtree::BoundingBox box { .x=3, .y=40, .w=71, .h=46 };
        const int index = indexCounter++;
        tree.insert(box, index);
        rootbox = rootbox & box;

        const auto& node = *tree.begin();
        BOOST_CHECK(not node.isLeaf());
        BOOST_CHECK_EQUAL(node.size(), 2);
        BOOST_CHECK_EQUAL(node.depth(), 0);
        BOOST_CHECK_EQUAL(node.getParent(), nullptr);
        BOOST_CHECK_MESSAGE(node.getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        // TODO: check child nodes
    }
}

BOOST_AUTO_TEST_SUITE_END()
