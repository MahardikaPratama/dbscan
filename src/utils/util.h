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
 * Description        : Header file for utility functions.
 *
 * Changelog:
 * - 0.1.0 (16 September 2025): Initial creation of utility header file.
 */

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include "algorithm/dbscan.h"

namespace Utils
{
    std::vector<Point> readCSV(const std::string &filename);

    void writeCSV(const std::string &filename, const std::vector<Point> &points);

    double haversineDistance(const Point &p1, const Point &p2);

    double generateKDistanceEpsilon(const std::vector<Point> &points, int minPts);
}

#endif // UTILS_H
