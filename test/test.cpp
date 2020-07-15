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

        auto nodeIt = tree.begin();
        BOOST_CHECK(nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->size(), 1);
        BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
        BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
    }

    {
        const rtree::BoundingBox box { .x=0, .y=0, .w=30, .h=68 };
        const int index = indexCounter++;
        tree.insert(box, index);
        rootbox = rootbox & box;

        auto nodeIt = tree.begin();
        BOOST_CHECK(nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->size(), 2);
        BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
        BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
    }

    {
        const auto entriesToFullNode = tree.getMaxEntries() - 2;
        for (size_t i = 0; i < entriesToFullNode; i++) {
            const rtree::BoundingBox box { .x=i*10.0, .y=i*10.0, .w=10, .h=10 };
            const int index = indexCounter++;
            tree.insert(box, index);
            rootbox = rootbox & box;
        }

        auto nodeIt = tree.begin();
        BOOST_CHECK(nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->size(), tree.getMaxEntries()); // check that node is full
        BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
        BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == rootbox, "Root node bounding box is incorrect");
        BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end()); // check that only one node is present
    }

    { // Insert to full root node
        const rtree::BoundingBox box { .x=3, .y=40, .w=71, .h=46 };
        const int index = indexCounter++;
        tree.insert(box, index);
        rootbox = rootbox & box;

        auto nodeIt = tree.begin();
        BOOST_CHECK(not nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->size(), 2);
        BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
        BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == rootbox, "Root node bounding box is incorrect");

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

        BOOST_CHECK_EQUAL(firstChildSize + secondChildSize, tree.getMaxEntries() + 1);
        BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end());
    }

    // TODO: insert two more enties to different nodes
    //       and check that node to insert to is choosen correctly
}

BOOST_AUTO_TEST_SUITE_END()
