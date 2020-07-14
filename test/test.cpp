#define BOOST_TEST_MODULE rtree test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <rtree/rtree.hpp>


BOOST_AUTO_TEST_SUITE(Tree)

BOOST_AUTO_TEST_CASE(insert)
{
    rtree::Tree<int> tree;
    BOOST_CHECK_EQUAL(tree.begin(), tree.end());

    {
        const rtree::BoundingBox box { .x=0, .y=0, .w=10, .h=10 };
        const int index = 0;
        tree.insert(box, index);
        const auto& node = *tree.begin();
        BOOST_CHECK(node.isLeaf());
        BOOST_CHECK_EQUAL(node.size(), 1);
        BOOST_CHECK_EQUAL(node.depth(), 0);
        BOOST_CHECK_EQUAL(node.getParent(), nullptr);
        BOOST_CHECK_MESSAGE(node.getBoundingBox() == box, "Root node bounding box is incorrect");
    }
}

BOOST_AUTO_TEST_SUITE_END()