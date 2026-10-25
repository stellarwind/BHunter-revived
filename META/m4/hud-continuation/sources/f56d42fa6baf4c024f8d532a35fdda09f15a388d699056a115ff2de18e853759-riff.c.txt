#include "../src/bh_riff.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

static unsigned long checks;
static int failures;
#define CHECK(x, label) check((x) != 0, label, __LINE__)
static void check(int ok, const char *label, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        if (failures < 8)
            printf("FAIL riff %d: %s\n", line, label);
    }
}
static void put(unsigned char *p, unsigned long n) {
    p[0] = (unsigned char)n;
    p[1] = (unsigned char)(n >> 8);
    p[2] = (unsigned char)(n >> 16);
    p[3] = (unsigned char)(n >> 24);
}
static unsigned long chunk(unsigned char *bytes, unsigned long at, const char *tag,
                           unsigned long size) {
    memcpy(bytes + at, tag, 4);
    put(bytes + at + 4, size);
    return at + 8 + size + (size & 1);
}
static unsigned long fixture(unsigned char *bytes, int reverse, unsigned long data_size,
                             unsigned long format_size) {
    unsigned long at = 12;
    memset(bytes, 0, 128);
    memcpy(bytes, "RIFF", 4);
    memcpy(bytes + 8, "WAVE", 4);
    if (reverse)
        at = chunk(bytes, at, "data", data_size);
    at = chunk(bytes, at, "fmt ", format_size);
    if (!reverse)
        at = chunk(bytes, at, "data", data_size);
    put(bytes + 4, at - 8);
    return at;
}
int bh_riff_real_check(const void *bytes, unsigned long size, const unsigned char *samples,
                       unsigned long sample_bytes) {
    const unsigned char *format = NULL, *data = NULL;
    unsigned long length = 0;
    return bh_riff_scan(bytes, size, &format, &data, &length) && format && data == samples &&
           length == sample_bytes;
}
static void cases(void) {
    unsigned char bytes[128], *pages, *boundary;
    const unsigned char *format, *samples;
    unsigned long length, size, mask, reverse, i, previous;
    SYSTEM_INFO system;
    for (reverse = 0; reverse < 2; ++reverse) {
        size = fixture(bytes, reverse, 3, 14);
        for (mask = 0; mask < 8; ++mask) {
            format = samples = bytes;
            length = 99;
            CHECK(bh_riff_scan(bytes, size, (mask & 1) ? &format : NULL,
                               (mask & 2) ? &samples : NULL,
                               (mask & 4) ? &length : NULL) == (mask != 0),
                  "all optional-output combinations in both chunk orders");
            CHECK(!(mask & 1) || format == bytes + (reverse ? 32 : 20),
                  "format pointer is original payload");
            CHECK(!(mask & 2) || samples == bytes + (reverse ? 20 : 42),
                  "sample pointer is original payload");
            CHECK(!(mask & 4) || length == 3, "requested data length retained");
        }
    }
    size = fixture(bytes, 0, 3, 13);
    CHECK(!bh_riff_scan(bytes, size, &format, &samples, &length) && !format && !samples && !length,
          "requested short format fails before later data");
    CHECK(bh_riff_scan(bytes, size, NULL, &samples, &length) && length == 3,
          "unrequested short format is skipped");
    size = fixture(bytes, 1, 0, 14);
    CHECK(!bh_riff_scan(bytes, size, &format, &samples, &length) && format && samples && !length,
          "empty data before fmt retains original partial outputs and failure");
    CHECK(bh_riff_scan(bytes, size, &format, &samples, NULL),
          "same empty data succeeds when zero size output is not requested");
    size = fixture(bytes, 0, 0, 14);
    CHECK(bh_riff_scan(bytes, size, &format, &samples, &length) && !length,
          "empty data after fmt succeeds immediately");
    size = fixture(bytes, 0, 3, 14);
    memcpy(bytes + size, "BAD", 3);
    put(bytes + 4, size - 8 + 3);
    CHECK(bh_riff_scan(bytes, size + 3, &format, &samples, &length),
          "successful requested chunks ignore malformed later tail");
    CHECK(!bh_riff_scan(bytes, size + 3, NULL, NULL, NULL),
          "no requested outputs never manufacture success");
    size = fixture(bytes, 0, 3, 14);
    GetSystemInfo(&system);
    pages = (unsigned char *)VirtualAlloc(NULL, system.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);
    CHECK(pages != NULL, "allocate protected scanner boundary");
    if (pages) {
        CHECK(
            VirtualProtect(pages + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &previous),
            "protect boundary after provided input");
        boundary = pages + system.dwPageSize;
        for (i = 0; i < size; ++i) {
            memcpy(boundary - i, bytes, i);
            CHECK(!bh_riff_scan(boundary - i, i, &format, &samples, &length),
                  "every short input prefix fails without reading guard page");
        }
        CHECK(VirtualFree(pages, 0, MEM_RELEASE), "release protected scanner pages");
    }
}
int bh_riff_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    *count = checks;
    printf("{\"scenario\":\"riff-optional\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
