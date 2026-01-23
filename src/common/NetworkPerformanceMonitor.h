#pragma once

#include "PerformanceUtils.h"
#include <chrono>
#include <atomic>
#include <vector>

namespace network_perf {

class NetworkPerformanceMonitor {
private:
    std::atomic<uint64_t> total_bytes_sent{0};
    std::atomic<uint64_t> total_bytes_received{0};
    std::atomic<uint64_t> total_packets_sent{0};
    std::atomic<uint64_t> total_packets_received{0};
    
    modern_utils::PerformanceTimer global_timer{"NetworkOperations"};
    
public:
    // Record network activity
    void record_sent(size_t bytes) {
        total_bytes_sent += bytes;
        total_packets_sent++;
    }
    
    void record_received(size_t bytes) {
        total_bytes_received += bytes;
        total_packets_received++;
    }
    
    // Get performance statistics
    struct Statistics {
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint64_t packets_sent;
        uint64_t packets_received;
        double elapsed_seconds;
        double bytes_per_second;
        double packets_per_second;
    };
    
    Statistics get_statistics() const {
        Statistics stats{};
        stats.bytes_sent = total_bytes_sent.load();
        stats.bytes_received = total_bytes_received.load();
        stats.packets_sent = total_packets_sent.load();
        stats.packets_received = total_packets_received.load();
        
        auto elapsed = global_timer.elapsed_time();
        stats.elapsed_seconds = elapsed.count() / 1000000.0; // μs to seconds
        
        if (stats.elapsed_seconds > 0) {
            stats.bytes_per_second = (stats.bytes_sent + stats.bytes_received) / stats.elapsed_seconds;
            stats.packets_per_second = (stats.packets_sent + stats.packets_received) / stats.elapsed_seconds;
        }
        
        return stats;
    }
    
    // Generate performance report
    modern_utils::StringBuffer generate_report() const {
        auto stats = get_statistics();
        modern_utils::StringBuffer report(512);
        
        report.append("Network Performance Report:\n")
              .append("  Bytes Sent: ").append(stats.bytes_sent).append("\n")
              .append("  Bytes Received: ").append(stats.bytes_received).append("\n")
              .append("  Packets Sent: ").append(stats.packets_sent).append("\n")
              .append("  Packets Received: ").append(stats.packets_received).append("\n")
              .append("  Duration: ").append(stats.elapsed_seconds).append("s\n")
              .append("  Throughput: ").append(stats.bytes_per_second).append(" B/s\n")
              .append("  Packet Rate: ").append(stats.packets_per_second).append(" pkt/s");
              
        return report;
    }
};

// Global network performance monitor
inline NetworkPerformanceMonitor g_network_perf_monitor;

// Helper macros for network performance monitoring
#ifdef NETWORK_PERF_MONITORING
#define RECORD_NETWORK_SENT(bytes) network_perf::g_network_perf_monitor.record_sent(bytes)
#define RECORD_NETWORK_RECEIVED(bytes) network_perf::g_network_perf_monitor.record_received(bytes)
#else
#define RECORD_NETWORK_SENT(bytes)
#define RECORD_NETWORK_RECEIVED(bytes)
#endif

} // namespace network_perf