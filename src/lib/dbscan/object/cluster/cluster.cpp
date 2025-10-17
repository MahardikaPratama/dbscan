#include "cluster.h"
#include <string>

Cluster::Cluster(int id) : clusterId(id) {}

void Cluster::addPoint(Point *p)
{
    if (p)
        points.push_back(p);
}

bool Cluster::removePoint(const std::string &pointId)
{
    for (auto it = points.begin(); it != points.end(); ++it)
    {
        if ((*it)->getId() == pointId)
        {
            points.erase(it);
            return true;
        }
    }
    return false;
}

void Cluster::removeAllPoints() { points.clear(); }
int Cluster::getId() const { return clusterId; }
Point *Cluster::getPoint(int pos) const { return points[pos]; }
int Cluster::getSize() const { return static_cast<int>(points.size()); }
const std::vector<Point *> &Cluster::getPoints() const { return points; }