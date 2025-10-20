#pragma once
#include "../dbscan.h"
#include <vector>

class KDBSCAN : public DBSCANBase
{
public:
    KDBSCAN(double epsilon, int minPts);

    double run(std::vector<Point> &all_points, DBSCANResult *out = nullptr, void *metrics_recorder = nullptr) override;

    void findCorePoints(std::vector<Point> &points);
    void mergeCoreClusters(std::vector<Point> &points);
    void assignBorderPoints(std::vector<Point> &points);
};
