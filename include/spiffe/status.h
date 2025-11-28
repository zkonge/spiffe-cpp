#pragma once

#include <string>
#include <vector>

namespace spiffe {
struct Status {
    int code = 0;
    std::string message;

    inline bool is_ok() const { return code == 0; }
    const std::string& code_str() const;
};
}  // namespace spiffe
