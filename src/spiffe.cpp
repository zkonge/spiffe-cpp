#include <spiffe/spiffe.h>

#include "der.h"
#include "grpc_client.h"
#include "proto/workloadapi.h"

namespace spiffe {

const std::vector<GrpcMetadata> DEFAULT_SPIFFE_GRPC_METADATA = {
    {"workload.spiffe.io", "true"},
};

class WorkloadApiClient::Impl {
   public:
    Impl(const std::string& socket_path) : socket_path_(socket_path) { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~Impl() { curl_global_cleanup(); }

    Status fetch_x509_svid(std::function<Status(const X509SvidContext&)> callback,
                           std::shared_future<void> cancellation_token) {
        GrpcClient client(socket_path_);

        ProtoX509SvidRequest request;

        Buffer request_buf = encode_proto_message(request);

        GrpcStatus grpc_status = client.call_stream(
            "SpiffeWorkloadAPI", "FetchX509SVID", request_buf,
            [&](const GrpcResponse& response) {
                ProtoX509SvidResponse proto_response;
                Buffer cloned_data(response.data);
                if (!decode_proto_message(cloned_data, proto_response)) {
                    return GrpcStatus{
                        .code = 13,
                        .message = "decode gRPC response failed",
                    };
                }

                X509SvidContext context;
                for (auto svid : proto_response.svids.get()) {
                    auto x509_svid = extract_all_certificates(svid.x509_svid.get());
                    auto x509_svid_key = svid.x509_svid_key.get();
                    auto bundle = extract_all_certificates(svid.bundle.get());

                    context.svids.emplace_back(X509Svid{
                        .spiffe_id = svid.spiffe_id.get(),
                        .x509_svid = x509_svid,
                        .x509_svid_key = Buffer(x509_svid_key.begin(), x509_svid_key.end()),
                        .bundle = bundle,
                        .hint = svid.hint.get(),
                    });
                }

                for (const auto& crl : proto_response.crl.get()) {
                    context.crl.push_back(Buffer(crl.begin(), crl.end()));
                }

                for (auto& item : proto_response.federated_bundles.get()) {
                    context.federated_bundles[item.key.get()] = extract_all_certificates(item.value.get());
                }

                Status status = callback(context);
                if (!status.is_ok()) {
                    return GrpcStatus{
                        .code = status.code,
                        .message = status.message,
                    };
                }
                return GrpcStatus{
                    .code = 0,  // OK
                };
            },
            DEFAULT_SPIFFE_GRPC_METADATA, cancellation_token);

        return Status{.code = grpc_status.code, .message = grpc_status.message};
    }

    Status fetch_x509_bundle(std::function<Status(const X509BundlesContext&)> callback,
                             std::shared_future<void> cancellation_token) {
        GrpcClient client(socket_path_);

        ProtoJwtBundlesRequest request;

        Buffer request_buf = encode_proto_message(request);

        GrpcStatus grpc_status = client.call_stream(
            "SpiffeWorkloadAPI", "FetchX509Bundles", request_buf,
            [&](const GrpcResponse& response) {
                ProtoX509BundlesResponse proto_response;
                Buffer cloned_data(response.data);
                if (!decode_proto_message(cloned_data, proto_response)) {
                    return GrpcStatus{
                        .code = 13,
                        .message = "decode gRPC response failed",
                    };
                }

                X509BundlesContext context;
                for (const auto& crl : proto_response.crl.get()) {
                    context.crl.push_back(Buffer(crl.begin(), crl.end()));
                }

                for (auto& item : proto_response.bundles.get()) {
                    context.bundles[item.key.get()] = extract_all_certificates(item.value.get());
                }

                Status status = callback(context);
                if (!status.is_ok()) {
                    return GrpcStatus{
                        .code = status.code,
                        .message = status.message,
                    };
                }
                return GrpcStatus{
                    .code = 0,  // OK
                };
            },
            DEFAULT_SPIFFE_GRPC_METADATA, cancellation_token);

        return Status{
            .code = grpc_status.code,
            .message = grpc_status.message,
        };
    }

    Status get_jwt_bundles(std::function<Status(const JwtBundles&)> callback,
                           std::shared_future<void> cancellation_token) {
        GrpcClient client(socket_path_);

        ProtoJwtBundlesRequest request;

        Buffer request_buf = encode_proto_message(request);

        GrpcStatus grpc_status = client.call_stream(
            "SpiffeWorkloadAPI", "FetchJWTBundles", request_buf,
            [&](const GrpcResponse& response) {
                ProtoJwtBundlesResponse proto_response;
                Buffer cloned_data(response.data);
                if (!decode_proto_message(cloned_data, proto_response)) {
                    return GrpcStatus{
                        .code = 13,
                        .message = "decode gRPC response failed",
                    };
                }

                JwtBundles bundles;
                for (auto item : proto_response.bundles.get()) {
                    bundles.bundles[item.key.get()] = item.value.get();
                }

                Status status = callback(bundles);
                if (!status.is_ok()) {
                    return GrpcStatus{
                        .code = status.code,
                        .message = status.message,
                    };
                }

                return GrpcStatus{
                    .code = 0,  // OK
                };
            },
            DEFAULT_SPIFFE_GRPC_METADATA, cancellation_token);

        return Status{
            .code = grpc_status.code,
            .message = grpc_status.message,
        };
    }

    Status get_jwt_svid(std::vector<JwtSvid>& out, const std::vector<std::string>& audience,
                        const std::string& spiffe_id, const std::chrono::milliseconds timeout) {
        GrpcClient client(socket_path_);

        ProtoJwtSvidRequest request;
        request.audience.set(audience);
        request.spiffe_id.set(spiffe_id);

        Buffer request_buf = encode_proto_message(request);

        GrpcResult result = client.call(          //
            "SpiffeWorkloadAPI", "FetchJWTSVID",  //
            request_buf,                          //
            DEFAULT_SPIFFE_GRPC_METADATA,         //
            timeout                               //
        );

        if (result.has_response) {
            ProtoJwtSvidResponse response;
            if (!decode_proto_message(result.response.data, response)) {
                return Status{
                    .code = 13,
                    .message = "decode gRPC response failed",
                };
            }

            for (auto svid : response.svids.get()) {
                out.emplace_back(JwtSvid{
                    .spiffe_id = svid.spiffe_id.get(),
                    .svid = svid.svid.get(),
                    .hint = svid.hint.get(),
                });
            }
        }

        return Status{.code = result.status.code, .message = result.status.message};
    }

   private:
    std::string socket_path_;
};

WorkloadApiClient::WorkloadApiClient(const std::string& socket_path)
    : impl_(std::make_unique<WorkloadApiClient::Impl>(socket_path)) {}
WorkloadApiClient::~WorkloadApiClient() = default;

WorkloadApiClient::WorkloadApiClient(WorkloadApiClient&&) = default;
WorkloadApiClient& WorkloadApiClient::operator=(WorkloadApiClient&&) = default;

Status WorkloadApiClient::fetch_x509_svid(std::function<Status(const X509SvidContext&)> callback,
                                          std::shared_future<void> cancellation_token) {
    return impl_->fetch_x509_svid(callback, cancellation_token);
}

Status WorkloadApiClient::fetch_x509_bundles(std::function<Status(const X509BundlesContext&)> callback,
                                             std::shared_future<void> cancellation_token) {
    return impl_->fetch_x509_bundle(callback, cancellation_token);
}

Status WorkloadApiClient::fetch_jwt_bundles(std::function<Status(const JwtBundles&)> callback,
                                            std::shared_future<void> cancellation_token) {
    return impl_->get_jwt_bundles(callback, cancellation_token);
}

Status WorkloadApiClient::fetch_jwt_svid(std::vector<JwtSvid>& out, const std::vector<std::string>& audience,
                                         const std::string& spiffe_id, const std::chrono::milliseconds timeout) {
    return impl_->get_jwt_svid(out, audience, spiffe_id, timeout);
}

}  // namespace spiffe
