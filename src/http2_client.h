#pragma once

#include <spiffe/types.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace spiffe {

class Http2Frame {
   public:
    static Buffer create_data_frame(const Buffer& payload, bool end_stream = true);
    static bool parse_data_frame(const Buffer& frame_data, Buffer& payload);
};

// gRPC message framing utilities
class GrpcFraming {
   public:
    // Pack protobuf message with gRPC framing (5-byte header + message)
    static Buffer pack_message(const Buffer& message);

    // Unpack gRPC message (remove 5-byte header)
    static bool unpack_message(const Buffer& grpc_data, Buffer& message);

    // Check if we have a complete message in buffer
    static bool has_complete_message(const Buffer& buffer, size_t& message_size);
};

}  // namespace spiffe
