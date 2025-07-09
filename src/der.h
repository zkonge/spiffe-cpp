#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace spiffe {

struct Tlv {
    uint8_t tag;
    std::string value;

    Tlv() : tag(0) {}
    Tlv(uint8_t t, const uint8_t* data, size_t len) : tag(t), value(reinterpret_cast<const char*>(data), len) {}
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
    std::string cert;

    SplitCertResult() : valid(false), consumed(0) {}
    SplitCertResult(size_t cons, const std::string& c) : valid(true), consumed(cons), cert(c) {}
};

class CertificateIter {
   private:
    std::vector<uint8_t> der_data;
    size_t current_pos;
    bool error_occurred;

   public:
    explicit CertificateIter(const std::vector<uint8_t>& data);
    explicit CertificateIter(const uint8_t* data, size_t size);
    explicit CertificateIter(const std::string& data);

    bool has_next() const;
    bool has_error() const;
    std::string next();
    std::vector<std::string> collect();
    size_t count() const;
};

// Function declarations
TlvResult read_der_tlv(const uint8_t* der, size_t size);
SplitCertResult split_cert(const uint8_t* raw, size_t size);

// Factory functions
CertificateIter split_certificates(const std::vector<uint8_t>& der);
CertificateIter split_certificates(const uint8_t* data, size_t size);
CertificateIter split_certificates(const std::string& der);

// Convenience functions
std::vector<std::string> extract_all_certificates(const std::vector<uint8_t>& der);
std::vector<std::string> extract_all_certificates(const uint8_t* data, size_t size);
std::vector<std::string> extract_all_certificates(const std::string& der);

}  // namespace spiffe
