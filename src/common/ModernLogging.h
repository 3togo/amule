#pragma once

#include <string_view>
#include <source_location>

// 兼容层
namespace modern_log {
    #ifdef USE_CPP20
    void Log(std::string_view msg, 
             bool critical = false,
             std::source_location loc = std::source_location::current());
    #else
    void Log(const wxString& msg, bool critical = false);
    #endif
}