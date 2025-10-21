
#include "lib/dbscan/dbscan.h"
#include "lib/dbscan/dbscan_regular/dbscan_regular.h"
#include "lib/dbscan/kdbscan/kdbscan.h"
#include "lib/dbscan/dbscan_dynamic_epsilon/dbscan_dynamic_epsilon.h"
#include "lib/dbscan/kdbscan/kdbscan.h"
#include "lib/dbscan/object/point/point.h"
#include "lib/data_handler/data_saver/data_saver.h"
#include "lib/data_handler/data_reader/data_reader.h"
#include "lib/metrics/MetricRecorder.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

void run_original_dbscan(const vector<Point> &points, const string &outdir, const string &inputCsvPath)
{
    int minPts = 1;
    // Choose epsilon based on filename: use larger radius for Airplane datasets
    std::string lowerFilename = inputCsvPath;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
    double epsilon = 0.3; // default in kilometers (300 meters)
    if (lowerFilename.find("airplane") != std::string::npos)
    {
        epsilon = 9.26; // ~5 nautical miles in kilometers
    }
    vector<Point> pts = points;
    string od = outdir + "/DBSCAN_original";
    DBSCANBase dbscan(epsilon, minPts);
    DBSCANResult res;
    dbscan::metrics::MetricRecorder *recorder = nullptr;
    // Cek apakah environment metrics aktif
    if (getenv("DBSCAN_ENABLE_METRICS"))
    {
        static dbscan::metrics::MetricRecorder static_rec;
        recorder = &static_rec;
    }
    double sse = dbscan.run(pts, &res, recorder);
    DataSaver::save(res, od, inputCsvPath, true);
    if (recorder)
    {
        recorder->saveToFile(od + "/dbscan_metrics.json");
    }
    cout << "DBSCAN Original run completed. SSE: " << sse << " Written to " << od << endl;
}

void run_kdbscan(const vector<Point> &points, const string &outdir, const string &inputCsvPath)
{
    int minPts = 1;
    // Choose epsilon based on filename: use larger radius for Airplane datasets
    std::string lowerFilename = inputCsvPath;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
    double epsilon = 0.3; // default in kilometers
    if (lowerFilename.find("airplane") != std::string::npos)
    {
        epsilon = 9.26;
    }

    vector<Point> pts = points;
    string od = outdir + "/KDBSCAN";
    KDBSCAN kdb(epsilon, minPts);
    DBSCANResult res;
    dbscan::metrics::MetricRecorder *recorder = nullptr;
    // Cek apakah environment metrics aktif
    if (getenv("DBSCAN_ENABLE_METRICS"))
    {
        static dbscan::metrics::MetricRecorder static_rec;
        recorder = &static_rec;
    }
    double sse = kdb.run(pts, &res, recorder);
    DataSaver::save(res, od, inputCsvPath, true);
    if (recorder)
        recorder->saveToFile(od + "/dbscan_metrics.json");
    cout << "KDBSCAN run completed. SSE: " << sse << " Written to " << od << endl;
}

void run_dynamic_epsilon_dbscan(const vector<Point> &points, const string &outdir, const string &inputCsvPath)
{
    int minPts = 1;
    vector<Point> pts = points;
    string od = outdir + "/DBSCAN_dynamic_epsilon";
    DBSCANDynamicEpsilon dbscan(minPts);
    DBSCANResult res;
    dbscan::metrics::MetricRecorder *recorder = nullptr;
    // Cek apakah environment metrics aktif
    if (getenv("DBSCAN_ENABLE_METRICS"))
    {
        static dbscan::metrics::MetricRecorder static_rec;
        recorder = &static_rec;
    }
    double sse = dbscan.run(pts, &res, recorder);
    DataSaver::save(res, od, inputCsvPath, true);
    if (recorder)
    {
        recorder->saveToFile(od + "/dbscan_metrics.json");
    }
    cout << "DBSCAN Dynamic Epsilon run completed. SSE: " << sse << " Written to " << od << endl;
}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "Usage: ./dbscan <INPUT.csv> <OUT-DIR> [--metrics]" << endl;
        return 1;
    }

    bool enable_metrics = false;
    vector<string> args;
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        ;
        if (arg == "--metrics")
        {
            enable_metrics = true;
        }
        else
        {
            args.push_back(arg);
        }
    }
    if (args.size() < 2)
    {
        cout << "Usage: ./dbscan <INPUT.csv> <OUT-DIR> [--metrics]" << endl;
        return 1;
    }
    string input = args[0];
    string outdir = args[1];

    vector<Point> points;
    if (!DataReader::loadPointsFromCSV(input, points))
    {
        cerr << "Error: Failed to read CSV input: " << input << endl;
        return 2;
    }

    // Run original DBSCAN
    vector<Point> pts = points;
    string od = outdir + "/DBSCAN_original";
    if (enable_metrics)
    {
        setenv("DBSCAN_ENABLE_METRICS", "1", 1);
    }
    run_original_dbscan(pts, outdir, input);

    // Run KDBSCAN
    pts = points;
    od = outdir + "/KDBSCAN";
    run_kdbscan(pts, outdir, input);

    // Run Dynamic Epsilon DBSCAN
    pts = points;
    od = outdir + "/DBSCAN_dynamic_epsilon";
    run_dynamic_epsilon_dbscan(pts, outdir, input);

    return 0;
}