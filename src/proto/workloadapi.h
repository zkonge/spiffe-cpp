#pragma once

// DO NOT EXPOSE ME, OR YOU WILL POLLUTE NAMESPACE
#include <SimpleProtos.h>

#include <cstring>  // SimpleProtos.h requires stdlib.h

namespace spiffe {

// ProtoMessage is the polyfill for protobuf 3
struct ProtoMapItem {
    FIELDS(                     //
        FIELD_BUFFER(1, key)    // buffer
        FIELD_BUFFER(2, value)  // buffer
    )

    ADD_FIELD_OPTIONAL(std::string, key);
    ADD_FIELD_OPTIONAL(std::string, value);
};

// X.509 SVIDs

struct ProtoX509Svid {
    FIELDS(                             //
        FIELD_BUFFER(1, spiffe_id)      // string
        FIELD_BUFFER(2, x509_svid)      // bytes
        FIELD_BUFFER(3, x509_svid_key)  // bytes
        FIELD_BUFFER(4, bundle)         // bytes
        FIELD_BUFFER(5, hint)           // string
    )

    ADD_FIELD_OPTIONAL(std::string, spiffe_id);
    ADD_FIELD_OPTIONAL(std::string, x509_svid);
    ADD_FIELD_OPTIONAL(std::string, x509_svid_key);
    ADD_FIELD_OPTIONAL(std::string, bundle);
    ADD_FIELD_OPTIONAL(std::string, hint);
};

struct ProtoX509SvidRequest {
    FIELDS(do {} while (0);)
};

struct ProtoX509SvidResponse {
    FIELDS(                                           //
        REPEATED_FIELD_MESSAGE(1, svids)              // repeated ProtoX509Svid
        REPEATED_FIELD_BUFFER(2, crl)                 // repeated bytes
        REPEATED_FIELD_MESSAGE(3, federated_bundles)  // map<string, bytes>
    )

    ADD_FIELD_OPTIONAL(std::vector<ProtoX509Svid>, svids);
    ADD_FIELD_OPTIONAL(std::vector<std::string>, crl);
    ADD_FIELD_OPTIONAL(std::vector<ProtoMapItem>, federated_bundles);
};

struct ProtoX509BundlesRequest {
    FIELDS(do {} while (0);)
};

struct ProtoX509BundlesResponse {
    FIELDS(                                 //
        REPEATED_FIELD_BUFFER(1, crl)       // repeated bytes
        REPEATED_FIELD_MESSAGE(2, bundles)  // map<string, bytes>
    )

    ADD_FIELD_OPTIONAL(std::vector<std::string>, crl);
    ADD_FIELD_OPTIONAL(std::vector<ProtoMapItem>, bundles);
};

// JWT SVIDs

struct ProtoJwtSvid {
    FIELDS(                         //
        FIELD_BUFFER(1, spiffe_id)  // string
        FIELD_BUFFER(2, svid)       // string
        FIELD_BUFFER(3, hint)       // string
    )

    ADD_FIELD_OPTIONAL(std::string, spiffe_id);
    ADD_FIELD_OPTIONAL(std::string, svid);
    ADD_FIELD_OPTIONAL(std::string, hint);
};

struct ProtoJwtSvidRequest {
    FIELDS(                                 //
        REPEATED_FIELD_BUFFER(1, audience)  // repeated string
        FIELD_BUFFER(2, spiffe_id)          // string
    )

    ADD_FIELD_OPTIONAL(std::vector<std::string>, audience);
    ADD_FIELD_OPTIONAL(std::string, spiffe_id);
};

struct ProtoJwtSvidResponse {
    FIELDS(                               //
        REPEATED_FIELD_MESSAGE(1, svids)  // repeated ProtoJwtSvid
    )

    ADD_FIELD_OPTIONAL(std::vector<ProtoJwtSvid>, svids);
};

struct ProtoJwtBundlesRequest {
    FIELDS(do {} while (0);)
};

struct ProtoJwtBundlesResponse {
    FIELDS(                                 //
        REPEATED_FIELD_MESSAGE(1, bundles)  // repeated bytes
    )

    ADD_FIELD_OPTIONAL(std::vector<ProtoMapItem>, bundles);
};

template <typename ProtoMessage>
std::vector<std::uint8_t> encode_proto_message(ProtoMessage& message) {
    auto writer = message.serialize();
    std::vector<std::uint8_t> buffer(writer->m_buf, writer->m_buf + writer->m_pos);
    delete writer;

    return buffer;
}

template <typename ProtoMessage>
bool decode_proto_message(std::vector<std::uint8_t>& buffer, ProtoMessage& message) {
    return message.deserialize(buffer.data(), buffer.size());
}

}  // namespace spiffe
