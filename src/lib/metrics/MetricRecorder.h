#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace dbscan
{
    namespace metrics
    {

        class MetricRecorder
        {
        public:
            MetricRecorder();
            ~MetricRecorder();

            // Start/stop background sampler thread
            void start();
            void stop();

            // Phase timing
            void startPhase(const std::string &name);
            void stopPhase(const std::string &name);

            // Counter
            void addDistanceCalls(uint64_t n = 1);

            // Metadata setters
            void setFinalSSE(double sse);
            void setNumPoints(int n);
            void setNumNoise(int n);
            void setNumClusters(int n);
            void setDistanceStats(double mean_km, double median_km, double max_km, double min_km);

            // Persist metrics to a JSON-ish file
            bool saveToFile(const std::string &path) const;

        private:
            using clock = std::chrono::high_resolution_clock;

            // background sampler
            std::atomic<bool> running{false};
            std::thread samplerThread;
            std::condition_variable cv;
            std::mutex cv_m;

            // phase accumulators
            mutable std::mutex phases_m;
            std::unordered_map<std::string, std::chrono::nanoseconds> phase_accum;
            std::unordered_map<std::string, clock::time_point> phase_start;

            std::atomic<uint64_t> distance_calls{0};

            // sampled stats
            std::atomic<long> peak_rss_kb{0};
            mutable std::mutex sampled_m;
            std::vector<double> cpu_samples;

            // metadata
            int num_points = 0;
            int num_noise = 0;
            int num_clusters = 0;
            double final_sse = 0.0;
            double mean_dist_km = 0.0;
            double median_dist_km = 0.0;
            double max_dist_km = 0.0;
            double min_dist_km = 0.0;

            // sampler loop
            void samplerLoop();
        };

    } // namespace metrics
} // namespace dbscan