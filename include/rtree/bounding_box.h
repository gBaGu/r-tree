#pragma once
#include <algorithm>
#include <cmath>


namespace rtree
{
    struct Point
    {
        double x;
        double y;

        const Point operator+(const Point& other) const { return Point{ .x=x+other.x, .y=y+other.y }; }
        const Point operator-(const Point& other) const { return Point{ .x=x-other.x, .y=y-other.y }; }
        const Point operator*(double val) const { return Point{ .x=x*val, .y=y*val }; }
        const Point operator/(double val) const { return Point{ .x=x/val, .y=y/val }; }
    };

    using Segment = std::pair<Point, Point>;

    
    bool isDividing(Segment s, Point p1, Point p2)
    {
        //this is z coordinate of a result of vector multiplication
        //of vector {s.first, s.second} and {s.second, p1}
        double z1 = (s.second.x - s.first.x) * (p1.y - s.second.y) - (s.second.y - s.first.y) * (p1.x - s.second.x);
        //this is z coordinate of a result of vector multiplication
        //of vector {s.first, s.second} and {s.second, p2}
        double z2 = (s.second.x - s.first.x) * (p2.y - s.second.y) - (s.second.y - s.first.y) * (p2.x - s.second.x);

        return std::min(z1, z2) <= 0 && std::max(z1, z2) >= 0;
    }

    bool isIntersected(Segment s1, Segment s2)
    {
        return isDividing(s1, s2.first, s2.second) && isDividing(s2, s1.first, s1.second);
    }

    double length(Point v)
    {
        return std::pow(v.x * v.x + v.y * v.y, 0.5);
    }

    double scalarMultiplication(Point l, Point r)
    {
        return l.x * r.x + l.y * r.y;
    }

    double distance(Point p, Segment s)
    {
        const auto segmentVector = s.second - s.first;
        const auto vectorAtoPoint = p - s.first;
        const auto vectorBtoPoint = p - s.second;
        const auto scalarA = scalarMultiplication(segmentVector, vectorAtoPoint);
        const auto scalarB = scalarMultiplication(segmentVector, vectorBtoPoint);

        if (scalarA < 0 && scalarB < 0 ||
            scalarA > 0 && scalarB > 0) {
            if (std::abs(scalarA) < std::abs(scalarB)) {
                return length(vectorAtoPoint);
            }
            else {
                return length(vectorBtoPoint);
            }
        }
        else {
            auto unitVector = segmentVector / length(segmentVector);
            const auto deometricalMulZ = unitVector.x * vectorAtoPoint.y - unitVector.y * vectorAtoPoint.x;
            return std::pow(deometricalMulZ * deometricalMulZ, 0.5);
        }
    }


    struct BoundingBox
    {
        double x;
        double y;
        double w;
        double h;

        const Point bl() const { return Point{ .x=std::min(x, x+w), .y=std::min(y, y+h) }; }
        const Point tr() const { return Point{ .x=std::max(x, x+w), .y=std::max(y, y+h) }; }

        double area() const { return h * w; } //TODO: rework to be able to work with negative width and height
        double distance(const BoundingBox& other) const;
        bool intersects(const BoundingBox& other) const;
        bool overlaps(const BoundingBox& other) const;

        bool operator==(const BoundingBox& other) const;
        bool operator!=(const BoundingBox& other) const { return !(*this == other); }
        const BoundingBox operator&(const BoundingBox& other) const;
    };

    double BoundingBox::distance(const BoundingBox& other) const
    {
        if (this->intersects(other)) {
            return 0.0;
        }

        const Point r1Center = { x + w / 2, y + h / 2 };
        const Point r2Center = { other.x + other.w / 2, other.y + other.h / 2 };
        const std::vector<Segment> r1Sides = {
            { Point{ x, y },         Point{ x + w, y } },
            { Point{ x + w, y },     Point{ x + w, y + h } },
            { Point{ x + w, y + h }, Point{ x, y + h } },
            { Point{ x, y + h },     Point{ x, y } }
        };
        const std::vector<Segment> r2Sides = {
            { Point{ other.x, other.y },                     Point{ other.x + other.w, other.y } },
            { Point{ other.x + other.w, other.y },           Point{ other.x + other.w, other.y + other.h } },
            { Point{ other.x + other.w, other.y + other.h }, Point{ other.x, other.y + other.h } },
            { Point{ other.x, other.y + other.h },           Point{ other.x, other.y } }
        };

        const Segment connectedCenters = { r1Center, r2Center };
        const auto it1 = std::find_if(r1Sides.begin(), r1Sides.end(),
            [&](const auto& side) { return isIntersected(side, connectedCenters); });
        const auto it2 = std::find_if(r2Sides.begin(), r2Sides.end(),
            [&](const auto& side) { return isIntersected(side, connectedCenters); });
        if (it1 == r1Sides.end() || it2 == r2Sides.end()) {
            throw std::runtime_error("Impossible!");
        }

        return std::min({ rtree::distance(it1->first, *it2),
                          rtree::distance(it1->second, *it2),
                          rtree::distance(it2->first, *it1),
                          rtree::distance(it2->second, *it1) });
    }

    bool BoundingBox::intersects(const BoundingBox& other) const
    {
        const auto interLeft = std::max(bl().x, other.bl().x);
        const auto interRight = std::min(tr().x, other.tr().x);
        const auto interBottom = std::max(bl().y, other.bl().y);
        const auto interTop = std::min(tr().y, other.tr().y);
        return interLeft <= interRight && 
            interBottom <= interTop;
    }

    bool BoundingBox::overlaps(const BoundingBox& other) const
    {
        return (*this & other) == *this; 
    }

    inline bool BoundingBox::operator==(const BoundingBox& other) const
    {
        return x == other.x && y == other.y && w == other.w && h == other.h;
    }

    inline const BoundingBox BoundingBox::operator&(const BoundingBox& other) const
    {
        //TODO: rework to be able to work with negative width and height
        const auto minX = std::min(x, other.x);
        const auto minY = std::min(y, other.y);
        const auto maxX = std::max(x + w, other.x + other.w);
        const auto maxY = std::max(y + h, other.y + other.h);
        return BoundingBox{ .x=minX, .y=minY, .w=maxX-minX, .h=maxY-minY };
    }
} // namespace rtree