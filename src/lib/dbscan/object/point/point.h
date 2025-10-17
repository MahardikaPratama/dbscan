#pragma once
#include <string>
#include <vector>

inline constexpr int NOISE = -1;
inline constexpr int UNCLASSIFIED = 0;

class Point
{
private:
    std::string object_id;
    double lat;
    double lon;
    int sensor;
    int clusterId;
    bool isVisited;

public:
    Point();
    Point(const std::string &id, double latitude, double longitude, int sensorId = 0);

    int getCluster() const;
    void setCluster(int val);
    int getSensor() const;
    double getLatitude() const;
    double getLongitude() const;
    bool visited() const;
    void setVisited(bool v);
    std::string getId() const;
};
