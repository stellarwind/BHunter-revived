#include "bh_riff.h"

static unsigned long u32(const unsigned char *p) {
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}
int __cdecl bh_riff_scan(const void *bytes, unsigned long size, const unsigned char **format,
                         const unsigned char **samples, unsigned long *sample_bytes) {
    const unsigned char *data = (const unsigned char *)bytes;
    unsigned long at = 12, end, length, tag, next;
    if (format)
        *format = 0;
    if (samples)
        *samples = 0;
    if (sample_bytes)
        *sample_bytes = 0;
    if (!data || size < 12 || u32(data) != 0x46464952UL || u32(data + 8) != 0x45564157UL ||
        u32(data + 4) <= 4 || u32(data + 4) > size - 8)
        return 0;
    end = u32(data + 4) + 8;
    while (at < end) {
        if (end - at < 8)
            return 0;
        tag = u32(data + at);
        length = u32(data + at + 4);
        next = at + 8;
        if (length > end - next)
            return 0;
        if (tag == 0x20746d66UL) {
            if (format && !*format) {
                if (length < 14)
                    return 0;
                *format = data + next;
                if ((!samples || *samples) && (!sample_bytes || *sample_bytes))
                    return 1;
            }
        } else if (tag == 0x61746164UL &&
                   ((samples && !*samples) || (sample_bytes && !*sample_bytes))) {
            if (samples)
                *samples = data + next;
            if (sample_bytes)
                *sample_bytes = length;
            if (!format || *format)
                return 1;
        }
        next += length;
        if (length & 1) {
            if (next == end)
                return 0;
            ++next;
        }
        at = next;
    }
    return 0;
}
