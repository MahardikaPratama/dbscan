#pragma once
#include <vector>
#include "../point/point.h"

class Cluster
{
private:
    int clusterId;
    std::vector<Point *> points;

public:
    explicit Cluster(int id);
    void addPoint(Point *p);
    bool removePoint(const std::string &pointId);
    void removeAllPoints();
    int getId() const;
    Point *getPoint(int pos) const;
    int getSize() const;
    const std::vector<Point *> &getPoints() const;
};
