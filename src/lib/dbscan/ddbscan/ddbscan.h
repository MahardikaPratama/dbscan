#pragma once
#include "../dbscan.h"
#include <vector>

class DDBSCAN : public DBSCANBase
{
public:
    DDBSCAN(int minPts);
    void initialize(std::vector<Point> &all_points) override;
    static std::vector<double> computeKDistance(const std::vector<Point> &points, int k);
};
