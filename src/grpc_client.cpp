#include "grpc_client.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "http2_client.h"

#if LIBCURL_VERSION_NUM < 0x073100  // 7.49.0
#error \
    "cURL version is too old. Minimum required version is 7.49.0, which introduced CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE."
#endif

#ifndef CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE
#error "CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE not defined. Please make sure cURL is built with HTTP/2 support."
#endif

namespace spiffe {

struct ResponseData {
    Buffer data;
    std::vector<GrpcMetadata> headers;
    long response_code = 0;
};

int GrpcClient::progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                                  curl_off_t ulnow) {
    const std::shared_future<void>* cancelation_token = static_cast<const std::shared_future<void>*>(clientp);
    if (cancelation_token && cancelation_token->valid()) {
        if (cancelation_token->wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
            return 0;
        }
    }
    return 1;  // Abort transfer
}

size_t GrpcClient::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    ResponseData* response = static_cast<ResponseData*>(userp);

    const uint8_t* data = static_cast<const uint8_t*>(contents);
    response->data.insert(response->data.end(), data, data + total_size);

    return total_size;
}

size_t GrpcClient::stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    StreamCallbackData* stream_data = static_cast<StreamCallbackData*>(userp);

    const uint8_t* data = static_cast<const uint8_t*>(contents);
    stream_data->buffer.insert(stream_data->buffer.end(), data, data + total_size);

    while (true) {
        size_t message_size;
        if (!GrpcFraming::has_complete_message(stream_data->buffer, message_size)) {
            break;  // No complete message yet
        }

        // Extract the complete message
        Buffer grpc_message(stream_data->buffer.begin(), stream_data->buffer.begin() + message_size);

        // Remove processed message from buffer
        stream_data->buffer.erase(stream_data->buffer.begin(), stream_data->buffer.begin() + message_size);

        // Unpack the message
        Buffer proto_message;
        if (GrpcFraming::unpack_message(grpc_message, proto_message)) {
            GrpcResponse response;
            response.data = proto_message;
            stream_data->last_status = stream_data->on_response(response);
            if (!stream_data->last_status.is_ok()) {
                // If the callback returns an error, we can stop processing
                return 0;  // Stop further processing
            }
        } else {
            // If unpacking fails, we can log or handle the error
            stream_data->last_status = GrpcStatus{.code = 13, .message = "Failed to unpack gRPC message"};
            // Cancel curl operation
            return 0;
        }
    }

    return total_size;
}

GrpcClient::GrpcClient(const std::string& socket_path) : socket_path_(socket_path), curl_(nullptr) {
    curl_ = curl_easy_init();
    setup_curl();
}

GrpcClient::~GrpcClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

void GrpcClient::setup_curl() {
    if (!curl_) return;

    // Force HTTP/2 without upgrade (direct HTTP/2)
    curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);

    // Unix Domain Socket specific settings
    curl_easy_setopt(curl_, CURLOPT_UNIX_SOCKET_PATH, socket_path_.c_str());

    // For non-HTTPS connections, forbid connection reuse to avoid cURL HTTP/2 framing issues
    // Not used currently. We create one GrpcClient per call.
    // curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 1L);

    // Enable verbose output for debugging
    // curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);

    // Set timeout
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 0L);

    // Enable TCP keepalive
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);

    // Disable Expect: 100-continue header
    curl_easy_setopt(curl_, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);

    // Prevent cURL from installing signal handlers
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
}

std::string GrpcClient::build_url(const std::string& service, const std::string& method) {
    std::ostringstream url;

    // For Unix Domain Socket, use a dummy host since the socket path is set separately
    url << "http://-/" << service << "/" << method;

    return url.str();
}

struct curl_slist* GrpcClient::build_headers(const std::vector<GrpcMetadata>& metadata) {
    struct curl_slist* headers = nullptr;

    // Required gRPC headers
    headers = curl_slist_append(headers, "content-type: application/grpc+proto");
    headers = curl_slist_append(headers, "te: trailers");
    headers = curl_slist_append(headers, "grpc-accept-encoding: identity");

    // Add custom metadata
    for (const auto& meta : metadata) {
        std::string header = meta.key + ": " + meta.value;
        headers = curl_slist_append(headers, header.c_str());
    }

    return headers;
}

GrpcResult GrpcClient::call(                    //
    const std::string& service,                 //
    const std::string& method,                  //
    const Buffer& request_data,                 //
    const std::vector<GrpcMetadata>& metadata,  //
    const std::chrono::milliseconds timeout     //
) {
    if (!curl_) {
        return GrpcResult(GrpcStatus{.code = 13, .message = "cURL not initialized"});
    }

    // Prepare gRPC framed message
    Buffer grpc_message = GrpcFraming::pack_message(request_data);

    // Setup request
    std::string url = build_url(service, method);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, grpc_message.data());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, grpc_message.size());

    // Setup headers
    struct curl_slist* headers = build_headers(metadata);
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    // Setup response callback
    ResponseData response_data;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_data);

    // Unary call need a timeout
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));

    // Perform the request
    CURLcode res = curl_easy_perform(curl_);

    // Cleanup headers
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        return GrpcResult(GrpcStatus{.code = 13, .message = curl_easy_strerror(res)});
    }

    // Get response code
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_data.response_code);

    // Check if HTTP response is successful
    if (response_data.response_code != 200) {
        return GrpcResult(
            GrpcStatus{.code = 13, .message = "HTTP error: " + std::to_string(response_data.response_code)});
    }

    // Extract gRPC status
    GrpcStatus grpc_status = extract_grpc_status(curl_);

    // If gRPC status is not OK, return status
    if (!grpc_status.is_ok()) {
        return GrpcResult(grpc_status);
    }

    // If successful, return response data
    GrpcResponse response;

    // Unpack gRPC message
    Buffer proto_message;
    if (GrpcFraming::unpack_message(response_data.data, proto_message)) {
        response.data = proto_message;
    } else {
        return GrpcResult(GrpcStatus{.code = 13, .message = "Failed to unpack gRPC message"});
    }

    return GrpcResult(response);
}

GrpcStatus GrpcClient::call_stream(                                    //
    const std::string& service,                                        //
    const std::string& method,                                         //
    const Buffer& request_data,                                        //
    const std::function<GrpcStatus(const GrpcResponse&)> on_response,  //
    const std::vector<GrpcMetadata>& metadata,                         //
    const std::shared_future<void> cancelation_token                   //
) {
    if (!curl_) {
        return GrpcStatus{.code = 13, .message = "cURL not initialized"};
    }

    // Prepare gRPC framed message
    Buffer grpc_message = GrpcFraming::pack_message(request_data);

    // Setup request
    std::string url = build_url(service, method);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, grpc_message.data());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, grpc_message.size());

    // Setup headers
    struct curl_slist* headers = build_headers(metadata);
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    // Setup streaming callback
    StreamCallbackData stream_data;
    stream_data.on_response = on_response;

    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, stream_write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &stream_data);

    // Setup cancellation
    curl_easy_setopt(curl_, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl_, CURLOPT_XFERINFODATA, &cancelation_token);
    curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 0L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl_);

    // Cleanup headers
    curl_slist_free_all(headers);

    if (!stream_data.last_status.is_ok()) {
        // If the last status is not OK, return it
        return stream_data.last_status;
    }

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK) {
            return GrpcStatus{.code = 1, .message = "user cancelled"};
        }
        return GrpcStatus{.code = 13, .message = curl_easy_strerror(res)};
    }

    // Check HTTP response code
    long response_code;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code != 200) {
        return GrpcStatus{.code = 13, .message = "HTTP error: " + std::to_string(response_code)};
    }

    // Extract and return gRPC status
    return extract_grpc_status(curl_);
}

GrpcStatus GrpcClient::extract_grpc_status(CURL* curl) {
    if (!curl) {
        return GrpcStatus{.code = 13, .message = "cURL not initialized"};
    }

    GrpcStatus status;

    // Try to get grpc-status from trailers
    struct curl_header* header;
    CURLHcode h_res = curl_easy_header(curl, "grpc-status", 0, CURLH_HEADER | CURLH_TRAILER, -1, &header);
    if (h_res == CURLHE_OK && header && header->value) {
        status.code = std::atoi(header->value);
    }

    // Try to get grpc-message from trailers if status is not OK
    if (status.code != 0) {
        h_res = curl_easy_header(curl, "grpc-message", 0, CURLH_HEADER | CURLH_TRAILER, -1, &header);
        if (h_res == CURLHE_OK && header && header->value) {
            // Use curl's built-in URL decode function
            int decode_len;
            char* decoded = curl_easy_unescape(curl, header->value, 0, &decode_len);
            if (decoded) {
                status.message = std::string(decoded, decode_len);
                curl_free(decoded);
            } else {
                status.message = header->value;  // Fallback to original if decode fails
            }
        }
    }

    return status;
}

}  // namespace spiffe
