#include <spiffe/spiffe.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace spiffe;

std::string toHex(const std::uint8_t* data, size_t size) {
    std::string hex;
    hex.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        hex.append(buf);
    }
    return hex;
}

int main() {
    std::promise<void> cancellation_promise;
    std::shared_future<void> cancellation_token(cancellation_promise.get_future());

    spiffe::WorkloadApiClient client("/tmp/spire-agent/public/api.sock");

    // Fetch JWT SVIDs
    std::vector<spiffe::JwtSvid> svids;
    spiffe::Status status = client.fetch_jwt_svid(svids, {"zti"});

    std::cout << "Status: " << status.code_str() << ", Message: " << status.message << std::endl;

    for (auto svid : svids) {
        std::cout << "SPIFFE ID: " << svid.spiffe_id << std::endl;
        std::cout << "SVID: " << svid.svid << std::endl;
        std::cout << "Hint: " << svid.hint << std::endl;
    }

    // Fetch JWT Bundles
    std::thread jwt_bundle_t([&, cancellation_token]() {
        spiffe::Status status = client.fetch_jwt_bundles(
            [](const spiffe::JwtBundles& bundles) {
                std::cout << "JWT Bundles:" << std::endl;
                for (const auto& bundle : bundles.bundles) {
                    std::cout << "TrustDomain: " << bundle.first << std::endl;
                    std::cout << "Bundle: " << bundle.second << std::endl;
                }
                return spiffe::Status{.code = 0};
            },
            cancellation_token);

        std::cout << "Status: " << status.code_str() << ", Message: " << status.message << std::endl;
    });

    // Fetch X.509 SVIDs
    std::thread x509_svid_t([&, cancellation_token]() {
        spiffe::Status status = client.fetch_x509_svid(
            [](const spiffe::X509SvidContext& svid_context) {
                std::cout << "X.509 SVIDs:" << std::endl;
                for (const auto& svid : svid_context.svids) {
                    std::cout << "SPIFFE ID: " << svid.spiffe_id << std::endl;
                    std::cout << "X.509 SVID contains:" << std::endl;
                    for (const auto& cert : svid.x509_svid) {
                        std::cout << "  Cert (hex): " << toHex(cert.data(), cert.size()) << std::endl;
                    }
                    std::cout << "X.509 SVID Key (hex): " << toHex(svid.x509_svid_key.data(), svid.x509_svid_key.size())
                              << std::endl;
                    std::cout << "Bundle contains:" << std::endl;
                    for (const auto& cert : svid.bundle) {
                        std::cout << "  Cert (hex): " << toHex(cert.data(), cert.size()) << std::endl;
                    }
                    std::cout << "Hint: " << svid.hint << std::endl;
                }
                return spiffe::Status{.code = 0};
            },
            cancellation_token);

        std::cout << "Status: " << status.code_str() << ", Message: " << status.message << std::endl;
    });

    // Fetch X.509 Bundles
    std::thread x509_bundle_t([&, cancellation_token]() {
        spiffe::Status status = client.fetch_x509_bundles(
            [](const spiffe::X509BundlesContext& bundles) {
                std::cout << "X.509 Bundles:" << std::endl;
                for (auto& bundle : bundles.bundles) {
                    std::cout << "TrustDomain: " << bundle.first << std::endl;
                    std::cout << "Bundle contains:" << std::endl;
                    for (const auto& cert : bundle.second) {
                        std::cout << "  Cert (hex): " << toHex(cert.data(), cert.size()) << std::endl;
                    }
                }
                return spiffe::Status{.code = 0};
            },
            cancellation_token);

        std::cout << "Status: " << status.code_str() << ", Message: " << status.message << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Cancelling all streaming calls..." << std::endl;

    cancellation_promise.set_value();

    jwt_bundle_t.join();
    x509_bundle_t.join();
    x509_svid_t.join();

    return 0;
}
