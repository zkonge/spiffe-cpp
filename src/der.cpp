#include "der.h"

namespace spiffe {

TlvResult read_der_tlv(const uint8_t* der, size_t size) {
    if (size < 2) {
        return TlvResult();
    }

    uint8_t tag = der[0];
    uint8_t first_len_byte = der[1];
    const uint8_t* rem = der + 2;
    size_t rem_size = size - 2;

    // short form length
    if ((first_len_byte & 0x80) == 0) {
        if (rem_size < first_len_byte) {
            return TlvResult();
        }

        size_t value_len = first_len_byte;
        size_t total_consumed = 2 + value_len;

        return TlvResult(total_consumed, Tlv(tag, rem, value_len));
    }

    // long form length
    uint8_t len_len = first_len_byte & 0x7f;
    if (rem_size < len_len) {
        return TlvResult();
    }

    const uint8_t* len_bytes = rem;
    rem += len_len;
    rem_size -= len_len;

    size_t len = 0;
    switch (len_len) {
        case 1:
            len = len_bytes[0];
            break;
        case 2:
            // DER uses big-endian encoding
            len = (static_cast<size_t>(len_bytes[0]) << 8) | len_bytes[1];
            break;
        case 3:
            len = (static_cast<size_t>(len_bytes[0]) << 16) | (static_cast<size_t>(len_bytes[1]) << 8) | len_bytes[2];
            break;
        default:
            return TlvResult();
    }

    if (rem_size < len) {
        return TlvResult();
    }

    size_t total_consumed = 2 + len_len + len;
    return TlvResult(total_consumed, Tlv(tag, rem, len));
}

SplitCertResult split_cert(const uint8_t* raw, size_t size) {
    TlvResult result = read_der_tlv(raw, size);
    if (!result.valid || result.tlv.tag != 0x30) {
        return SplitCertResult();
    }

    std::string cert(reinterpret_cast<const char*>(raw), result.consumed);
    return SplitCertResult(result.consumed, cert);
}

CertificateIter::CertificateIter(const std::vector<uint8_t>& data)
    : der_data(data), current_pos(0), error_occurred(false) {}

CertificateIter::CertificateIter(const uint8_t* data, size_t size)
    : der_data(data, data + size), current_pos(0), error_occurred(false) {}

CertificateIter::CertificateIter(const std::string& data)
    : der_data(reinterpret_cast<const uint8_t*>(data.data()),
               reinterpret_cast<const uint8_t*>(data.data()) + data.size()),
      current_pos(0),
      error_occurred(false) {}

bool CertificateIter::has_next() const { return current_pos < der_data.size() && !error_occurred; }

bool CertificateIter::has_error() const { return error_occurred; }

std::string CertificateIter::next() {
    if (current_pos >= der_data.size() || error_occurred) {
        return std::string();
    }

    const uint8_t* current_data = der_data.data() + current_pos;
    size_t remaining_size = der_data.size() - current_pos;

    SplitCertResult result = split_cert(current_data, remaining_size);
    if (!result.valid) {
        error_occurred = true;
        return std::string();
    }

    current_pos += result.consumed;
    return result.cert;
}

std::vector<std::string> CertificateIter::collect() {
    std::vector<std::string> certs;

    while (has_next()) {
        std::string cert = next();
        if (cert.empty()) {
            break;
        }
        certs.push_back(cert);
    }

    return certs;
}

size_t CertificateIter::count() const {
    if (error_occurred) {
        return 0;
    }

    size_t cert_count = 0;
    size_t pos = current_pos;

    while (pos < der_data.size()) {
        const uint8_t* data = der_data.data() + pos;
        size_t remaining = der_data.size() - pos;

        SplitCertResult result = split_cert(data, remaining);
        if (!result.valid) {
            break;
        }

        cert_count++;
        pos += result.consumed;
    }

    return cert_count;
}

CertificateIter split_certificates(const std::vector<uint8_t>& der) { return CertificateIter(der); }

CertificateIter split_certificates(const uint8_t* data, size_t size) { return CertificateIter(data, size); }

CertificateIter split_certificates(const std::string& der) { return CertificateIter(der); }

std::vector<std::string> extract_all_certificates(const std::vector<uint8_t>& der) {
    return CertificateIter(der).collect();
}

std::vector<std::string> extract_all_certificates(const uint8_t* data, size_t size) {
    return CertificateIter(data, size).collect();
}

std::vector<std::string> extract_all_certificates(const std::string& der) { return CertificateIter(der).collect(); }

}  // namespace spiffe