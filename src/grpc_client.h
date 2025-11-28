#pragma once

#include <curl/curl.h>
#include <spiffe/types.h>

#include <functional>
#include <future>
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
struct GrpcStatus {
    int code = 0;
    std::string message;

    bool is_ok() const { return code == 0; }
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

    // Disable copy
    GrpcClient(const GrpcClient&) = delete;
    GrpcClient& operator=(const GrpcClient&) = delete;

    // Unary call - returns either response or status
    GrpcResult call(                                //
        const std::string& service,                 //
        const std::string& method,                  //
        const Buffer& request_data,                 //
        const std::vector<GrpcMetadata>& metadata,  //
        const std::chrono::milliseconds timeout     //
    );

    // Server streaming call - returns final status
    GrpcStatus call_stream(                                                //
        const std::string& service,                                        //
        const std::string& method,                                         //
        const Buffer& request_data,                                        //
        const std::function<GrpcStatus(const GrpcResponse&)> on_response,  //
        const std::vector<GrpcMetadata>& metadata,                         //
        const std::shared_future<void> cancelation_token                   //
    );

   private:
    std::string socket_path_;
    CURL* curl_;

    void setup_curl();
    std::string build_url(const std::string& service, const std::string& method);
    struct curl_slist* build_headers(const std::vector<GrpcMetadata>& metadata);
    GrpcStatus extract_grpc_status(CURL* curl);

    // Callback data structure for streaming
    struct StreamCallbackData {
        std::function<GrpcStatus(const GrpcResponse&)> on_response;
        GrpcStatus last_status;  // for user to filling last status, and passing to original call_stream result

        Buffer buffer;
        bool in_message = false;
        uint32_t message_length = 0;
        size_t bytes_read = 0;
    };

    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                                 curl_off_t ulnow);
};

}  // namespace spiffe
