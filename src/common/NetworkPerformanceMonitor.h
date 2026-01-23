#pragma once

#include "PerformanceUtils.h"
#include "NetworkSummaryUtil.h"
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
    
    // Protocol-specific tracking
    std::atomic<uint64_t> tcp_bytes_sent{0};
    std::atomic<uint64_t> tcp_bytes_received{0};
    std::atomic<uint64_t> tcp_packets_sent{0};
    std::atomic<uint64_t> tcp_packets_received{0};
    
    std::atomic<uint64_t> udp_bytes_sent{0};
    std::atomic<uint64_t> udp_bytes_received{0};
    std::atomic<uint64_t> udp_packets_sent{0};
    std::atomic<uint64_t> udp_packets_received{0};
    
    modern_utils::PerformanceTimer global_timer{"NetworkOperations"};
    
    // Summary utility
    CNetworkSummaryUtil summary_util;
    
public:
    // Record network activity
    void record_sent(size_t bytes, bool is_tcp = true) {
        total_bytes_sent += bytes;
        total_packets_sent++;
        
        if (is_tcp) {
            tcp_bytes_sent += bytes;
            tcp_packets_sent++;
            summary_util.record_tcp_activity(0, bytes);
        } else {
            udp_bytes_sent += bytes;
            udp_packets_sent++;
            summary_util.record_udp_activity(0, bytes);
        }
    }
    
    void record_received(size_t bytes, bool is_tcp = true) {
        total_bytes_received += bytes;
        total_packets_received++;
        
        if (is_tcp) {
            tcp_bytes_received += bytes;
            tcp_packets_received++;
            summary_util.record_tcp_activity(bytes, 0);
        } else {
            udp_bytes_received += bytes;
            udp_packets_received++;
            summary_util.record_udp_activity(bytes, 0);
        }
    }
    
    // Protocol-specific recording
    void record_tcp_sent(size_t bytes) { record_sent(bytes, true); }
    void record_tcp_received(size_t bytes) { record_received(bytes, true); }
    void record_udp_sent(size_t bytes) { record_sent(bytes, false); }
    void record_udp_received(size_t bytes) { record_received(bytes, false); }
    
    // Get performance statistics
    struct Statistics {
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint64_t packets_sent;
        uint64_t packets_received;
        uint64_t tcp_bytes_sent;
        uint64_t tcp_bytes_received;
        uint64_t tcp_packets_sent;
        uint64_t tcp_packets_received;
        uint64_t udp_bytes_sent;
        uint64_t udp_bytes_received;
        uint64_t udp_packets_sent;
        uint64_t udp_packets_received;
        double elapsed_seconds;
        double bytes_per_second;
        double packets_per_second;
        double tcp_bytes_per_second;
        double udp_bytes_per_second;
    };
    
    Statistics get_statistics() const {
        Statistics stats{};
        stats.bytes_sent = total_bytes_sent.load();
        stats.bytes_received = total_bytes_received.load();
        stats.packets_sent = total_packets_sent.load();
        stats.packets_received = total_packets_received.load();
        
        stats.tcp_bytes_sent = tcp_bytes_sent.load();
        stats.tcp_bytes_received = tcp_bytes_received.load();
        stats.tcp_packets_sent = tcp_packets_sent.load();
        stats.tcp_packets_received = tcp_packets_received.load();
        
        stats.udp_bytes_sent = udp_bytes_sent.load();
        stats.udp_bytes_received = udp_bytes_received.load();
        stats.udp_packets_sent = udp_packets_sent.load();
        stats.udp_packets_received = udp_packets_received.load();
        
        auto elapsed = global_timer.elapsed_time();
        stats.elapsed_seconds = elapsed.count() / 1000000.0; // μs to seconds
        
        if (stats.elapsed_seconds > 0) {
            stats.bytes_per_second = (stats.bytes_sent + stats.bytes_received) / stats.elapsed_seconds;
            stats.packets_per_second = (stats.packets_sent + stats.packets_received) / stats.elapsed_seconds;
            stats.tcp_bytes_per_second = (stats.tcp_bytes_sent + stats.tcp_bytes_received) / stats.elapsed_seconds;
            stats.udp_bytes_per_second = (stats.udp_bytes_sent + stats.udp_bytes_received) / stats.elapsed_seconds;
        }
        
        return stats;
    }
    
    // Get detailed breakdown
    CNetworkSummaryUtil& get_summary_util() { return summary_util; }
    const CNetworkSummaryUtil& get_summary_util() const { return summary_util; }
    
    // Generate performance report
    modern_utils::StringBuffer generate_report() const {
        auto stats = get_statistics();
        modern_utils::StringBuffer report(1024);
        
        report.append("Network Performance Report:\n")
              .append("  Total Bytes: Sent=").append(stats.bytes_sent)
              .append(", Received=").append(stats.bytes_received).append("\n")
              .append("  Total Packets: Sent=").append(stats.packets_sent)
              .append(", Received=").append(stats.packets_received).append("\n")
              .append("  TCP: ").append(stats.tcp_bytes_sent + stats.tcp_bytes_received)
              .append(" bytes, ").append(stats.tcp_packets_sent + stats.tcp_packets_received).append(" packets\n")
              .append("  UDP: ").append(stats.udp_bytes_sent + stats.udp_bytes_received)
              .append(" bytes, ").append(stats.udp_packets_sent + stats.udp_packets_received).append(" packets\n")
              .append("  Duration: ").append(stats.elapsed_seconds).append("s\n")
              .append("  Throughput: ").append(stats.bytes_per_second).append(" B/s (TCP: ")
              .append(stats.tcp_bytes_per_second).append(" B/s, UDP: ")
              .append(stats.udp_bytes_per_second).append(" B/s)");
              
        return report;
    }
    
    // Generate protocol-specific summary
    modern_utils::StringBuffer generate_protocol_summary() const {
        auto stats = get_statistics();
        modern_utils::StringBuffer summary(512);
        
        summary.append("Protocol Breakdown:\n")
               .append("  TCP: ").append(stats.tcp_bytes_sent + stats.tcp_bytes_received)
               .append(" bytes (").append(100.0 * (stats.tcp_bytes_sent + stats.tcp_bytes_received) / 
                       (stats.bytes_sent + stats.bytes_received)).append("%)\n")
               .append("  UDP: ").append(stats.udp_bytes_sent + stats.udp_bytes_received)
               .append(" bytes (").append(100.0 * (stats.udp_bytes_sent + stats.udp_bytes_received) / 
                       (stats.bytes_sent + stats.bytes_received)).append("%)");
               
        return summary;
    }
};

// Global network performance monitor
inline NetworkPerformanceMonitor g_network_perf_monitor;

// Helper macros for network performance monitoring
#ifdef NETWORK_PERF_MONITORING
#define RECORD_NETWORK_SENT(bytes) network_perf::g_network_perf_monitor.record_sent(bytes)
#define RECORD_NETWORK_RECEIVED(bytes) network_perf::g_network_perf_monitor.record_received(bytes)
#define RECORD_TCP_SENT(bytes) network_perf::g_network_perf_monitor.record_tcp_sent(bytes)
#define RECORD_TCP_RECEIVED(bytes) network_perf::g_network_perf_monitor.record_tcp_received(bytes)
#define RECORD_UDP_SENT(bytes) network_perf::g_network_perf_monitor.record_udp_sent(bytes)
#define RECORD_UDP_RECEIVED(bytes) network_perf::g_network_perf_monitor.record_udp_received(bytes)
#else
#define RECORD_NETWORK_SENT(bytes)
#define RECORD_NETWORK_RECEIVED(bytes)
#define RECORD_TCP_SENT(bytes)
#define RECORD_TCP_RECEIVED(bytes)
#define RECORD_UDP_SENT(bytes)
#define RECORD_UDP_RECEIVED(bytes)
#endif

} // namespace network_perf