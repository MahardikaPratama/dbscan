#pragma once
#include "../../dbscan/object/point/point.h"
#include <string>
#include <vector>

class DataReader
{
public:
    static bool loadPointsFromCSV(const std::string &filename, std::vector<Point> &out);
    static int countUniqueObjects(const std::string &filename);
};