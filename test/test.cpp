#define BOOST_TEST_MODULE rtree test
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <rtree/rtree.hpp>


BOOST_AUTO_TEST_SUITE(Tree)

BOOST_AUTO_TEST_CASE(creation)
{
    rtree::Tree<int> tree;
    BOOST_REQUIRE_EQUAL(tree.getMaxEntries(), rtree::DefaultMaxEntries);
    BOOST_REQUIRE_EQUAL(tree.getMinEntries(), rtree::DefaultMinEntries);
}

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

    { // Insert into a tree with a single node and single entry
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

    { // Insert until root node is full
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

        // Check that root node is not a leaf now
        auto nodeIt = tree.begin();
        BOOST_CHECK(not nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->size(), 2);
        BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
        BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == rootbox, "Root node bounding box is incorrect");

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

    { // Insert two entries that are close to different nodes
        const rtree::BoundingBox box1 { .x=1, .y=1, .w=10, .h=10 };
        const rtree::BoundingBox box2 { .x=90, .y=90, .w=10, .h=10 };
        const int index1 = indexCounter++;
        tree.insert(box1, index1);
        rootbox = rootbox & box1;
        const int index2 = indexCounter++;
        tree.insert(box2, index2);
        rootbox = rootbox & box2;

        // Check that root node is unchanged
        auto nodeIt = tree.begin();
        BOOST_CHECK(not nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->size(), 2);
        BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
        BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == rootbox, "Root node bounding box is incorrect");

        // Check that the first node contains one of inserted entries
        nodeIt = std::next(nodeIt);
        const auto firstChildSize = nodeIt->size();
        BOOST_CHECK(nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->depth(), 1);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), tree.begin().get());
        BOOST_CHECK_MESSAGE(std::find_if(nodeIt->getEntries().begin(), nodeIt->getEntries().end(),
                                         [&box=box2](const auto& e) { return e.box == box; }) ==
                            nodeIt->getEntries().end(), "Inserted entry is missing in expected child node");

        // Check that the second node contains one of inserted entries
        nodeIt = std::next(nodeIt);
        const auto secondChildSize = nodeIt->size();
        BOOST_CHECK(nodeIt->isLeaf());
        BOOST_CHECK_EQUAL(nodeIt->depth(), 1);
        BOOST_CHECK_EQUAL(nodeIt->getParent(), tree.begin().get());
        BOOST_CHECK_MESSAGE(std::find_if(nodeIt->getEntries().begin(), nodeIt->getEntries().end(),
                                         [&box=box1](const auto& e) { return e.box == box; }) ==
                            nodeIt->getEntries().end(), "Inserted entry is missing in expected child node");

        // Check that overall size increased by 2
        BOOST_CHECK_EQUAL(firstChildSize + secondChildSize, tree.getMaxEntries() + 1 + 2);
        BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end());
    }

    { // Fill up a node that is not a root
        const auto intactNodeSize = std::next(std::next(tree.begin()))->size();
        while (std::next(tree.begin())->size() < tree.getMaxEntries()) {
            const rtree::BoundingBox box { .x=5, .y=5, .w=5, .h=5 };
            const int index = indexCounter++;
            tree.insert(box, index);
            rootbox = rootbox & box;
        }
        // One more to perform split
        const rtree::BoundingBox box { .x=2, .y=2, .w=2, .h=2 };
        const int index = indexCounter++;
        tree.insert(box, index);
        rootbox = rootbox & box;

        const auto intactNode = std::next(tree.begin());
        // Check that the second child node didn`t changed
        BOOST_CHECK(intactNode->isLeaf());
        BOOST_CHECK_EQUAL(intactNode->depth(), 1);
        BOOST_CHECK_EQUAL(intactNode->getParent(), tree.begin().get());
        BOOST_CHECK_EQUAL(intactNode->size(), intactNodeSize);

        // Check that new nodes are created at the same level as deleted one
        const auto newNode1 = std::next(intactNode);
        BOOST_CHECK(newNode1->isLeaf());
        BOOST_CHECK_EQUAL(newNode1->depth(), 1);
        BOOST_CHECK_EQUAL(newNode1->getParent(), tree.begin().get());

        const auto newNode2 = std::next(newNode1);
        BOOST_CHECK(newNode2->isLeaf());
        BOOST_CHECK_EQUAL(newNode2->depth(), 1);
        BOOST_CHECK_EQUAL(newNode2->getParent(), tree.begin().get());

        BOOST_CHECK_EQUAL(newNode1->size() + newNode2->size(), tree.getMaxEntries() + 1);
        BOOST_CHECK_EQUAL(std::next(newNode2), tree.end()); // check that there are not other nodes in the tree
    }
}

BOOST_AUTO_TEST_SUITE_END()
