#include <muleunit/test.h>
#include "StructuredDiagnostics.h"
#include "libwebcommon/picojson.h"
#include <wx/filename.h>
#include <iterator>
#include <thread>
#include <vector>
using namespace muleunit;
DECLARE_SIMPLE(StructuredDiagnostics)
TEST(StructuredDiagnostics, DisabledAndEscapedEnvelope) {
    CStructuredDiagnostics sink;
    ASSERT_FALSE(sink.Write("peer", "ban", "warning"));
    const auto name = wxFileName::CreateTempFileName("amule-diag");
    const std::filesystem::path path(std::string(name.utf8_str()));
    ASSERT_TRUE(sink.Configure(path, 4096));
    ASSERT_TRUE(sink.Write("peer", "quoted\"event", "warning", 7, 8, 9));
    sink.Configure({}, 0);
    std::ifstream in(path);
    const std::string data((std::istreambuf_iterator<char>(in)), {});
    ASSERT_TRUE(data.find("\"schema\":\"diag_event_v1\"") != std::string::npos);
    ASSERT_TRUE(data.find("quoted\\\"event") != std::string::npos);
    ASSERT_TRUE(data.find("\"seq\":1") != std::string::npos);
    in.close(); std::filesystem::remove(path);
}
TEST(StructuredDiagnostics, RotationAndConcurrentLines) {
    const auto name = wxFileName::CreateTempFileName("amule-diag");
    const std::filesystem::path path(std::string(name.utf8_str()));
    CStructuredDiagnostics sink; ASSERT_TRUE(sink.Configure(path, 1024));
    std::vector<std::thread> threads;
    for (int n = 0; n < 4; ++n) threads.emplace_back([&sink] {
        for (int i = 0; i < 20; ++i) sink.Write("scheduler", "capacity", "info");
    });
    for (auto &thread : threads) thread.join();
    sink.Configure({}, 0);
    auto previous = path; previous += ".1";
    ASSERT_TRUE(std::filesystem::file_size(path) <= 1024);
    ASSERT_TRUE(std::filesystem::file_size(previous) <= 1024);
    std::ifstream in(path); std::string line;
    while (std::getline(in, line)) {
        picojson::value root;
        ASSERT_TRUE(picojson::parse(root, line).empty());
        ASSERT_TRUE(root.is<picojson::object>());
        ASSERT_TRUE(line.find("\"seq\":") != std::string::npos);
    }
    in.close(); std::filesystem::remove(path); std::filesystem::remove(previous);
}
