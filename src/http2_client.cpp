#include "http2_client.h"

#include <arpa/inet.h>

#include <cstring>

namespace spiffe {

const size_t GRPC_FRAME_HEADER_LEN = 1 + sizeof(uint32_t);

Buffer GrpcFraming::pack_message(const Buffer& message) {
    Buffer result;
    result.resize(GRPC_FRAME_HEADER_LEN + message.size());

    // gRPC message format:
    // 1 byte: compressed flag (0 = not compressed)
    result[0] = 0;

    // 4 bytes: message length (big-endian)
    // TODO: Handle messages larger than 4GB if needed
    const uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    std::memcpy(&result[1], &length, sizeof(length));

    // Message data
    if (!message.empty()) {
        std::memcpy(&result[GRPC_FRAME_HEADER_LEN], message.data(), message.size());
    }

    return result;
}

bool GrpcFraming::unpack_message(const Buffer& grpc_data, Buffer& message) {
    if (grpc_data.size() < GRPC_FRAME_HEADER_LEN) {
        return false;
    }

    // Compressed messages are not supported
    if (grpc_data[0] != 0) {
        return false;
    }

    // Skip compression flag (1 byte)
    // Read message length (4 bytes, big-endian)
    uint32_t length;
    std::memcpy(&length, &grpc_data[1], sizeof(length));
    length = ntohl(length);

    if (grpc_data.size() < GRPC_FRAME_HEADER_LEN + length) {
        return false;
    }

    // Extract message
    message.assign(grpc_data.begin() + GRPC_FRAME_HEADER_LEN, grpc_data.begin() + GRPC_FRAME_HEADER_LEN + length);
    return true;
}

bool GrpcFraming::has_complete_message(const Buffer& buffer, size_t& message_size) {
    if (buffer.size() < GRPC_FRAME_HEADER_LEN) {
        return false;
    }

    // Read message length from header
    uint32_t length;
    std::memcpy(&length, &buffer[1], sizeof(length));
    length = ntohl(length);

    message_size = GRPC_FRAME_HEADER_LEN + length;
    return buffer.size() >= message_size;
}

}  // namespace spiffe
