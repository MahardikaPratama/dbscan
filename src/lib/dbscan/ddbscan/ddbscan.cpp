#include "ddbscan.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include "../../haversine/haversine.h"

DDBSCAN::DDBSCAN(int minPts)
    : DBSCANBase(0.0, minPts)
{
}

void DDBSCAN::initialize(std::vector<Point> &all_points)
{
    DBSCANBase::initialize(all_points);
    double epsScale = 1.0;
    int percentile = 50;

    // compute per-point epsilon using k-distance (returns kilometers)
    std::vector<double> per_point_eps = DDBSCAN::computeKDistance(all_points, this->m_minPts);

    // compute percentile value of k-distance vector to use as a floor/smoothing
    auto compute_percentile = [](std::vector<double> v, int p)
    {
        if (v.empty())
            return 0.0;
        std::sort(v.begin(), v.end());
        double idx = (p / 100.0) * (v.size() - 1);
        size_t lo = static_cast<size_t>(std::floor(idx));
        size_t hi = static_cast<size_t>(std::ceil(idx));
        if (hi >= v.size())
            hi = v.size() - 1;
        if (lo == hi)
            return v[lo];
        double frac = idx - lo;
        return v[lo] * (1.0 - frac) + v[hi] * frac;
    };

    double kdist_percentile = compute_percentile(per_point_eps, percentile);

    for (size_t i = 0; i < all_points.size() && i < per_point_eps.size(); ++i)
    {
        // assign smoothed epsilon: at least the chosen percentile scaled
        all_points[i].setEpsilon(std::max(per_point_eps[i], kdist_percentile) * epsScale);
    }

    std::sort(all_points.begin(), all_points.end(), [](const Point &a, const Point &b)
              { return a.getEpsilon() < b.getEpsilon(); });

    // Set a representative global epsilon so metrics (which expect a single
    // epsilon value) record a sensible number. Use the percentile floor.
    if (kdist_percentile > 0.0)
    {
        this->m_epsilon = kdist_percentile * epsScale;
    }
}

std::vector<double> DDBSCAN::computeKDistance(const std::vector<Point> &points, int k)
{
    const size_t n = points.size();
    std::vector<double> kdist(n, 0.0);

    if (n == 0 || k <= 0)
        return kdist;

    for (size_t i = 0; i < n; ++i)
    {
        std::vector<double> dists;
        dists.reserve(n > 0 ? n - 1 : 0);
        for (size_t j = 0; j < n; ++j)
        {
            if (i == j)
                continue;
            double d = calculate_distance(points[i].getLatitude(), points[i].getLongitude(), points[j].getLatitude(), points[j].getLongitude());
            dists.push_back(d);
        }
        if (dists.empty())
        {
            kdist[i] = 0.0;
            continue;
        }
        std::sort(dists.begin(), dists.end());
        if ((size_t)k <= dists.size())
            kdist[i] = dists[k - 1];
        else
            kdist[i] = dists.back();
    }

    return kdist;
}