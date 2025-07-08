#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace spiffe {

class Http2Frame {
   public:
    static std::vector<uint8_t> create_data_frame(const std::vector<uint8_t>& payload, bool end_stream = true);
    static bool parse_data_frame(const std::vector<uint8_t>& frame_data, std::vector<uint8_t>& payload);
};

// gRPC message framing utilities
class GrpcFraming {
   public:
    // Pack protobuf message with gRPC framing (5-byte header + message)
    static std::vector<uint8_t> pack_message(const std::vector<uint8_t>& message);

    // Unpack gRPC message (remove 5-byte header)
    static bool unpack_message(const std::vector<uint8_t>& grpc_data, std::vector<uint8_t>& message);

    // Check if we have a complete message in buffer
    static bool has_complete_message(const std::vector<uint8_t>& buffer, size_t& message_size);
};

}  // namespace spiffe
