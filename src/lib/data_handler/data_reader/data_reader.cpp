#include "data_reader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <cctype>

bool DataReader::loadPointsFromCSV(const std::string &filename, std::vector<Point> &out)
{
    std::ifstream infile(filename);
    if (!infile.is_open())
        return false;

    std::string header;
    if (!std::getline(infile, header))
        return false;

    std::vector<std::string> cols;
    std::istringstream hss(header);
    std::string col;
    while (std::getline(hss, col, ','))
    {
        size_t a = col.find_first_not_of(" \t\r\n");
        size_t b = col.find_last_not_of(" \t\r\n");
        if (a == std::string::npos)
            cols.push_back("");
        else
            cols.push_back(col.substr(a, b - a + 1));
    }

    int idxObject = -1, idxLat = -1, idxLon = -1, idxSensor = -1;
    for (size_t i = 0; i < cols.size(); ++i)
    {
        std::string c = cols[i];
        for (auto &ch : c)
            ch = tolower(ch);
        if (c.find("object") != std::string::npos || c.find("id") != std::string::npos)
            idxObject = i;
        else if (c.find("lat") != std::string::npos)
            idxLat = i;
        else if (c.find("lon") != std::string::npos)
            idxLon = i;
        else if (c.find("sensor") != std::string::npos)
            idxSensor = i;
    }

    if (idxLat < 0 || idxLon < 0)
    {
        std::cerr << "CSV missing lat/lon columns" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(infile, line))
    {
        if (line.find_first_not_of(" \t\r\n,") == std::string::npos)
            continue;

        std::vector<std::string> fields;
        std::istringstream ss(line);
        std::string f;
        while (std::getline(ss, f, ','))
            fields.push_back(f);

        int maxIdx = std::max({idxObject, idxLat, idxLon, idxSensor});
        if ((int)fields.size() <= maxIdx)
            continue;

        std::string objectId = (idxObject >= 0) ? fields[idxObject] : std::to_string((int)out.size() + 1);
        std::string slat = fields[idxLat];
        std::string slon = fields[idxLon];
        int sensorVal = (idxSensor >= 0) ? std::stoi(fields[idxSensor]) : 0;

        double dlat = 0.0, dlon = 0.0;
        try
        {
            dlat = std::stod(slat);
            dlon = std::stod(slon);
        }
        catch (...)
        {
            continue;
        }

        Point p(objectId, dlat, dlon, sensorVal);
        out.push_back(p);
    }
    infile.close();
    return !out.empty();
}

int DataReader::countUniqueObjects(const std::string &filename)
{
    std::ifstream infile(filename);
    if (!infile.is_open())
        return 0;
    std::string header;
    if (!std::getline(infile, header))
        return 0;
    std::vector<std::string> cols;
    std::istringstream hss(header);
    std::string col;
    while (std::getline(hss, col, ','))
        cols.push_back(col);
    int idxObject = -1;
    for (size_t i = 0; i < cols.size(); ++i)
    {
        std::string c = cols[i];
        for (auto &ch : c)
            ch = tolower(ch);
        if (c.find("object") != std::string::npos || c.find("id") != std::string::npos)
        {
            idxObject = i;
            break;
        }
    }
    if (idxObject < 0)
        return 0;
    std::unordered_set<std::string> uniq;
    std::string line;
    while (std::getline(infile, line))
    {
        if (line.empty())
            continue;
        std::istringstream ss(line);
        std::string f;
        int colidx = 0;
        while (std::getline(ss, f, ','))
        {
            if (colidx == idxObject)
            {
                size_t a = f.find_first_not_of(" \t\r\n");
                size_t b = f.find_last_not_of(" \t\r\n");
                if (a != std::string::npos)
                    uniq.insert(f.substr(a, b - a + 1));
                break;
            }
            ++colidx;
        }
    }
    infile.close();
    return (int)uniq.size();
}