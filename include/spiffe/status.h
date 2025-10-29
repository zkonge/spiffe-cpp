#pragma once

#include <string>
#include <vector>

namespace spiffe {
class Status {
   public:
    int code = 0;
    std::string message;

    inline bool is_ok() const { return code == 0; }
    std::string code_str();
};
}  // namespace spiffe
