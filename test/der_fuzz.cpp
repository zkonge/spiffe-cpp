#include "der.h"
#include <cstdint>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    spiffe::extract_all_certificates(Data, Size);
    return 0;
}
