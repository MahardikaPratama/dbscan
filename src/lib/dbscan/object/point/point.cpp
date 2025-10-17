#include "point.h"

Point::Point()
    : object_id(), lat(0.0), lon(0.0), sensor(0), clusterId(UNCLASSIFIED), isVisited(false) {}

Point::Point(const std::string &id, double latitude, double longitude, int sensorId)
    : object_id(id), lat(latitude), lon(longitude), sensor(sensorId), clusterId(UNCLASSIFIED), isVisited(false) {}

int Point::getCluster() const { return clusterId; }
void Point::setCluster(int val) { clusterId = val; }
int Point::getSensor() const { return sensor; }
double Point::getLatitude() const { return lat; }
double Point::getLongitude() const { return lon; }
bool Point::visited() const { return isVisited; }
void Point::setVisited(bool v) { isVisited = v; }
std::string Point::getId() const { return object_id; }