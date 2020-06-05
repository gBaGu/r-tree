#include <rtree/rtree.hpp>


int main()
{
    rtree::Tree<int> tree;
    //tree.insert(rtree::BoundingBox{ .x=0, .y=0, .w=10, .h=10 }, 0);

    for (int i = 0; i < 100; i++) {
        const double x = std::rand() % 100;
        const double y = std::rand() % 100;
        const double w = std::rand() % 100;
        const double h = std::rand() % 100;
        tree.insert(rtree::BoundingBox{ .x=x, .y=y, .w=w, .h=h }, i);
    }
    tree.print();
    
    return 0;
}