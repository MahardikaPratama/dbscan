#pragma once
#include "../dbscan.h"

class DBSCANRegular : public DBSCANBase
{
public:
    DBSCANRegular(double epsilon, int minPts);
};