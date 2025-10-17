/**
 * Copyright PT Len Innovation Technology
 * THIS SOFTWARE SOURCE CODE AND ANY EXECUTABLE DERIVED THEREOF ARE PROPRIETARY
 * TO PT LEN INNOVATION TECHNOLOGY, AS APPLICABLE, AND SHALL NOT BE USED IN ANY WAY
 * OTHER THAN BEFOREHAND AGREED ON BY PT LEN INNOVATION TECHNOLOGY, NOR BE REPRODUCED
 * OR DISCLOSED TO THIRD PARTIES WITHOUT PRIOR WRITTEN AUTHORIZATION BY
 * PT LEN INNOVATION TECHNOLOGY, AS APPLICABLE.
 *
 * Author             : Mahardika Pratama
 * Version            : 0.1.0
 * Created Date       : 16 September 2025
 * Description        : Main entry point for DBSCAN clustering application.
 *
 * Changelog:
 * - 0.1.0 (16 September 2025): Initial creation of main entry point.
 */

#include "algorithm/dbscan.h"
#include "utils/util.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <cmath>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_csv_path>\n";
        return 1;
    }
    std::string filename = argv[1];
    std::vector<Point> points = Utils::readCSV(filename);

    if (points.empty())
    {
        std::cerr << "Dataset is empty or failed to load.\n";
        return 1;
    }

    int minPts = 1;
    if (argc >= 3)
    {
        try
        {
            minPts = std::stoi(argv[2]);
            if (minPts <= 0)
                minPts = 1;
        }
        catch (...)
        {
            minPts = 1;
        }
    }

    // optional third argument: epsilon scale (double, default 1.0)
    double epsScale = 1.0;
    if (argc >= 4)
    {
        try
        {
            epsScale = std::stod(argv[3]);
            if (epsScale <= 0.0)
                epsScale = 1.0;
        }
        catch (...)
        {
            epsScale = 1.0;
        }
    }

    // optional fourth argument: percentile smoothing (integer 0-100, default 50)
    int percentile = 50;
    if (argc >= 5)
    {
        try
        {
            percentile = std::stoi(argv[4]);
            if (percentile < 0)
                percentile = 0;
            if (percentile > 100)
                percentile = 100;
        }
        catch (...)
        {
            percentile = 50;
        }
    }

    // compute per-point epsilon using k-distance (returns kilometers)
    std::vector<double> per_point_eps = Utils::computeKDistance(points, minPts);

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

    for (size_t i = 0; i < points.size() && i < per_point_eps.size(); ++i)
    {
        // assign smoothed epsilon: at least the chosen percentile scaled
        points[i].epsilon = std::max(per_point_eps[i], kdist_percentile) * epsScale;
    }

    // sort points by epsilon ascending (smallest epsilon first)
    std::sort(points.begin(), points.end(), [](const Point &a, const Point &b)
              { return a.epsilon < b.epsilon; });

    // run DBSCAN with per-point epsilons assigned
    DBSCAN dbscan(minPts, points);
    dbscan.run();

    namespace fs = std::filesystem;
    fs::path inputPath(filename);
    std::string stem = inputPath.stem().string();
    fs::path outDir = fs::path("output");
    if (!fs::exists(outDir))
    {
        std::error_code ec;
        fs::create_directories(outDir, ec);
        if (ec)
        {
            std::cerr << "Failed to create output directory: " << outDir << " (" << ec.message() << ")\n";
            return 1;
        }
    }
    fs::path outPath = outDir / ("Result_" + stem + ".csv");

    // Compute per-object majority-cluster accuracy
    // Map object_id -> map(clusterId -> count)
    std::unordered_map<std::string, std::unordered_map<int, int>> objClusterCounts;
    std::unordered_map<std::string, int> objTotalCounts;
    for (const auto &p : points)
    {
        objClusterCounts[p.object_id][p.clusterId]++;
        objTotalCounts[p.object_id]++;
    }

    // For each object, find majority cluster and compute accuracy = majority_count / total_count
    std::unordered_map<std::string, double> objAccuracy;
    for (const auto &kv : objClusterCounts)
    {
        const std::string &obj = kv.first;
        const auto &clusterMap = kv.second;
        int majorityCount = 0;
        for (const auto &ckv : clusterMap)
        {
            majorityCount = std::max(majorityCount, ckv.second);
        }
        int total = objTotalCounts[obj];
        double acc = total > 0 ? (double)majorityCount / (double)total : 0.0;
        objAccuracy[obj] = acc;
    }

    // Build per-point accuracies using object's accuracy, then compute mean over objects
    std::vector<double> accuracies;
    accuracies.reserve(points.size());
    for (const auto &p : points)
    {
        auto it = objAccuracy.find(p.object_id);
        double acc = (it != objAccuracy.end()) ? it->second : 0.0;
        accuracies.push_back(acc);
    }

    // Compute total accuracy averaged per object (not per point)
    double total_accuracy = 0.0;
    if (!objAccuracy.empty())
    {
        double sum = 0.0;
        for (const auto &kv : objAccuracy)
            sum += kv.second;
        total_accuracy = sum / objAccuracy.size();
    }

    std::cout << "DBSCAN completed. Results written to " << outPath << "\n";
    Utils::writeCSV(outPath.string(), points, accuracies, total_accuracy);
    return 0;
}
