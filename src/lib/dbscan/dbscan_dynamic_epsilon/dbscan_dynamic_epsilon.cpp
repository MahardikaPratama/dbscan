#include "dbscan_dynamic_epsilon.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include "../../haversine/haversine.h"

DBSCANDynamicEpsilon::DBSCANDynamicEpsilon(int minPts)
    : DBSCANBase(0.0, minPts) // epsilon akan di-set di initialize
{
}

void DBSCANDynamicEpsilon::initialize(std::vector<Point> &all_points)
{
    DBSCANBase::initialize(all_points);
    double epsilon = generateKDistanceEpsilon(all_points, this->m_minPts);
    if (epsilon > 0.0)
    {
        this->m_epsilon = epsilon;
    }
}

double DBSCANDynamicEpsilon::generateKDistanceEpsilon(
    const std::vector<Point> &points,
    int minPts)
{
    // input validation
    if (points.empty() || minPts <= 0)
    {
        std::cerr << "Invalid input for k-distance generation.\n";
        return 0.0;
    }

    std::vector<double> kDistances;
    kDistances.reserve(points.size());

    // calculate k-distance for each point
    for (size_t i = 0; i < points.size(); ++i)
    {
        std::vector<double> distances;
        distances.reserve(points.size() - 1);

        for (size_t j = 0; j < points.size(); ++j)
        {
            if (i == j)
                continue;

            // calculate haversine distance in kilometers
            double d = calculate_distance(points[i].getLatitude(), points[i].getLongitude(), points[j].getLatitude(), points[j].getLongitude());
            distances.push_back(d);
        }

        std::sort(distances.begin(), distances.end());

        if (distances.size() >= static_cast<size_t>(minPts))
        {
            int kIndex = std::max(0, minPts - 1);
            kDistances.push_back(distances[kIndex]);
        }
    }

    if (kDistances.empty())
    {
        std::cerr << "No valid k-distances found.\n";
        return 0.0;
    }

    // sort all k-distances (ascending)
    std::sort(kDistances.begin(), kDistances.end());

    // If too few points, fallback to median
    if (kDistances.size() < 3)
    {
        double epsilon;
        size_t mid = kDistances.size() / 2;
        if (kDistances.size() % 2 == 0)
            epsilon = (kDistances[mid - 1] + kDistances[mid]) / 2.0;
        else
            epsilon = kDistances[mid];
        return epsilon; // in kilometers
    }

    // Elbow detection (knee) via maximum perpendicular distance from
    // the line connecting first and last point in the k-distance plot.
    // x = index (0..n-1), y = kDistances[index]
    const size_t n = kDistances.size();
    const double x1 = 0.0;
    const double y1 = kDistances.front();
    const double x2 = static_cast<double>(n - 1);
    const double y2 = kDistances.back();

    // Precompute components for line distance formula
    const double dx = x2 - x1; // = n-1
    const double dy = y2 - y1;
    const double denom = std::sqrt(dx * dx + dy * dy);

    // If denom is zero (all y equal), fallback to median
    if (denom <= 0.0)
    {
        double epsilon = kDistances[n / 2];
        return epsilon;
    }

    double maxDist = -1.0;
    size_t maxIdx = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double xi = static_cast<double>(i);
        const double yi = kDistances[i];
        // Line-point distance (absolute cross product / length)
        // |dy*xi - dx*yi + x2*y1 - y2*x1| / denom
        const double num = std::abs(dy * xi - dx * yi + x2 * y1 - y2 * x1);
        const double dist = num / denom;
        if (dist > maxDist)
        {
            maxDist = dist;
            maxIdx = i;
        }
    }

    // If the maximum distance is negligibly small, fallback to median
    if (maxDist <= 0.0)
    {
        double epsilon = kDistances[n / 2];
        return epsilon;
    }

    // Use the k-distance at the elbow index as epsilon
    double epsilon = kDistances[maxIdx];
    return epsilon; // in kilometers
}