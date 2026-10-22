#pragma once
#include <stdint.h>
#include <stddef.h>
struct Preferences {
    bool begin(const char*,bool) { return true; }
    uint64_t getULong64(const char*,uint64_t n) { return n; }
    size_t putULong64(const char*,uint64_t) { return 8; }
    void end() {}
};
