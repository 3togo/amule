// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "libwebcommon/JsonWriter.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>

// Typed events intentionally carry no addresses, paths, file names or packet payload.
class CStructuredDiagnostics {
public:
    static CStructuredDiagnostics &Get() { static CStructuredDiagnostics sink; return sink; }
    bool Configure(const std::filesystem::path &path, uint64_t limit) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_enabled.store(false);
        m_file.close(); m_file.clear();
        m_path = path; m_limit = limit; m_sequence = 0;
        if (path.empty() || !limit) return true;
        std::error_code ec;
        m_size = std::filesystem::exists(path, ec) ? std::filesystem::file_size(path, ec) : 0;
        if (ec) return false;
        m_file.open(path, std::ios::binary | std::ios::app);
        m_enabled.store(m_file.is_open());
        return m_enabled.load();
    }
    bool Enabled() const { return m_enabled.load(std::memory_order_relaxed); }
    bool Write(const char *family, const char *event, const char *severity,
               uint64_t code = 0, uint64_t bytes = 0, uint64_t count = 0) {
        if (!Enabled()) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!Enabled()) return false;
        CJsonWriter w;
        w.BeginObject();
        w.Key("schema"); w.ValueString("diag_event_v1");
        w.Key("client"); w.ValueString("amule");
        w.Key("ts"); w.ValueInt(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        w.Key("seq"); w.ValueUInt(++m_sequence);
        w.Key("family"); w.ValueString(family);
        w.Key("event"); w.ValueString(event);
        w.Key("severity"); w.ValueString(severity);
        w.Key("keys"); w.BeginObject(); w.EndObject();
        w.Key("body"); w.BeginObject();
        w.Key("code"); w.ValueUInt(code);
        w.Key("bytes"); w.ValueUInt(bytes);
        w.Key("count"); w.ValueUInt(count);
        w.EndObject(); w.EndObject();
        const std::string line = w.GetBuffer() + "\n";
        if (line.size() > m_limit) return false;
        if (m_size > m_limit - line.size()) {
            m_file.close();
            auto previous = m_path; previous += ".1";
            std::error_code ec;
            std::filesystem::remove(previous, ec);
            if (!ec) std::filesystem::rename(m_path, previous, ec);
            if (ec) { m_enabled.store(false); return false; }
            m_file.open(m_path, std::ios::binary | std::ios::trunc);
            m_size = 0;
        }
        m_file.write(line.data(), line.size()); m_file.flush();
        if (!m_file) { m_enabled.store(false); return false; }
        m_size += line.size();
        return true;
    }
private:
    std::mutex m_mutex;
    std::atomic<bool> m_enabled{false};
    std::filesystem::path m_path;
    std::ofstream m_file;
    uint64_t m_limit = 0, m_size = 0, m_sequence = 0;
};
