#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <spiffe/types.h>

namespace spiffe {

struct Tlv {
    uint8_t tag;
    Buffer value;

    Tlv() : tag(0) {}
    Tlv(uint8_t t, const uint8_t* data, size_t len) : tag(t), value(data, data + len) {}
};

struct TlvResult {
    bool valid;
    size_t consumed;
    Tlv tlv;

    TlvResult() : valid(false), consumed(0) {}
    TlvResult(size_t cons, const Tlv& t) : valid(true), consumed(cons), tlv(t) {}
};

struct SplitCertResult {
    bool valid;
    size_t consumed;
    Buffer cert;

    SplitCertResult() : valid(false), consumed(0) {}
    SplitCertResult(size_t cons, const Buffer& c) : valid(true), consumed(cons), cert(c) {}
};

class CertificateIter {
   private:
    Buffer der_data;
    size_t current_pos;
    bool error_occurred;

   public:
    explicit CertificateIter(const Buffer& data);
    explicit CertificateIter(const uint8_t* data, size_t size);
    explicit CertificateIter(const std::string& data);

    bool has_next() const;
    bool has_error() const;
    Buffer next();
    std::vector<Buffer> collect();
    size_t count() const;
};

// Function declarations
TlvResult read_der_tlv(const uint8_t* der, size_t size);
SplitCertResult split_cert(const uint8_t* raw, size_t size);

// Convenience functions
std::vector<Buffer> extract_all_certificates(const Buffer& der);
std::vector<Buffer> extract_all_certificates(const uint8_t* data, size_t size);
std::vector<Buffer> extract_all_certificates(const std::string& der);

}  // namespace spiffe
