#include "data_saver.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

static bool make_dirs_recursive(const std::string &dir)
{
    if (dir.empty())
        return false;
    std::string cur;
    size_t i = 0;
    if (dir[0] == '/')
    {
        cur = "/";
        i = 1;
    }
    for (; i < dir.size(); ++i)
    {
        cur.push_back(dir[i]);
        if (dir[i] == '/' || i == dir.size() - 1)
        {
            std::string toCreate = cur;
            if (toCreate.size() > 1 && toCreate.back() == '/')
                toCreate.pop_back();
            if (toCreate.empty())
                continue;
            if (mkdir(toCreate.c_str(), 0755) != 0 && errno != EEXIST)
            {
                return false;
            }
        }
    }
    return true;
}

bool writeCsvWithClusters(const std::string &inputCsvPath, const DBSCANResult &res, const std::string &outCsvPath, bool writeAccuracyFormulas = false)
{
    std::ifstream infile(inputCsvPath, std::ios::binary);
    if (!infile.is_open())
        return false;

    // Read entire file into a string so we can properly parse CSV records that may
    // contain embedded newlines inside quoted fields.
    std::ostringstream ss;
    ss << infile.rdbuf();
    std::string fileContent = ss.str();

    // Parse fileContent into CSV records (lines), respecting quotes.
    auto parseCsvRecords = [](const std::string &s)
    {
        std::vector<std::string> records;
        std::string cur;
        bool inQuotes = false;
        for (size_t i = 0; i < s.size(); ++i)
        {
            char c = s[i];
            // Ignore bare CR; treat LF as newline boundary (handle CRLF by ignoring CR)
            if (c == '\r')
                continue;
            if (c == '"')
            {
                // handle escaped quote "" -> " inside quoted field
                if (inQuotes && i + 1 < s.size() && s[i + 1] == '"')
                {
                    cur.push_back('"');
                    ++i; // skip the escape
                }
                else
                {
                    inQuotes = !inQuotes;
                }
            }
            else if (c == '\n' && !inQuotes)
            {
                records.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }
        if (!cur.empty())
            records.push_back(cur);
        return records;
    };

    std::vector<std::string> records = parseCsvRecords(fileContent);
    if (records.empty())
        return false;

    // First record is the header; the rest are data rows
    std::string header = records[0];
    std::vector<std::string> lines;
    for (size_t i = 1; i < records.size(); ++i)
        lines.push_back(records[i]);

    std::ofstream outfile(outCsvPath);
    if (!outfile.is_open())
    {
        infile.close();
        return false;
    }
    if (!header.empty() && header.back() == ',')
        header.pop_back();

    // Parse header columns to locate ObjectID/Cluster names (case-insensitive)
    auto trim = [](std::string s)
    {
        // trim spaces
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
            s.erase(s.begin());
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
            s.pop_back();
        // remove surrounding quotes if present
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            s = s.substr(1, s.size() - 2);
        // unescape double quotes inside
        std::string out;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '"' && i + 1 < s.size() && s[i + 1] == '"')
            {
                out.push_back('"');
                ++i;
            }
            else
                out.push_back(s[i]);
        }
        return out;
    };
    auto toLower = [](const std::string &s)
    {
        std::string o = s;
        for (char &c : o)
            c = std::tolower((unsigned char)c);
        return o;
    };

    // split a single CSV record into fields respecting quoted fields and escaped quotes
    auto splitCsvRecord = [](const std::string &rec)
    {
        std::vector<std::string> fields;
        std::string cur;
        bool inQuotes = false;
        for (size_t i = 0; i < rec.size(); ++i)
        {
            char c = rec[i];
            if (c == '"')
            {
                if (inQuotes && i + 1 < rec.size() && rec[i + 1] == '"')
                {
                    cur.push_back('"');
                    ++i; // skip escape
                }
                else
                {
                    inQuotes = !inQuotes;
                }
            }
            else if (c == ',' && !inQuotes)
            {
                fields.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }
        fields.push_back(cur);
        return fields;
    };

    std::vector<std::string> cols;
    {
        std::vector<std::string> rawCols = splitCsvRecord(header);
        for (auto &c : rawCols)
            cols.push_back(trim(c));
    }

    // find indices (0-based) for object id and cluster if present
    int idxObjectId = -1;
    int idxCluster = -1;
    int idxClusterAcc = -1;
    for (size_t i = 0; i < cols.size(); ++i)
    {
        std::string l = toLower(cols[i]);
        if (l == "objectid" || l == "object id" || l == "object_id")
            idxObjectId = (int)i;
        if (l == "cluster")
            idxCluster = (int)i;
        if (l == "cluster accuracy" || l == "cluster_accuracy" || l == "clusteraccuracy")
            idxClusterAcc = (int)i;
    }

    // When we append Cluster and Cluster Accuracy, their indices will be at the end
    int baseCols = (int)cols.size();
    if (idxCluster == -1)
        idxCluster = baseCols; // will be appended
    if (idxClusterAcc == -1)
        idxClusterAcc = baseCols + 1; // will be appended
    if (idxObjectId == -1)
        idxObjectId = 0; // fallback to first column

    auto colIndexToLetter = [](int index)
    {
        // index is 0-based, convert to Excel-style letters
        int n = index + 1;
        std::string s;
        while (n > 0)
        {
            int rem = (n - 1) % 26;
            s.push_back('A' + rem);
            n = (n - 1) / 26;
        }
        std::reverse(s.begin(), s.end());
        return s;
    };

    std::string colObjLetter = colIndexToLetter(idxObjectId);
    std::string colClusterLetter = colIndexToLetter(idxCluster);
    std::string colClusterAccLetter = colIndexToLetter(idxClusterAcc);

    // If writing formulas, append two columns: Cluster Accuracy and Total Accuracy (Total only in first row's formula cell)
    if (writeAccuracyFormulas)
    {
        outfile << header << ",Cluster,Cluster Accuracy,Total Accuracy\n";
    }
    else
    {
        outfile << header << ",Cluster\n";
    }

    // Write rows from previously-read lines and attach cluster IDs and optional formulas
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string line = lines[i];
        if (!line.empty() && line.back() == ',')
            line.pop_back();

        if (i < res.points.size())
        {
            outfile << line << "," << res.points[i].getCluster();
        }
        else
        {
            outfile << line << ","
                    << "";
        }

        if (writeAccuracyFormulas)
        {
            // Excel-style formulas use 1-based indexing; data rows start at row 2 (header row at 1)
            size_t row = i + 2;

            // Build the COUNTIFS-based formula using detected column letters. Do NOT force percentage.
            std::ostringstream formula;
            // format: =MIN( COUNTIFS($C:$C,$C$2,$A:$A,$A$2)/COUNTIF($C:$C,$C$2), COUNTIFS($A:$A,$A$2,$C:$C,$C$2)/COUNTIF($A:$A,$A$2) )
            formula << "=MIN(  COUNTIFS($" << colClusterLetter << ":$" << colClusterLetter << ",$" << colClusterLetter << "$" << row << ",$" << colObjLetter << ":$" << colObjLetter << ",$" << colObjLetter << "$" << row << ")/COUNTIF($" << colClusterLetter << ":$" << colClusterLetter << ",$" << colClusterLetter << "$" << row << "),  COUNTIFS($" << colObjLetter << ":$" << colObjLetter << ",$" << colObjLetter << "$" << row << ",$" << colClusterLetter << ":$" << colClusterLetter << ",$" << colClusterLetter << "$" << row << ")/COUNTIF($" << colObjLetter << ":$" << colObjLetter << ",$" << colObjLetter << "$" << row << ") )";
            outfile << ",\"" << formula.str() << "\"";

            // For the Total Accuracy column, place aggregate formula in the first data row cell
            if (i == 0)
            {
                size_t lastRow = lines.size() + 1; // header is row 1
                std::ostringstream totalF;
                totalF << "=SUMPRODUCT(" << colClusterAccLetter << "2:" << colClusterAccLetter << lastRow << ")/COUNTA(" << colClusterAccLetter << "2:" << colClusterAccLetter << lastRow << ")";
                outfile << "," << totalF.str();
            }
            else
            {
                outfile << ",";
            }
        }

        outfile << "\n";
    }

    infile.close();
    outfile.close();
    return true;
}

bool DataSaver::save(const DBSCANResult &res, const std::string &outdir, const std::string &inputCsvPath, bool writeAccuracyFormulas)
{
    if (!make_dirs_recursive(outdir))
    {
        std::cerr << "Warning: could not create directory " << outdir << std::endl;
    }

    std::string pointsCsv = outdir + "/original.csv";
    if (!writeCsvWithClusters(inputCsvPath, res, pointsCsv, writeAccuracyFormulas))
    {
        std::cerr << "Error: failed to write points CSV to " << pointsCsv << std::endl;
        return false;
    }
    return true;
}