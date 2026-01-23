#include "common/ModernLogging.h"
#include "common/PerformanceUtils.h"
#include <iostream>

// 传统方式 - 多次字符串分配
void traditional_logging() {
    modern_log::Log("Starting data processing");
    modern_log::Log("Processing file: data.txt");
    modern_log::Log("File size: 1024 bytes");
    modern_log::Log("Processing completed");
}

// 优化方式 - 使用StringBuffer减少分配
void optimized_logging() {
    using namespace modern_utils;
    
    StringBuffer buf1(128);
    buf1.append("Starting data processing");
    modern_log::Log(std::string_view(buf1.str()));
    
    StringBuffer buf2(128);
    buf2.append("Processing file: data.txt");
    modern_log::Log(std::string_view(buf2.str()));
    
    StringBuffer buf3(128);
    buf3.append("File size: ").append(1024).append(" bytes");
    modern_log::Log(std::string_view(buf3.str()));
    
    StringBuffer buf4(128);
    buf4.append("Processing completed");
    modern_log::Log(std::string_view(buf4.str()));
}

// 批量处理优化的例子
void batch_processing_example() {
    using namespace modern_utils;
    
    const char* operations[] = {
        "network_connect", "file_read", "data_process", "network_send"
    };
    
    for (int i = 0; i < 4; ++i) {
        // 使用编译时哈希快速路由
        switch (ct_hash(operations[i])) {
            case ct_hash("network_connect"):
                modern_log::Log("Network connection established");
                break;
            case ct_hash("file_read"):
                modern_log::Log("File read completed");
                break;
            case ct_hash("data_process"):
                modern_log::Log("Data processing finished");
                break;
            case ct_hash("network_send"):
                modern_log::Log("Network send successful");
                break;
        }
    }
}

// 性能比较演示
void performance_comparison() {
    std::cout << "=== Performance Comparison ===" << std::endl;
    
    // 这里可以添加实际的性能测量代码
    // 在实际项目中会使用PerformanceTimer进行微秒级测量
    
    using namespace modern_utils;
    PerformanceTimer timer("Logging operations");
    
    // 执行一些日志操作
    for (int i = 0; i < 100; ++i) {
        StringBuffer buf(256);
        buf.append("Operation ").append(i).append(" completed");
        modern_log::Log(std::string_view(buf.str()));
    }
}

int main() {
    std::cout << "Logging Optimization Demo" << std::endl;
    
    traditional_logging();
    optimized_logging();
    batch_processing_example();
    performance_comparison();
    
    return 0;
}