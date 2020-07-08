#include <iostream>

#include <rtree/rtree.hpp>


int main()
{
    rtree::Tree<int> tree;

    for (int i = 0; i < 100; i++) {
        const double x = std::rand() % 100;
        const double y = std::rand() % 100;
        const double w = std::rand() % 100;
        const double h = std::rand() % 100;
        std::cout << i << ": " << x << ":" << y << ":" << w << ":" << h << std::endl;
        tree.insert(rtree::BoundingBox{ .x=x, .y=y, .w=w, .h=h }, i);
    }

    std::cout << "Printing R-tree:\n";
    std::for_each(tree.begin(), tree.end(), [](const auto& node) {
        const auto box = node.getBoundingBox();
        std::cout << std::string(node.depth() * 2, ' ')
            << "(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
        if (node.isLeaf()) {
            for (const auto& entry: node.getEntries()) {
                const auto box = entry.box;
                std::cout << std::string(node.depth() * 2, ' ')
                    << ".(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
            }
        }
    });

    auto intersected = tree.find(rtree::BoundingBox{ .x=9, .y=9, .w=2, .h=2 });
    std::cout << "Found intersected: " << std::endl;
    for (const auto& entry: intersected) {
        const auto box = entry.box;
        std::cout << "(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
    }
    std::cout << std::endl;

    tree.remove(80);

    intersected = tree.find(rtree::BoundingBox{ .x=9, .y=9, .w=2, .h=2 });
    std::cout << "Found intersected: " << std::endl;
    for (const auto& entry: intersected) {
        const auto box = entry.box;
        std::cout << "(" << box.x << ", " << box.y << ", " << box.w << ", " << box.h << ")\n";
    }
    std::cout << std::endl;
    
    return 0;
}