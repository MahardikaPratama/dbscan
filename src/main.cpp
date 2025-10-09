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

int main()
{
    std::string filename = "../data/TrackDataset.csv";
    std::vector<Point> points = Utils::readCSV(filename);

    if (points.empty())
    {
        std::cerr << "Dataset is empty or failed to load.\n";
        return 1;
    }

    double epsilon = 2;
    int minPts = 3;
    DBSCAN dbscan(epsilon, minPts, points);
    dbscan.run();

    Utils::writeCSV("../data/ClusteredOutput.csv", points);
    std::cout << "DBSCAN completed. Results written to ClusteredOutput.csv\n";
    return 0;
}
