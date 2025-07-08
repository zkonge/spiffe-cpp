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
    spiffe::SpiffeWorkloadApiClient client("/tmp/spire-agent/public/api.sock");

    std::vector<spiffe::JwtSvid> svids;
    auto status = client.fetch_jwt_svid(svids, {"zti"});

    std::cout << "Status: " << status.code_str() << ", Message: " << status.message << std::endl;

    for (auto svid : svids) {
        std::cout << "SPIFFE ID: " << svid.spiffe_id << std::endl;
        std::cout << "SVID: " << svid.svid << std::endl;
        std::cout << "Hint: " << svid.hint << std::endl;
    }

    status = client.fetch_jwt_bundles([](const spiffe::JwtBundles& bundles) {
        std::cout << "JWT Bundles:" << std::endl;
        for (const auto& bundle : bundles.bundles) {
            std::cout << "TrustDomain: " << bundle.first << std::endl;
            std::cout << "Bundle: " << bundle.second << std::endl;
        }
        return spiffe::Status{.code = 1, .message = "simulate error"};
    });

    std::cout << "Status: " << status.code_str() << ", Message: " << status.message << std::endl;

    return 0;
}
