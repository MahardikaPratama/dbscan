#pragma once
#include "object/point/point.h"
#include "object/cluster/cluster.h"
#include <vector>
#include <string>

struct DBSCANResult
{
    double sse = 0.0;
    std::vector<Point> points;
    std::string metrics_file;
};

class DBSCANBase
{
private:
    double m_epsilon;
    int m_minPts;
    std::vector<Cluster> clusters;

protected:
    virtual std::vector<int> regionQuery(int pointIndex, std::vector<Point> &all_points);
    virtual void expandCluster(int pointIndex, std::vector<int> &neighbors, int clusterId, int minPts, std::vector<Point> &all_points);

public:
    DBSCANBase(double epsilon, int minPts);
    virtual ~DBSCANBase() = default;

    virtual void initialize(std::vector<Point> &all_points);

    virtual double run(std::vector<Point> &all_points, DBSCANResult *out = nullptr, void *metrics_recorder = nullptr);
};
