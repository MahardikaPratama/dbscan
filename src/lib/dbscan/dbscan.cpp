#include "dbscan.h"
#include "../metrics/MetricRecorder.h"
#include "object/cluster/cluster.h"
#include "util/utils.h"
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <algorithm>
#include "../haversine/haversine.h"

DBSCANBase::DBSCANBase(double epsilon, int minPts)
{
    this->m_epsilon = epsilon;
    this->m_minPts = minPts;
}

void DBSCANBase::initialize(std::vector<Point> &all_points)
{
    // Reset all points
    for (auto &p : all_points)
    {
        p.setCluster(UNCLASSIFIED);
        p.setVisited(false);
    }
}

double DBSCANBase::run(std::vector<Point> &all_points, DBSCANResult *out, void *metrics_recorder)
{
    using dbscan::metrics::MetricRecorder;
    MetricRecorder *rec = nullptr;
    if (metrics_recorder)
        rec = reinterpret_cast<MetricRecorder *>(metrics_recorder);

    if (rec)
    {
        rec->start();
        rec->startPhase("initialize");
    }

    initialize(all_points);

    if (rec)
        rec->stopPhase("initialize");

    int clusterId = 0;
    for (size_t i = 0; i < all_points.size(); ++i)
    {
        if (all_points[i].visited())
        {
            continue;
        }
        all_points[i].setVisited(true);

        std::vector<int> neighbors = regionQuery(i, all_points);

        if (neighbors.size() < m_minPts)
        {
            all_points[i].setCluster(NOISE);
        }
        else
        {
            expandCluster(i, neighbors, clusterId, m_minPts, all_points);
            clusterId++;
        }
    }

    // Hitung jumlah noise dan jumlah cluster
    int num_noise = 0;
    std::set<int> cluster_ids;
    for (const auto &p : all_points)
    {
        if (p.getCluster() == NOISE)
            num_noise++;
        else if (p.getCluster() != UNCLASSIFIED)
            cluster_ids.insert(p.getCluster());
    }
    int num_clusters = (int)cluster_ids.size();

    double sse = 0.0;
    std::vector<double> dists;
    dists.reserve(all_points.size());

    // Hitung medoid (representative point) untuk setiap cluster
    std::map<int, Point> medoids;
    for (int cid : cluster_ids)
    {
        // Cari medoid: ambil point pertama di cluster sebagai contoh
        auto it = std::find_if(all_points.begin(), all_points.end(),
                               [cid](const Point &p)
                               { return p.getCluster() == cid; });
        if (it != all_points.end())
            medoids[cid] = *it;
    }
    for (const auto &p : all_points)
    {
        int cid = p.getCluster();
        if (cid == NOISE || cid == UNCLASSIFIED)
            continue;
        const Point &medoid = medoids[cid];
        double dist = calculate_distance(
            p.getLatitude(), p.getLongitude(),
            medoid.getLatitude(), medoid.getLongitude());
        dists.push_back(dist);
        sse += dist * dist;
    }

    if (rec)
    {
        rec->setNumPoints((int)all_points.size());
        rec->setNumNoise(num_noise);
        rec->setNumClusters(num_clusters);
        rec->setFinalSSE(sse);
        if (!dists.empty())
        {
            double sum = 0.0;
            double mx = dists[0];
            double mn = dists[0];
            for (double v : dists)
            {
                sum += v;
                if (v > mx)
                    mx = v;
                if (v < mn)
                    mn = v;
            }
            double mean = sum / dists.size();
            std::sort(dists.begin(), dists.end());
            double median = dists[dists.size() / 2];
            rec->setDistanceStats(mean, median, mx, mn);
        }
        rec->stop();
    }

    if (out)
    {
        out->sse = sse;
        out->points = all_points;
    }
    return sse;
}

std::vector<int> DBSCANBase::regionQuery(int pointIndex, std::vector<Point> &all_points)
{
    std::vector<int> neighbors;
    for (size_t i = 0; i < all_points.size(); ++i)
    {

        double dist = calculate_distance(
            all_points[pointIndex].getLatitude(), all_points[pointIndex].getLongitude(),
            all_points[i].getLatitude(), all_points[i].getLongitude());
        if (dist <= m_epsilon)
        {
            neighbors.push_back(i);
        }
    }
    return neighbors;
}

void DBSCANBase::expandCluster(int pointIndex, std::vector<int> &neighbors, int clusterId, int minPts, std::vector<Point> &all_points)
{
    all_points[pointIndex].setCluster(clusterId);

    for (size_t i = 0; i < neighbors.size(); ++i)
    {
        int neighborIndex = neighbors[i];
        Point &neighbor = all_points[neighborIndex];

        if (!neighbor.visited())
        {
            neighbor.setVisited(true);

            std::vector<int> neighborNeighbors = regionQuery(neighborIndex, all_points);

            if (neighborNeighbors.size() >= minPts)
            {
                neighbors.insert(neighbors.end(), neighborNeighbors.begin(), neighborNeighbors.end());
            }
        }
        if (neighbor.getCluster() == UNCLASSIFIED || neighbor.getCluster() == NOISE)
        {
            neighbor.setCluster(clusterId);
        }
    }
}