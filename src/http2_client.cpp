#include "http2_client.h"

#include <arpa/inet.h>

#include <cstring>

namespace spiffe {

Buffer Http2Frame::create_data_frame(const Buffer& payload, bool end_stream) {
    // For simplicity, we'll let curl handle HTTP/2 framing
    // This is just a placeholder implementation
    return payload;
}

bool Http2Frame::parse_data_frame(const Buffer& frame_data, Buffer& payload) {
    // For simplicity, we'll let curl handle HTTP/2 framing
    payload = frame_data;
    return true;
}

Buffer GrpcFraming::pack_message(const Buffer& message) {
    Buffer result;
    result.reserve(5 + message.size());

    // gRPC message format:
    // 1 byte: compressed flag (0 = not compressed)
    result.push_back(0);

    // 4 bytes: message length (big-endian)
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    const uint8_t* length_bytes = reinterpret_cast<const uint8_t*>(&length);
    result.insert(result.end(), length_bytes, length_bytes + 4);

    // Message data
    result.insert(result.end(), message.begin(), message.end());

    return result;
}

bool GrpcFraming::unpack_message(const Buffer& grpc_data, Buffer& message) {
    if (grpc_data.size() < 5) {
        return false;
    }

    // Skip compression flag (1 byte)
    // Read message length (4 bytes, big-endian)
    uint32_t length;
    std::memcpy(&length, &grpc_data[1], 4);
    length = ntohl(length);

    if (grpc_data.size() < 5 + length) {
        return false;
    }

    // Extract message
    message.assign(grpc_data.begin() + 5, grpc_data.begin() + 5 + length);
    return true;
}

bool GrpcFraming::has_complete_message(const Buffer& buffer, size_t& message_size) {
    if (buffer.size() < 5) {
        return false;
    }

    // Read message length from header
    uint32_t length;
    std::memcpy(&length, &buffer[1], 4);
    length = ntohl(length);

    message_size = 5 + length;
    return buffer.size() >= message_size;
}

}  // namespace spiffe
