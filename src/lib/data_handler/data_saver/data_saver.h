#pragma once
#include "../../dbscan/dbscan.h"
#include <string>

class DataSaver
{
public:
    // If writeAccuracyFormulas is true, the saver will append a "Cluster Accuracy" column
    // with the same COUNTIFS-style Excel formula per row and a "Total Accuracy" formula
    // in the header (as seen in Results/original_with_formula.csv). Default: false.
    static bool save(const DBSCANResult &res, const std::string &outdir, const std::string &inputCsvPath, bool writeAccuracyFormulas = false);
};