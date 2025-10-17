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
 * Description        : Implementation file for DBSCAN clustering algorithm.
 *
 * Changelog:
 * - 0.1.0 (16 September 2025): Initial creation of DBSCAN implementation file.
 */

#include "dbscan.h"
#include "utils/util.h"
#include <cmath>
#include <vector>

const double EARTH_RADIUS = 6371000.0;

DBSCAN::DBSCAN(int minPts, std::vector<Point> &points)
    : m_points(points), m_minPts(minPts) {}

void DBSCAN::run()
{
    int clusterId = 0;
    for (size_t i = 0; i < m_points.size(); ++i)
    {
        if (m_points[i].isVisited)
        {
            continue;
        }
        m_points[i].isVisited = true;

        std::vector<int> neighbors = regionQuery(i);

        if (neighbors.size() < m_minPts)
        {
            m_points[i].clusterId = NOISE;
        }
        else
        {
            expandCluster(i, neighbors, clusterId, m_minPts);
            clusterId++;
        }
    }
}

std::vector<int> DBSCAN::regionQuery(int pointIndex)
{
    std::vector<int> neighbors;
    for (size_t i = 0; i < m_points.size(); ++i)
    {
        // symmetric epsilon: allow neighbor if distance <= max(eps_query, eps_candidate)
        const double eps_query = m_points[pointIndex].epsilon;
        const double eps_candidate = m_points[i].epsilon;
        const double eps_threshold = std::max(eps_query, eps_candidate);
        if (Utils::haversineDistance(m_points[pointIndex], m_points[i]) <= eps_threshold)
        {
            neighbors.push_back(i);
        }
    }
    return neighbors;
}

void DBSCAN::expandCluster(int pointIndex, std::vector<int> &neighbors, int clusterId, int minPts)
{

    m_points[pointIndex].clusterId = clusterId;

    for (size_t i = 0; i < neighbors.size(); ++i)
    {
        int neighborIndex = neighbors[i];
        Point &neighbor = m_points[neighborIndex];

        if (!neighbor.isVisited)
        {
            neighbor.isVisited = true;

            std::vector<int> neighborNeighbors = regionQuery(neighborIndex);

            if (neighborNeighbors.size() >= minPts)
            {
                neighbors.insert(neighbors.end(), neighborNeighbors.begin(), neighborNeighbors.end());
            }
        }
        if (neighbor.clusterId == UNCLASSIFIED || neighbor.clusterId == NOISE)
        {
            neighbor.clusterId = clusterId;
        }
    }
}