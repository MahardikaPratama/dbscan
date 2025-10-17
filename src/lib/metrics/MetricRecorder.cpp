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

        void MetricRecorder::addDistanceCalls(uint64_t n)
        {
            distance_calls.fetch_add(n, std::memory_order_relaxed);
        }

        void MetricRecorder::setFinalSSE(double sse) { final_sse = sse; }
        void MetricRecorder::setNumPoints(int n) { num_points = n; }
        void MetricRecorder::setNumNoise(int n) { num_noise = n; }
        void MetricRecorder::setNumClusters(int n) { num_clusters = n; }
        void MetricRecorder::setDistanceStats(double mean_km, double median_km, double max_km, double min_km)
        {
            mean_dist_km = mean_km;
            median_dist_km = median_km;
            max_dist_km = max_km;
            min_dist_km = min_km;
        }

        bool MetricRecorder::saveToFile(const string &path) const
        {
            ofstream f(path);
            if (!f.is_open())
                return false;

            struct rusage ru;
            long peak_rss = 0;
            if (getrusage(RUSAGE_SELF, &ru) == 0)
                peak_rss = ru.ru_maxrss;

            f << "{" << '\n';
            f << "  \"final_sse\": " << final_sse << ",\n";
            // RMSE = sqrt(SSE / N)
            double rmse = 0.0;
            if (num_points > 0)
                rmse = sqrt(final_sse / (double)num_points);
            f << "  \"num_points\": " << num_points << ",\n";
            f << "  \"num_noise\": " << num_noise << ",\n";
            f << "  \"num_clusters\": " << num_clusters << ",\n";
            f << "  \"rmse_km\": " << rmse << ",\n";
            f << "  \"mean_dist_km\": " << mean_dist_km << ",\n";
            f << "  \"median_dist_km\": " << median_dist_km << ",\n";
            f << "  \"max_dist_km\": " << max_dist_km << ",\n";
            f << "  \"min_dist_km\": " << min_dist_km << ",\n";
            f << "  \"peak_rss_kb\": " << peak_rss << ",\n";

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
            // Simple sampler: sample RSS periodically and update peak
            using namespace std::chrono_literals;
            while (running.load())
            {
                struct rusage ru;
                if (getrusage(RUSAGE_SELF, &ru) == 0)
                {
                    long rss = ru.ru_maxrss;
                    long prev = peak_rss_kb.load();
                    if (rss > prev)
                        peak_rss_kb.store(rss);
                }
                std::unique_lock<std::mutex> lk(cv_m);
                cv.wait_for(lk, 200ms);
            }
        }

    } // namespace metrics
} // namespace dbscan