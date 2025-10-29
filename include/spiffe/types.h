
#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

namespace spiffe {

using Buffer = std::vector<uint8_t>;

using TrustDomain = std::string;
using X509CertificateChain = std::vector<Buffer>;
using X509Bundle = std::vector<Buffer>;

struct X509Svid {
    std::string spiffe_id;
    X509CertificateChain x509_svid;
    Buffer x509_svid_key;
    X509Bundle bundle;
    std::string hint;
};

// Response of FetchX509SVID
struct X509SvidContext {
    std::vector<X509Svid> svids;
    std::vector<Buffer> crl;
    std::unordered_map<TrustDomain, X509Bundle> federated_bundles;
};

// Response of FetchX509Bundles
struct X509BundlesContext {
    std::vector<Buffer> crl;
    std::unordered_map<TrustDomain, X509Bundle> bundles;
};

struct JwtSvid {
    std::string spiffe_id;
    std::string svid;
    std::string hint;
};

struct JwtBundles {
    std::unordered_map<TrustDomain, std::string> bundles;
};

// Simple cancellation token for aborting long-running/streaming operations
class CancelContext {
   public:
    void cancel() { canceled_.store(true, std::memory_order_relaxed); }
    bool canceled() const { return canceled_.load(std::memory_order_relaxed); }

   private:
    std::atomic<bool> canceled_{false};
};

}  // namespace spiffe
