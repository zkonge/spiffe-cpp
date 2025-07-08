#include <spiffe/status.h>

namespace spiffe {

std::string Status::code_str() {
    switch (code) {
        case 0:
            return "OK";
        case 1:
            return "CANCELLED";
        case 2:
            return "UNKNOWN";
        case 3:
            return "INVALID_ARGUMENT";
        case 4:
            return "DEADLINE_EXCEEDED";
        case 5:
            return "NOT_FOUND";
        case 6:
            return "ALREADY_EXISTS";
        case 7:
            return "PERMISSION_DENIED";
        case 8:
            return "RESOURCE_EXHAUSTED";
        case 9:
            return "FAILED_PRECONDITION";
        case 10:
            return "ABORTED";
        case 11:
            return "OUT_OF_RANGE";
        case 12:
            return "UNIMPLEMENTED";
        case 13:
            return "INTERNAL";
        case 14:
            return "UNAVAILABLE";
        case 15:
            return "DATA_LOSS";
        default:
            return "UNKNOWN_STATUS_CODE";
    }
}

}  // namespace spiffe
