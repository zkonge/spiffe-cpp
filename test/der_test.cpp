#include "der.h"
#include <gtest/gtest.h>
#include <vector>

namespace spiffe {

TEST(DerTest, ReadTlvSimple) {
    // Tag: 0x30 (Sequence), Length: 0x02, Value: 0x01, 0x02
    std::vector<uint8_t> data = {0x30, 0x02, 0x01, 0x02};
    TlvResult result = read_der_tlv(data.data(), data.size());

    ASSERT_TRUE(result.valid);
    EXPECT_EQ(result.tlv.tag, 0x30);
    EXPECT_EQ(result.tlv.value.size(), 2);
    EXPECT_EQ(result.tlv.value[0], 0x01);
    EXPECT_EQ(result.tlv.value[1], 0x02);
    EXPECT_EQ(result.tlv_len, 4);
}

TEST(DerTest, ReadTlvLongLength) {
    // Tag: 0x04 (Octet String)
    // Length: 0x81 0x80 (Long form, 128 bytes)
    std::vector<uint8_t> data;
    data.push_back(0x04);
    data.push_back(0x81);
    data.push_back(0x80);
    for (int i = 0; i < 128; ++i) {
        data.push_back(0xAA);
    }

    TlvResult result = read_der_tlv(data.data(), data.size());

    ASSERT_TRUE(result.valid);
    EXPECT_EQ(result.tlv.tag, 0x04);
    EXPECT_EQ(result.tlv.value.size(), 128);
    EXPECT_EQ(result.tlv_len, 131); // 1 tag + 2 length + 128 value
}

TEST(DerTest, ReadTlvIncomplete) {
    std::vector<uint8_t> data = {0x30, 0x05, 0x01}; // Length says 5, but only 1 byte provided
    TlvResult result = read_der_tlv(data.data(), data.size());
    EXPECT_FALSE(result.valid);
}

TEST(DerTest, ExtractCertificates) {
    // Construct a fake certificate chain (Sequence of Sequences)
    // Cert 1: Sequence (0x30), Len 2, Val {0x01, 0x01}
    // Cert 2: Sequence (0x30), Len 2, Val {0x02, 0x02}
    std::vector<uint8_t> data = {
        0x30, 0x02, 0x01, 0x01,
        0x30, 0x02, 0x02, 0x02
    };

    auto certs = extract_all_certificates(data.data(), data.size());
    ASSERT_EQ(certs.size(), 2);
    
    EXPECT_EQ(certs[0].size(), 4); // Full TLV
    EXPECT_EQ(certs[0][0], 0x30);
    
    EXPECT_EQ(certs[1].size(), 4);
    EXPECT_EQ(certs[1][2], 0x02);
}

TEST(DerTest, ExtractCertificatesGarbage) {
    std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF};
    auto certs = extract_all_certificates(data.data(), data.size());
    EXPECT_EQ(certs.size(), 0);
}

} // namespace spiffe
