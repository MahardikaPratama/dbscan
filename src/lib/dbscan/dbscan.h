#pragma once
#include "object/point/point.h"
#include "object/cluster/cluster.h"
#include <vector>
#include <string>

// Forward declaration for metrics recorder to avoid requiring the header
// in this public header and to ensure the pointer type is known to the
// compiler when used in method signatures below.
namespace dbscan
{
    namespace metrics
    {
        class MetricRecorder;
    }
}

struct DBSCANResult
{
    double sse = 0.0;
    std::vector<Point> points;
    std::string metrics_file;
};

class DBSCANBase
{
protected:
    double m_epsilon;
    int m_minPts;
    std::vector<Cluster> clusters;
    virtual std::vector<int> regionQuery(int pointIndex, std::vector<Point> &all_points, dbscan::metrics::MetricRecorder *rec);
    virtual void expandCluster(int pointIndex, std::vector<int> &neighbors, int clusterId, int minPts, std::vector<Point> &all_points, dbscan::metrics::MetricRecorder *rec);

public:
    DBSCANBase(double epsilon, int minPts);
    virtual ~DBSCANBase() = default;

    virtual void initialize(std::vector<Point> &all_points);

    virtual double run(std::vector<Point> &all_points, DBSCANResult *out = nullptr, void *metrics_recorder = nullptr);
};
