#include "../src/bh_pcx_reader.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks;
static int failures;
#define CHECK(x, label) check((x) != 0, label, __LINE__)
static void check(int ok, const char *label, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        if (failures < 8)
            printf("FAIL PCX reader %d: %s\n", line, label);
    }
}
static int write_fixture(const unsigned char *encoded, unsigned long count, unsigned long xmax,
                         unsigned long ymax, int valid) {
    unsigned char header[128], palette[768];
    unsigned long i;
    FILE *file = fopen("bhpcx_reader.tmp", "wb");
    int ok;
    if (!file)
        return 0;
    memset(header, 0, sizeof(header));
    header[0] = valid ? 10 : 0;
    header[1] = 2;
    header[2] = 1;
    header[3] = 8;
    header[4] = 17;
    header[6] = 23;
    header[8] = (unsigned char)xmax;
    header[9] = (unsigned char)(xmax >> 8);
    header[10] = (unsigned char)ymax;
    header[11] = (unsigned char)(ymax >> 8);
    for (i = 0; i < 768; ++i)
        palette[i] = (unsigned char)(i * 37);
    ok = fwrite(header, 1, 128, file) == 128 && fwrite(encoded, 1, count, file) == count &&
         fwrite(palette, 1, 768, file) == 768;
    return !fclose(file) && ok;
}
int bh_pcx_reader_real_check(const char *name, const BHPcx *image) {
    BHPcxReader reader;
    unsigned char pixel;
    unsigned long i;
    bh_pcx_reader_init(&reader);
    if (!bh_pcx_reader_open(&reader, name))
        return 0;
    if (reader.width != image->width || reader.height != image->height ||
        memcmp(reader.palette, image->palette, 768))
        goto failed;
    for (i = 0; i < image->pixel_bytes; ++i)
        if (!bh_pcx_reader_next(&reader, &pixel) || pixel != image->pixels[i])
            goto failed;
    return bh_pcx_reader_close(&reader);
failed:
    bh_pcx_reader_close(&reader);
    return 0;
}
static void cases(void) {
    unsigned char encoded[1100], pixel;
    unsigned long i, n;
    BHPcxReader reader;
    BHVFile *file;
    bh_pcx_reader_init(&reader);
    memset(encoded, 7, 1023);
    n = 1023;
    encoded[n++] = 0xc3;
    encoded[n++] = 9;
    encoded[n++] = 0xc0;
    encoded[n++] = 0xc0;
    encoded[n++] = 0xc1;
    encoded[n++] = 200;
    encoded[n++] = 0xc2;
    encoded[n++] = 255;
    CHECK(write_fixture(encoded, n, 1028, 0, 1),
          "write actual buffered stream fixture without palette marker");
    CHECK(bh_pcx_reader_open(&reader, "bhpcx_reader.tmp") && reader.width == 1029 &&
              reader.height == 1 && bh_vfile_tell(reader.file) == 128,
          "original header accepts non-v5/nonzero-origin layout and seeks to pixels");
    for (i = 0; i < 256; ++i)
        CHECK(reader.map[i] == i, "open initializes full identity mapping");
    for (i = 0; i < 768; ++i)
        CHECK(reader.palette[i] == (unsigned char)(i * 37),
              "complete final palette bytes retained");
    for (i = 0; i < 1023; ++i)
        CHECK(bh_pcx_reader_next(&reader, &pixel) && pixel == 7, "literal stream before refill");
    CHECK(bh_pcx_reader_next(&reader, &pixel) && pixel == 9 && reader.pending == 2,
          "run prefix and value cross 1024-byte refill boundary");
    reader.map[9] = 77;
    CHECK(bh_pcx_reader_next(&reader, &pixel) && pixel == 77,
          "pending repeat consults current index map");
    reader.map[9] = 12;
    CHECK(bh_pcx_reader_next(&reader, &pixel) && pixel == 12 && !reader.pending,
          "each later repeat remaps raw value independently");
    CHECK(bh_pcx_reader_next(&reader, &pixel) && pixel == 200,
          "C0 prefixes skip and C1 emits exactly one high literal");
    CHECK(bh_pcx_reader_next(&reader, &pixel) && pixel == 255 &&
              bh_pcx_reader_next(&reader, &pixel) && pixel == 255,
          "high-value repeated pixels retain low-byte behavior");
    pixel = 123;
    CHECK(!bh_pcx_reader_next(&reader, &pixel) && pixel == 123 && reader.error == BH_PCX_TRUNCATED,
          "bounded EOF never reads palette as stale pixel data");
    CHECK(bh_pcx_reader_close(&reader) && !reader.file, "close releases owning VFILE");
    CHECK(write_fixture(encoded, n, 65535, 65535, 1) &&
              bh_pcx_reader_open(&reader, "bhpcx_reader.tmp") && !reader.width && !reader.height,
          "original low-16-bit dimensions wrap without invented header rejection");
    file = reader.file;
    CHECK(!bh_pcx_reader_open(&reader, "bhpcx_reader.tmp") && reader.file == file,
          "live owner must be closed before reopening");
    CHECK(bh_pcx_reader_close(&reader), "close wrapped-dimension reader");
    CHECK(write_fixture(encoded, n, 0, 0, 0) && !bh_pcx_reader_open(&reader, "bhpcx_reader.tmp") &&
              !reader.file && reader.error == BH_PCX_FORMAT,
          "invalid header closes temporary file");
    CHECK(!remove("bhpcx_reader.tmp"), "failed open retained no live file handle");
    encoded[0] = 0xc1;
    CHECK(write_fixture(encoded, 1, 0, 0, 1) && bh_pcx_reader_open(&reader, "bhpcx_reader.tmp"),
          "open stream with truncated run value");
    pixel = 121;
    CHECK(!bh_pcx_reader_next(&reader, &pixel) && pixel == 121,
          "missing run value preserves caller pixel");
    CHECK(bh_pcx_reader_close(&reader), "close truncated reader");
    encoded[0] = encoded[1] = 0xc0;
    CHECK(write_fixture(encoded, 2, 0, 0, 1) && bh_pcx_reader_open(&reader, "bhpcx_reader.tmp"),
          "open only-skip compressed span");
    CHECK(!bh_pcx_reader_next(&reader, &pixel), "only-C0 input terminates at bounded EOF");
    CHECK(bh_pcx_reader_close(&reader), "close only-skip reader");
    remove("bhpcx_reader.tmp");
}
int bh_pcx_reader_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    *count = checks;
    printf("{\"scenario\":\"pcx-buffered-reader\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
