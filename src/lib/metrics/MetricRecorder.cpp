#include "MetricRecorder.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/resource.h>
#include <thread>
#include <unistd.h>
#include <cmath>

using namespace std;
using namespace std::chrono;

namespace dbscan
{
    namespace metrics
    {

        MetricRecorder::MetricRecorder() {}

        MetricRecorder::~MetricRecorder()
        {
            stop();
        }

        void MetricRecorder::start()
        {
            bool expected = false;
            if (!running.compare_exchange_strong(expected, true))
                return;
            samplerThread = std::thread(&MetricRecorder::samplerLoop, this);
        }

        void MetricRecorder::stop()
        {
            bool expected = true;
            if (!running.compare_exchange_strong(expected, false))
                return;
            cv.notify_all();
            if (samplerThread.joinable())
                samplerThread.join();
        }

        void MetricRecorder::startPhase(const string &name)
        {
            lock_guard<std::mutex> g(phases_m);
            phase_start[name] = clock::now();
        }

        void MetricRecorder::stopPhase(const string &name)
        {
            lock_guard<std::mutex> g(phases_m);
            auto it = phase_start.find(name);
            if (it == phase_start.end())
                return;
            auto diff = duration_cast<nanoseconds>(clock::now() - it->second);
            phase_accum[name] += diff;
            phase_start.erase(it);
        }

        void MetricRecorder::addNeighborQueries(uint64_t n)
        {
            neighbor_queries.fetch_add(n, std::memory_order_relaxed);
        }

        void MetricRecorder::setEpsilon(double eps) { epsilon = eps; }
        void MetricRecorder::setMinSamples(int ms) { min_samples = ms; }
        void MetricRecorder::setNumPoints(int n) { num_points = n; }
        void MetricRecorder::setThreads(int t) { threads = t; }
        void MetricRecorder::setNumClustersFound(int n) { num_clusters_found = n; }
        void MetricRecorder::setNumNoisePoints(int n) { num_noise_points = n; }

        bool MetricRecorder::saveToFile(const string &path) const
        {
            ofstream f(path);
            if (!f.is_open())
                return false;

            // Dapatkan RSS puncak terakhir dari sampler ATAU getrusage saat ini
            long final_peak_rss = peak_rss_kb.load();
            struct rusage ru;
            if (getrusage(RUSAGE_SELF, &ru) == 0)
            {
                if (ru.ru_maxrss > final_peak_rss)
                    final_peak_rss = ru.ru_maxrss;
            }

            // Calculate percentage of noise points
            double percentage_noise = 0.0;
            if (num_points > 0)
                percentage_noise = (double)num_noise_points / (double)num_points * 100.0;

            f << "{" << '\n';
            f << "  \"algorithm\": \"DBSCAN\",\n";

            // Input Parameters
            f << "  \"epsilon\": " << epsilon << ",\n";
            f << "  \"min_samples\": " << min_samples << ",\n";
            f << "  \"num_points\": " << num_points << ",\n";
            f << "  \"threads\": " << threads << ",\n";

            // Output Metrics
            f << "  \"num_clusters_found\": " << num_clusters_found << ",\n";
            f << "  \"num_noise_points\": " << num_noise_points << ",\n";
            f << "  \"percentage_noise\": " << fixed << setprecision(2) << percentage_noise << ",\n";

            // Performance Metrics
            f << "  \"peak_rss_kb\": " << final_peak_rss << ",\n";
            f << "  \"neighbor_queries\": " << neighbor_queries.load() << ",\n";

            f << "  \"phases_ms\": {\n";
            bool first = true;
            for (auto &kv : phase_accum)
            {
                if (!first)
                    f << ",\n";
                first = false;
                double ms = duration_cast<duration<double, milli>>(kv.second).count();
                f << "    \"" << kv.first << "\": " << fixed << setprecision(3) << ms;
            }
            f << "\n  }\n";

            f << "}\n";
            f.close();
            return true;
        }

        void MetricRecorder::samplerLoop()
        {
            // Simple sampler: periodically take RSS and update peak
            using namespace std::chrono_literals;
            while (running.load())
            {
                struct rusage ru;
                if (getrusage(RUSAGE_SELF, &ru) == 0)
                {
                    long rss = ru.ru_maxrss; // in KB
                    long prev = peak_rss_kb.load();
                    while (rss > prev && !peak_rss_kb.compare_exchange_weak(prev, rss))
                    {
                        // Loop until successful
                    }
                }
                std::unique_lock<std::mutex> lk(cv_m);
                cv.wait_for(lk, 200ms); // Sample every 200ms
            }
        }

    } // namespace metrics
} // namespace dbscan