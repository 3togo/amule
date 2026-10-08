#include <muleunit/test.h>
#include "webapi/QBitCompat.h"
using namespace muleunit;
DECLARE_SIMPLE(QBitCompat)
TEST(QBitCompat, StrictFormDecoding) {
    qbit_compat::Fields fields;
    ASSERT_TRUE(qbit_compat::Form("urls=ed2k%3A%2F%2F%7Cfile%7Ca%2Bb&category=TV+Shows", fields));
    ASSERT_EQUALS(std::string("TV Shows"), fields["category"]);
    ASSERT_EQUALS(std::string("ed2k://|file|a+b"), fields["urls"]);
    ASSERT_FALSE(qbit_compat::Form("hashes=%00", fields));
    ASSERT_FALSE(qbit_compat::Form("hashes=%gg", fields));
    ASSERT_FALSE(qbit_compat::Form("hashes=a&hashes=b", fields));
}
TEST(QBitCompat, HashesAreBoundedAndNormalized) {
    std::vector<std::string> hashes;
    ASSERT_TRUE(qbit_compat::Hashes(std::string(32, 'A'), hashes));
    ASSERT_EQUALS(std::string(32, 'a'), hashes[0]);
    ASSERT_FALSE(qbit_compat::Hashes(std::string(32, 'a') + "|", hashes));
    ASSERT_FALSE(qbit_compat::Hashes("all", hashes));
}
TEST(QBitCompat, PreservesNativeFailureAndCompletionMeaning) {
    ASSERT_EQUALS(std::string("error"), std::string(qbit_compat::State("insufficient_disk", 0)));
    ASSERT_EQUALS(std::string("checkingDL"), std::string(qbit_compat::State("hashing", 0)));
    ASSERT_EQUALS(std::string("stoppedUP"), std::string(qbit_compat::State("completed", 0)));
    ASSERT_EQUALS(std::string("stoppedDL"), std::string(qbit_compat::State("stopped", 0)));
    ASSERT_EQUALS(std::string("stalledDL"), std::string(qbit_compat::State("downloading", 0)));
    ASSERT_EQUALS(std::string("downloading"), std::string(qbit_compat::State("downloading", 100)));
}
TEST(QBitCompat, MultipartFieldsAndRejectsTorrentFiles) {
    qbit_compat::Fields fields;
    const std::string form = "--abc\r\nContent-Disposition: form-data; name=\"urls\"\r\n\r\ned2k://|file|a\r\n--abc--\r\n";
    ASSERT_TRUE(qbit_compat::Multipart(form, "abc", fields));
    ASSERT_EQUALS(std::string("ed2k://|file|a"), fields["urls"]);
    ASSERT_FALSE(qbit_compat::Multipart("--abc\r\nContent-Disposition: form-data; name=\"torrents\"; filename=\"x.torrent\"\r\n\r\nx\r\n--abc--\r\n", "abc", fields));
}
