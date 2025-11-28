#pragma once

#include <spiffe/types.h>

#include <cstdint>
#include <string>
#include <vector>

namespace spiffe {

// TLV
struct Tlv {
    uint8_t tag;
    Buffer value;

    Tlv() : tag(0) {}
    explicit Tlv(uint8_t t, size_t len, const uint8_t* data) : tag(t), value(data, data + len) {}
};

struct TlvResult {
    bool valid;
    size_t tlv_len;
    Tlv tlv;

    TlvResult() : valid(false), tlv_len(0) {}
    explicit TlvResult(size_t len, const Tlv& t) : valid(true), tlv_len(len), tlv(t) {}
};

TlvResult read_der_tlv(const uint8_t* der, size_t size);

// Certificate Iterator
class CertificateIter {
   private:
    Buffer der_data;
    size_t current_pos;
    bool error_occurred;

   public:
    explicit CertificateIter(const uint8_t* data, size_t size);

    bool has_next() const;
    bool has_error() const;
    Buffer next();
    std::vector<Buffer> collect();
};

// Convenience functions
std::vector<Buffer> extract_all_certificates(const uint8_t* data, size_t size);
std::vector<Buffer> extract_all_certificates(const Buffer& der);
std::vector<Buffer> extract_all_certificates(const std::string& der);

}  // namespace spiffe
