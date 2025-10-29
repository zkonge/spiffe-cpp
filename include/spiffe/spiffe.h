#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "spiffe/status.h"
#include "spiffe/types.h"

namespace spiffe {

class SpiffeWorkloadApiClient {
   public:
    SpiffeWorkloadApiClient(const std::string& socket_path);
    ~SpiffeWorkloadApiClient();

    Status fetch_x509_svid(std::function<Status(const X509SvidContext&)> callback);
    Status fetch_x509_svid(std::function<Status(const X509SvidContext&)> callback, const CancelContext& cancel);
    Status fetch_x509_bundles(std::function<Status(const X509BundlesContext&)> callback);
    Status fetch_x509_bundles(std::function<Status(const X509BundlesContext&)> callback, const CancelContext& cancel);
    Status fetch_jwt_svid(std::vector<JwtSvid>& out, const std::vector<std::string>& audience,
                          const std::string& spiffe_id = "");
    Status fetch_jwt_bundles(std::function<Status(const JwtBundles&)> callback);
    Status fetch_jwt_bundles(std::function<Status(const JwtBundles&)> callback, const CancelContext& cancel);

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace spiffe
