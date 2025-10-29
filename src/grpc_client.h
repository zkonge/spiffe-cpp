#pragma once

#include <curl/curl.h>
#include <spiffe/types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace spiffe {

struct GrpcMetadata {
    std::string key;
    std::string value;
};

// Pure response data (without status information)
class GrpcResponse {
   public:
    Buffer data;
    std::vector<GrpcMetadata> metadata;

    bool has_data() const { return !data.empty(); }
};

// gRPC status information
class GrpcStatus {
   public:
    int code = 0;                        // gRPC status code (0 = OK)
    std::string message;                 // gRPC status message
    std::vector<GrpcMetadata> metadata;  // Status metadata

    bool is_ok() const { return code == 0; }
    std::string error_message() const {
        if (code == 0) return "";
        std::string msg = std::to_string(code);
        if (!message.empty()) {
            msg += ": " + message;
        }
        return msg;
    }
};

// Result type for unary calls
struct GrpcResult {
    bool has_response;
    GrpcResponse response;
    GrpcStatus status;

    GrpcResult(const GrpcResponse& resp) : has_response(true), response(resp) {}
    GrpcResult(const GrpcStatus& stat) : has_response(false), status(stat) {}
};

class GrpcClient {
   public:
    GrpcClient(const std::string& socket_path);
    ~GrpcClient();

    // Unary call - returns either response or status
    GrpcResult call(                                    //
        const std::string& service,                     //
        const std::string& method,                      //
        const Buffer& request_data,                     //
        const std::vector<GrpcMetadata>& metadata = {}  //
    );

    // Server streaming call - returns final status
    GrpcStatus call_stream(                                          //
        const std::string& service,                                  //
        const std::string& method,                                   //
        const Buffer& request_data,                                  //
        std::function<GrpcStatus(const GrpcResponse&)> on_response,  //
        const std::vector<GrpcMetadata>& metadata = {}               //
    );

    // Server streaming call with cancellation support - returns final status
    GrpcStatus call_stream(                                          //
        const std::string& service,                                  //
        const std::string& method,                                   //
        const Buffer& request_data,                                  //
        std::function<GrpcStatus(const GrpcResponse&)> on_response,  //
        const std::vector<GrpcMetadata>& metadata,                   //
        const CancelContext* cancel                                  //
    );

   private:
    std::string socket_path_;
    CURL* curl_;
    mutable std::mutex curl_mutex_;  // Protect curl handle from concurrent access

    void setup_curl();
    std::string build_url(const std::string& service, const std::string& method);
    struct curl_slist* build_headers(const std::vector<GrpcMetadata>& metadata);
    GrpcStatus extract_grpc_status(CURL* curl);

    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    static int xferinfo_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                                 curl_off_t ulnow);
};

}  // namespace spiffe
