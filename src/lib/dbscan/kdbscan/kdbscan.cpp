
#include "kdbscan.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include "../../haversine/haversine.h"
#include "../../metrics/MetricRecorder.h"
#include <omp.h>
#include <set>
#include <map>

KDBSCAN::KDBSCAN(double epsilon, int minPts)
    : DBSCANBase(epsilon, minPts)
{
}

double KDBSCAN::run(std::vector<Point> &all_points, DBSCANResult *out, void *metrics_recorder)
{
    using dbscan::metrics::MetricRecorder;
    MetricRecorder *rec = nullptr;
    if (metrics_recorder)
        rec = reinterpret_cast<MetricRecorder *>(metrics_recorder);

    if (rec)
    {
        // Record input params and start recorder. Epsilon recorded after
        // initialization in case it is modified during the initialize step.
        rec->setMinSamples(this->m_minPts);
        rec->setNumPoints((int)all_points.size());
        rec->setThreads(omp_get_max_threads());

        rec->start();
        rec->startPhase("execution_time_ms");
    }

    // Step 1: Find core points
    findCorePoints(all_points);

    if (rec)
    {
        // KDBSCAN's epsilon is fixed at construction, but record it after
        // initialization for consistency with other algorithms.
        rec->setEpsilon(this->m_epsilon);
    }

    // Step 2: Collect core indexes
    std::vector<int> coreIndexes;
    for (size_t i = 0; i < all_points.size(); ++i)
    {
        if (all_points[i].isCorePoint())
            coreIndexes.push_back(i);
    }

    // Step 3: Assign each core point to a new cluster
    int clusterId = 1;
    for (int idx : coreIndexes)
    {
        if (all_points[idx].getCluster() == UNCLASSIFIED)
        {
            all_points[idx].setCluster(clusterId);
            std::vector<int> neighbors = regionQuery(idx, all_points, rec);
            for (int nb : neighbors)
            {
                if (all_points[nb].isCorePoint())
                {
                    all_points[nb].setCluster(clusterId);
                }
            }
            clusterId++;
        }
    }

    // Step 4: Merge overlapping core clusters
    mergeCoreClusters(all_points);

    // Step 5: Assign border points
    assignBorderPoints(all_points);

    // Calculate metrics
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

    // Medoid calculation (simple: first point in cluster)
    std::map<int, Point> medoids;
    for (int cid : cluster_ids)
    {
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
        rec->setNumNoisePoints(num_noise);
        rec->setNumClustersFound(num_clusters);
        rec->stopPhase("execution_time_ms");
        rec->stop();
    }

    if (out)
    {
        out->sse = sse;
        out->points = all_points;
    }
    return sse;
}

void KDBSCAN::findCorePoints(std::vector<Point> &points)
{
    for (size_t i = 0; i < points.size(); ++i)
    {
        std::vector<int> neighbors = this->regionQuery(i, points, nullptr);
        if (neighbors.size() >= static_cast<size_t>(this->m_minPts))
        {
            points[i].setCorePoint(true);
        }
    }
}

void KDBSCAN::mergeCoreClusters(std::vector<Point> &points)
{
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (!points[i].isCorePoint())
            continue;

        for (size_t j = i + 1; j < points.size(); ++j)
        {
            if (!points[j].isCorePoint())
                continue;

            if (calculate_distance(points[i].getLatitude(), points[i].getLongitude(), points[j].getLatitude(), points[j].getLongitude()) <= this->m_epsilon)
            {
                int cid_i = points[i].getCluster();
                int cid_j = points[j].getCluster();

                if (cid_i != cid_j && cid_i != UNCLASSIFIED && cid_j != UNCLASSIFIED)
                {
                    // Gabungkan dua cluster yang overlap
                    for (auto &p : points)
                    {
                        if (p.getCluster() == cid_j)
                            p.setCluster(cid_i);
                    }
                }
            }
        }
    }
}

void KDBSCAN::assignBorderPoints(std::vector<Point> &points)
{
    for (auto &p : points)
    {
        if (p.isCorePoint() || p.getCluster() != UNCLASSIFIED)
            continue;

        std::vector<int> neighbors = this->regionQuery(&p - &points[0], points, nullptr);
        for (int idx : neighbors)
        {
            if (points[idx].isCorePoint())
            {
                p.setCluster(points[idx].getCluster());
                break;
            }
        }
        if (p.getCluster() == UNCLASSIFIED)
            p.setCluster(NOISE);
    }
}