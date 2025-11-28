#pragma once

#include <spiffe/status.h>
#include <spiffe/types.h>

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace spiffe {

class WorkloadApiClient {
   public:
    WorkloadApiClient(const std::string& socket_path);
    ~WorkloadApiClient();

    // Disallow copy
    WorkloadApiClient(const WorkloadApiClient&) = delete;
    WorkloadApiClient& operator=(const WorkloadApiClient&) = delete;

    // Allow move
    WorkloadApiClient(WorkloadApiClient&&);
    WorkloadApiClient& operator=(WorkloadApiClient&&);

    // Streaming calls, only returns when cancelled or error occurs
    Status fetch_x509_svid(                                      //
        std::function<Status(const X509SvidContext&)> callback,  //
        std::shared_future<void> cancellation_token              //
    );
    Status fetch_x509_bundles(                                      //
        std::function<Status(const X509BundlesContext&)> callback,  //
        std::shared_future<void> cancellation_token                 //
    );
    Status fetch_jwt_bundles(                               //
        std::function<Status(const JwtBundles&)> callback,  //
        std::shared_future<void> cancellation_token         //
    );

    // Unary calls
    Status fetch_jwt_svid(                                                         //
        std::vector<JwtSvid>& out,                                                 //
        const std::vector<std::string>& audience,                                  //
        const std::string& spiffe_id = "",                                         //
        const std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)  //
    );

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace spiffe
