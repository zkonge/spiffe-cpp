#include "der.h"

namespace spiffe {

TlvResult read_der_tlv(const uint8_t* der, size_t size) {
    // TODO: audit me
    // This is a critical function for security; make sure all boundary conditions are handled correctly.
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
        size_t total_len = 2 + value_len;

        return TlvResult(total_len, Tlv(tag, value_len, rem));
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
            // Is it possible to have a certificate that lengths longer than 2**23 bytes?
            return TlvResult();
    }

    if (rem_size < len) {
        return TlvResult();
    }

    size_t total_len = 2 + len_len + len;
    return TlvResult(total_len, Tlv(tag, len, rem));
}

CertificateIter::CertificateIter(const uint8_t* data, size_t size)
    : der_data(data, data + size), current_pos(0), error_occurred(false) {}

bool CertificateIter::has_next() const { return current_pos < der_data.size() && !error_occurred; }

bool CertificateIter::has_error() const { return error_occurred; }

Buffer CertificateIter::next() {
    if (current_pos >= der_data.size() || error_occurred) {
        return {};
    }

    // boundary safety: we have already checked boundaries in read_der_tlv
    const uint8_t* current_data = der_data.data() + current_pos;
    size_t remaining_size = der_data.size() - current_pos;

    TlvResult result = read_der_tlv(current_data, remaining_size);
    if (!result.valid || result.tlv.tag != 0x30) {
        error_occurred = true;
        return {};
    }

    Buffer cert(current_data, current_data + result.tlv_len);
    current_pos += result.tlv_len;
    return cert;
}

std::vector<Buffer> CertificateIter::collect() {
    std::vector<Buffer> certs;

    while (has_next()) {
        Buffer cert = next();
        if (cert.empty()) {
            break;
        }
        certs.push_back(cert);
    }

    return certs;
}

std::vector<Buffer> extract_all_certificates(const uint8_t* data, size_t size) {
    return CertificateIter(data, size).collect();
}

std::vector<Buffer> extract_all_certificates(const Buffer& der) {
    return CertificateIter(der.data(), der.size()).collect();
}

std::vector<Buffer> extract_all_certificates(const std::string& der) {
    return CertificateIter(reinterpret_cast<const uint8_t*>(der.data()), der.size()).collect();
}

}  // namespace spiffe
