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
 * Description        : Header file for DBSCAN clustering algorithm implementation.
 *
 * Changelog:
 * - 0.1.0 (16 September 2025): Initial creation of DBSCAN header file.
 */

#ifndef DBSCAN_H
#define DBSCAN_H

#include <vector>
#include <string>

inline constexpr int NOISE = -1;
inline constexpr int UNCLASSIFIED = 0;

struct Point
{
    std::string object_id;
    double lat{}, lon{};
    int clusterId{UNCLASSIFIED};
    bool isVisited{false};
};

class DBSCAN
{
public:
    DBSCAN(double epsilon, int minPts, std::vector<Point> &points);

    void run();

private:
    std::vector<Point> &m_points;
    double m_epsilon;
    int m_minPts;

    // distance computation moved to Utils

    std::vector<int> regionQuery(int pointIndex);

    void expandCluster(int pointIndex, std::vector<int> &neighbors, int clusterId, int minPts);
};

#endif // DBSCAN_H