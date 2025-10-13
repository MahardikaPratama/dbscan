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
 * Description        : Main entry point for DBSCAN clustering application.
 *
 * Changelog:
 * - 0.1.0 (16 September 2025): Initial creation of main entry point.
 */

#include "algorithm/dbscan.h"
#include "utils/util.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_csv_path>\n";
        return 1;
    }
    std::string filename = argv[1];
    std::vector<Point> points = Utils::readCSV(filename);

    if (points.empty())
    {
        std::cerr << "Dataset is empty or failed to load.\n";
        return 1;
    }

    int minPts = 1;
    // Choose epsilon based on filename: use larger radius for Airplane datasets
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
    double epsilon = 0.3; // default in kilometers (300 meters)
    if (lowerFilename.find("airplane") != std::string::npos)
    {
        epsilon = 9.26; // ~5 nautical miles in kilometers
    }
    DBSCAN dbscan(epsilon, minPts, points);
    dbscan.run();

    namespace fs = std::filesystem;
    fs::path inputPath(filename);
    std::string stem = inputPath.stem().string();
    fs::path outDir = fs::path("../output");
    if (!fs::exists(outDir))
    {
        std::error_code ec;
        fs::create_directories(outDir, ec);
        if (ec)
        {
            std::cerr << "Failed to create output directory: " << outDir << " (" << ec.message() << ")\n";
            return 1;
        }
    }
    fs::path outPath = outDir / ("Result_" + stem + ".csv");

    Utils::writeCSV(outPath.string(), points);
    std::cout << "DBSCAN completed. Results written to " << outPath << "\n";
    return 0;
}
