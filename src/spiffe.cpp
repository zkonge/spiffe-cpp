#include <spiffe/spiffe.h>

#include "grpc_client.h"
#include "proto/workloadapi.h"

namespace spiffe {

const std::vector<GrpcMetadata> DEFAULT_SPIFFE_GRPC_METADATA = {
    {"workload.spiffe.io", "true"},
};

class SpiffeWorkloadApiClient::Impl {
   public:
    Impl(const std::string& socket_path) : socket_path_(socket_path) {}

    Status fetch_x509_svid(std::function<Status(const std::vector<X509SvidContext>&)> cb) {
        // GrpcClient client(socket_path_);

        // ProtoX509SvidRequest request;

        // std::vector<uint8_t> request_buf = encode_proto_message(request);

        // GrpcResult result = client.call_stream(
        //     "SpiffeWorkloadAPI", "FetchX509SVID", request_buf,
        //     [&](const GrpcResponse& response) {
        //         ProtoX509SvidResponse proto_response;
        //         if (!decode_proto_message(response.data, proto_response)) {
        //             return GrpcStatus{
        //                 .code = 13,
        //                 .message = "decode gRPC response failed",
        //             };
        //         }

        //         X509SvidContext context;
        //         for (auto svid : proto_response.svids.get()) {
        //             context.svids.emplace_back(X509Svid{
        //                 .spiffe_id = svid.spiffe_id.get(),
        //                 .x509_svid = svid.x509_svid.get(),
        //                 .x509_svid_key = svid.x509_svid_key.get(),
        //                 .bundle = svid.bundle.get(),
        //                 .hint = svid.hint.get(),
        //             });
        //         }

        //         for (const auto& crl : proto_response.crl.get()) {
        //             contexts.back().crl.push_back(crl);
        //         }

        //         for (const auto& item : proto_response.federated_bundles.get()) {
        //             contexts.back().federated_bundles[item.key.get()] = X509Bundle{item.value.get()};
        //         }

        //         Status status = cb(contexts);
        //         if (!status.is_ok()) {
        //             return GrpcStatus{
        //                 .code = status.code,
        //                 .message = status.message,
        //             };
        //         }
        //         return GrpcStatus{
        //             .code = 0,  // OK
        //         };
        //     },
        //     DEFAULT_SPIFFE_GRPC_METADATA);

        return Status{
            .code = 12,
            .message = "not implemented",
        };
    }

    Status fetch_x509_bundle(std::function<Status(const std::vector<X509BundlesContext>&)> cb) {
        return Status{
            .code = 12,
            .message = "not implemented",
        };
    }

    Status get_jwt_svid(std::vector<JwtSvid>& out, const std::vector<std::string>& audience,
                        const std::string& spiffe_id = "") {
        GrpcClient client(socket_path_);

        ProtoJwtSvidRequest request;
        request.audience.set(audience);
        request.spiffe_id.set(spiffe_id);

        std::vector<uint8_t> request_buf = encode_proto_message(request);

        GrpcResult result = client.call(          //
            "SpiffeWorkloadAPI", "FetchJWTSVID",  //
            request_buf,                          //
            DEFAULT_SPIFFE_GRPC_METADATA          //
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

        return Status{.code = result.status.code};
    }

    Status get_jwt_bundles(std::function<Status(const JwtBundles&)> callback) {
        GrpcClient client(socket_path_);

        ProtoJwtBundlesRequest request;

        std::vector<uint8_t> request_buf = encode_proto_message(request);

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
            DEFAULT_SPIFFE_GRPC_METADATA);

        return Status{
            .code = grpc_status.code,
            .message = grpc_status.message,
        };
    }

    // void test_spiffe_bundle_call() {
    //     GrpcClient client("/tmp/spire-agent/public/api.sock");

    //     // Create request
    //     std::vector<uint8_t> request = ProtobufUtils::create_fetch_jwt_bundle_request();

    //     // Make the streaming call

    //     GrpcResult result = client.call_stream(
    //         "SpiffeWorkloadAPI", "FetchJWTBundles", request,
    //         [&](const GrpcResponse& response) {
    //             std::string reply = ProtobufUtils::parse_fetch_jwt_bundle_reply(response.data);
    //             std::cout << "Stream response: " << reply << std::endl;
    //         },
    //         {
    //             {"workload.spiffe.io", "true"},
    //         });

    //     GrpcStatus status = result.status;
    //     std::cout << "Final gRPC Status: " << status.code;
    //     if (status.is_ok()) {
    //         std::cout << " (OK)" << std::endl;
    //     } else {
    //         std::cout << " (" << status.error_message() << ")" << std::endl;
    //     }
    // }

   private:
    std::string socket_path_;
};

SpiffeWorkloadApiClient::SpiffeWorkloadApiClient(const std::string& socket_path)
    : impl_(std::make_unique<SpiffeWorkloadApiClient::Impl>(socket_path)) {}

SpiffeWorkloadApiClient::~SpiffeWorkloadApiClient() = default;

Status SpiffeWorkloadApiClient::fetch_x509_svid(std::function<Status(const std::vector<X509SvidContext>&)> callback) {
    return impl_->fetch_x509_svid(callback);
}

Status SpiffeWorkloadApiClient::fetch_x509_bundles(
    std::function<Status(const std::vector<X509BundlesContext>&)> callback) {
    return impl_->fetch_x509_bundle(callback);
}

Status SpiffeWorkloadApiClient::fetch_jwt_svid(std::vector<JwtSvid>& out, const std::vector<std::string>& audience,
                                               const std::string& spiffe_id) {
    return impl_->get_jwt_svid(out, audience, spiffe_id);
}

Status SpiffeWorkloadApiClient::fetch_jwt_bundles(std::function<Status(const JwtBundles&)> callback) {
    return impl_->get_jwt_bundles(callback);
}

}  // namespace spiffe