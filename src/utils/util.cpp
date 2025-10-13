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
 * Description        : Implementation file for utility functions.
 *
 * Changelog:
 * - 0.1.0 (16 September 2025): Initial creation of utility implementation file.
 */

#include "utils/util.h"
#include "utils/haversine.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

std::vector<Point> Utils::readCSV(const std::string &filename)
{
    std::vector<Point> points;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return points;
    }

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        Point p;
        std::string id, lat, lon, sensor;

        std::getline(ss, id, ',');
        std::getline(ss, lat, ',');
        std::getline(ss, lon, ',');
        std::getline(ss, sensor, ',');
        // Basic validation: skip empty lines or header row
        if (id.empty() || lat.empty() || lon.empty() || sensor.empty())
        {
            // Could be a header row or malformed line - skip it
            continue;
        }

        p.object_id = id;
        try
        {
            p.lat = std::stod(lat);
            p.lon = std::stod(lon);
            p.sensor = std::stoi(sensor);
        }
        catch (const std::invalid_argument &)
        {
            // Non-numeric value (likely header) - skip this row
            std::cerr << "Skipping non-numeric CSV row: " << line << std::endl;
            continue;
        }
        catch (const std::out_of_range &)
        {
            // Value out of range for double - skip this row
            std::cerr << "Skipping out-of-range CSV row: " << line << std::endl;
            continue;
        }
        p.clusterId = UNCLASSIFIED;
        p.isVisited = false;

        points.push_back(p);
    }

    file.close();
    return points;
}

void Utils::writeCSV(const std::string &filename, const std::vector<Point> &points)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // Write header row so downstream tools can read the columns
    file << "ObjectID,Latitude,Longitude,Sensor,ClusterId\n";

    for (const auto &p : points)
    {
        file << p.object_id << ","
             << p.lat << ","
             << p.lon << ","
             << p.sensor << ","
             << p.clusterId << "\n";
    }

    file.close();
}

double Utils::haversineDistance(const Point &p1, const Point &p2)
{
    // Use the shared haversine implementation (calculate_distance) which returns kilometers
    const double horiz_km = calculate_distance(p1.lat, p1.lon, p2.lat, p2.lon);
    return horiz_km;
}

double Utils::generateKDistanceEpsilon(const std::vector<Point> &points, int minPts)
{
    if (points.empty() || minPts <= 0)
    {
        std::cerr << "Invalid input for k-distance generation.\n";
        return 0.0;
    }

    std::vector<double> kDistances;
    kDistances.reserve(points.size());

    int kIndex = std::max(0, minPts - 1); // because distances excludes self

    for (size_t i = 0; i < points.size(); ++i)
    {
        std::vector<double> distances;
        distances.reserve(points.size() - 1);

        for (size_t j = 0; j < points.size(); ++j)
        {
            if (i == j)
                continue;
            distances.push_back(haversineDistance(points[i], points[j]));
        }

        if (distances.size() < static_cast<size_t>(kIndex + 1))
            continue;

        // faster: nth_element
        std::nth_element(distances.begin(), distances.begin() + kIndex, distances.end());
        double kth = distances[kIndex];
        kDistances.push_back(kth);
    }

    if (kDistances.empty())
        return 0.0;

    std::sort(kDistances.begin(), kDistances.end());

    // choose percentile or median. Example: 90th percentile
    size_t idx = static_cast<size_t>(kDistances.size() * 0.9);
    if (idx >= kDistances.size())
        idx = kDistances.size() - 1;
    double epsilon = kDistances[idx];
    return epsilon;
}