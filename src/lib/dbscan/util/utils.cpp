#include "utils.h"
#include <cmath>

namespace dbscan
{

    std::vector<int> regionQuery(const std::vector<Point> &points, int idx, double eps)
    {
        std::vector<int> neighbors;
        if (idx < 0 || idx >= (int)points.size())
            return neighbors;
        const Point &p = points[idx];
        for (int i = 0; i < (int)points.size(); ++i)
        {
            if (i == idx)
                continue;
            const Point &q = points[i];
            // haversine::calculate_distance expects (lat1, lon1, lat2, lon2)
            if (calculate_distance(p.getLatitude(), p.getLongitude(), q.getLatitude(), q.getLongitude()) <= eps)
                neighbors.push_back(i);
        }
        return neighbors;
    }

} // namespace dbscan
