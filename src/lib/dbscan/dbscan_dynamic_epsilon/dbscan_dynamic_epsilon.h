#pragma once
#include "../dbscan.h"
#include <vector>

class DBSCANDynamicEpsilon : public DBSCANBase
{
public:
    DBSCANDynamicEpsilon(int minPts);
    void initialize(std::vector<Point> &all_points) override;
    static double generateKDistanceEpsilon(const std::vector<Point> &points, int minPts);
};
