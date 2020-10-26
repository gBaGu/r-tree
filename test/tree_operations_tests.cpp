#include <boost/test/unit_test.hpp>

#include <rtree/rtree.hpp>


BOOST_AUTO_TEST_SUITE(tree_operations)

BOOST_AUTO_TEST_CASE(create_with_defaults)
{
    rtree::Tree<int> tree;
    BOOST_REQUIRE_EQUAL(tree.getMaxEntries(), rtree::DefaultMaxEntries);
    BOOST_REQUIRE_EQUAL(tree.getMinEntries(), rtree::DefaultMinEntries);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

BOOST_AUTO_TEST_CASE(create_with_custom_settings)
{
    rtree::Tree<int> tree(20, 100);
    BOOST_REQUIRE_EQUAL(tree.getMaxEntries(), 100);
    BOOST_REQUIRE_EQUAL(tree.getMinEntries(), 20);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

/**
 * Insert tests:
 *  Possible boxes:
 *   empty
 *   0, 0, 0, 0
 *   small box: 0, 0, 0.0001, 0.0001
 *   negative box: -1, -10, 0.5, 0.5
 *   negative box that has positive right top: -5, -5, 10, 10
 *   huge box: 10000000, 10000000, 1000000, 1000000
 *   rectangle: 5, 8, 120, 1
 *   random box: -1.05, -0.41, 0.084, 0.54
 * 
 *  Possible indices:
 *   0, 1, 9999999, -1, -9999999, -459560
 * 
 *  Possible tree conditions before insert:
 *   empty tree
 *   single entry
 *   full single node
 *   two far nodes -> insert to right one
 *   three far nodes -> insert to right one
 *   100 entries
 *   1000 entries
 * 
 *  Possible tree conditions after insert:
 *   split
 *   no split
 * 
 *  Remove:
 *   condense
 *   no condense
 * 
 *  Other cases:
 *   Root is covering 4 far boxes
 */

BOOST_AUTO_TEST_CASE(insert_empty_box)
{
    rtree::Tree<int> tree;
    const rtree::BoundingBox box;
    BOOST_REQUIRE_THROW(tree.insert(box, 0), rtree::EmptyBoundingBoxException);
}

BOOST_AUTO_TEST_CASE(insert_duplicate_id)
{
    rtree::Tree<int> tree;
    tree.insert({ 10, 10, 1, 1 }, 0);
    BOOST_CHECK_THROW(tree.insert({ 1, 10, 1, 1 }, 0), rtree::DuplicateEntryException);
}

BOOST_AUTO_TEST_CASE(insert_into_empty_tree)
{
    rtree::Tree<int> tree;
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
    rtree::Tree<int> tree;
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
    rtree::Tree<int> tree;
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

BOOST_AUTO_TEST_CASE(insert)
{
    int indexCounter = 0;
    rtree::Tree<int> tree;
    std::vector<rtree::BoundingBox> boxes {
        { 0, 0, 10, 10 },
        { 0, 0, 30, 68 },
        { 0, 0, 10, 10 },
        { 10, 10, 10, 10 },
        { 20, 20, 10, 10 },
        { 30, 30, 10, 10 },
        { 40, 40, 10, 10 },
        { 50, 50, 10, 10 },
        { 60, 60, 10, 10 },
        { 70, 70, 10, 10 },
        { 3, 40, 71, 46 }
    };
    for (const auto& box: boxes) {
        const int index = indexCounter++;
        tree.insert(box, index);
    }
    rtree::BoundingBox rootbox(0, 0, 80, 86);

    {
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
        const rtree::BoundingBox box1(1, 1, 10, 10);
        const rtree::BoundingBox box2(90, 90, 10, 10);
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
            const rtree::BoundingBox box(5, 5, 5, 5);
            const int index = indexCounter++;
            tree.insert(box, index);
            rootbox = rootbox & box;
        }
        // One more to perform split
        const rtree::BoundingBox box(2, 2, 2, 2);
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

BOOST_AUTO_TEST_CASE(insert_into_root_and_split)
{
    rtree::Tree<int> tree(2, 4);
    for (size_t i = 0; i < tree.getMaxEntries() + 1; i++) {
        const rtree::BoundingBox box(i*0.1, i*0.1, 0.1, 0.1);
        //root = root & box;
        tree.insert(box, i);
    }

    const rtree::BoundingBox root(0, 0, 0.5, 0.5);
    // Check that root node is not a leaf now
    auto nodeIt = tree.begin();
    BOOST_CHECK(not nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 2);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);
    BOOST_CHECK_MESSAGE(nodeIt->getBoundingBox() == root, "Root node bounding box is incorrect");

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

BOOST_AUTO_TEST_CASE(find_works_when_root_has_precision_loss)
{
    rtree::Tree<int> tree(2, 4);
    for (size_t i = 0; i < tree.getMaxEntries() + 1; i++) {
        const rtree::BoundingBox box(i*0.1, i*0.1, 0.2, 0.2);
        //root = root & box;
        tree.insert(box, i);
    }

    // Check that root node is not a leaf now
    auto nodeIt = tree.begin();
    BOOST_CHECK(not nodeIt->isLeaf());
    BOOST_CHECK_EQUAL(nodeIt->size(), 2);
    BOOST_CHECK_EQUAL(nodeIt->depth(), 0);
    BOOST_CHECK_EQUAL(nodeIt->getParent(), nullptr);

    const auto intersected = tree.find(rtree::BoundingBox(0.6, 0.6, 0.2, 0.2));
    const auto entry = rtree::Entry<int>{ .box=rtree::BoundingBox(0.4, 0.4, 0.2, 0.2), .data=4 };
    BOOST_REQUIRE_EQUAL(intersected.size(), 1);
    BOOST_REQUIRE(intersected[0] == entry);
}

BOOST_AUTO_TEST_CASE(remove_from_empty_tree)
{
    rtree::Tree<int> tree;
    tree.remove(0);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

BOOST_AUTO_TEST_CASE(remove_the_only_entry)
{
    rtree::Tree<int> tree;
    tree.insert({ 10, 10, 1, 1 }, 0);
    tree.remove(0);
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());
}

BOOST_AUTO_TEST_CASE(remove_missing_entry)
{
    rtree::Tree<int> tree;
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

BOOST_AUTO_TEST_CASE(remove_with_condense)
{
    int indexCounter = 0;
    rtree::Tree<int> tree;

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

    // Remove from node with minimum number of entries to trigger compaction
    tree.remove(0);

    const auto nodeIt = tree.begin();
    BOOST_CHECK_EQUAL(nodeIt->size(), tree.getMaxEntries());
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end());
}

BOOST_AUTO_TEST_CASE(remove_without_condense)
{
    int indexCounter = 0;
    rtree::Tree<int> tree;

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

    tree.remove(indexCounter - 1);

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

    BOOST_CHECK_EQUAL(firstChildSize + secondChildSize, tree.getMaxEntries());
    BOOST_CHECK_EQUAL(std::next(nodeIt), tree.end());
}

BOOST_AUTO_TEST_SUITE_END()
