#include <spiffe/status.h>

namespace spiffe {

const std::string STATUS_STRING[] = {
    "OK",                   //
    "CANCELLED",            //
    "UNKNOWN",              //
    "INVALID_ARGUMENT",     //
    "DEADLINE_EXCEEDED",    //
    "NOT_FOUND",            //
    "ALREADY_EXISTS",       //
    "PERMISSION_DENIED",    //
    "RESOURCE_EXHAUSTED",   //
    "FAILED_PRECONDITION",  //
    "ABORTED",              //
    "OUT_OF_RANGE",         //
    "UNIMPLEMENTED",        //
    "INTERNAL",             //
    "UNAVAILABLE",          //
    "DATA_LOSS"             //
};
const std::string UNKNOWN_STATUS_STRING = "UNKNOWN_STATUS_CODE";

const std::string& Status::code_str() const {
    if (code >= 0 && code < static_cast<int>(sizeof(STATUS_STRING) / sizeof(STATUS_STRING[0]))) {
        return STATUS_STRING[code];
    }
    return UNKNOWN_STATUS_STRING;
}

}  // namespace spiffe
