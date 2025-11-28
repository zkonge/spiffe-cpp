#pragma once

#include <spiffe/types.h>

namespace spiffe {

// gRPC message framing utilities
class GrpcFraming {
   public:
    // Pack protobuf message with gRPC framing (5-byte header + message)
    static Buffer pack_message(const Buffer& message);

    // Unpack gRPC message (remove 5-byte header)
    // user must ensure grpc_data contains a complete message
    static bool unpack_message(const Buffer& grpc_data, Buffer& message);

    // Check if we have a complete message in buffer
    static bool has_complete_message(const Buffer& buffer, size_t& message_size);
};

}  // namespace spiffe
