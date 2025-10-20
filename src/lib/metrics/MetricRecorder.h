#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace dbscan
{
    namespace metrics
    {
        using namespace std;
        using namespace std::chrono;
        using clock = steady_clock;

        class MetricRecorder
        {
        public:
            MetricRecorder();
            ~MetricRecorder();

            void start();
            void stop();

            void startPhase(const string &name);
            void stopPhase(const string &name);

            // Call this to increment neighbor query count
            void addNeighborQueries(uint64_t n = 1);

            // Setter for input parameters
            void setEpsilon(double eps);
            void setMinSamples(int ms);
            void setNumPoints(int n);
            void setThreads(int t);

            // Setter for output metrics
            void setNumClustersFound(int n);
            void setNumNoisePoints(int n);

            // Setter for quality metrics (optional)
            void setSilhouetteScore(double score);

            // Save results to JSON file
            bool saveToFile(const string &path) const;

        private:
            void samplerLoop();

            // Input Parameters
            double epsilon = 0.0;
            int min_samples = 0;
            int num_points = 0;
            int threads = 0;

            // Output Metrics
            int num_clusters_found = 0;
            int num_noise_points = 0;

            // Performance Metrics
            atomic<uint64_t> neighbor_queries{0};
            atomic<long> peak_rss_kb{0};

            // Phase tracking
            map<string, clock::time_point> phase_start;
            map<string, nanoseconds> phase_accum;
            mutable std::mutex phases_m;

            // Memory sampler thread control
            atomic<bool> running{false};
            std::thread samplerThread;
            std::mutex cv_m;
            std::condition_variable cv;
        };

    } // namespace metrics
} // namespace dbscan