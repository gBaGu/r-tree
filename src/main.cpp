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

    const auto intersected = tree.findIntersected(rtree::BoundingBox{ .x=9, .y=9, .w=2, .h=2 });
    std::cout << "Found intersected: " << std::endl;
    for (const auto& entry: intersected) {
        const auto box = entry.box;
        std::cout << "(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
    }
    std::cout << std::endl;
    
    return 0;
}