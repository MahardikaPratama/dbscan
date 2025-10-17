#ifndef DBSCAN_UTILS_H
#define DBSCAN_UTILS_H

#include <vector>
#include "../object/point/point.h"
#include "../../haversine/haversine.h"

namespace dbscan
{
    // Return indices of points within radius eps of point at index idx
    std::vector<int> regionQuery(const std::vector<Point> &points, int idx, double eps);

} // namespace dbscan

#endif // DBSCAN_UTILS_H
