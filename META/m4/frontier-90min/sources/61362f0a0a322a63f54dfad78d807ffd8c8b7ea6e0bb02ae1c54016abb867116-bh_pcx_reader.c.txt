#include "bh_pcx_reader.h"
#include <string.h>

void __cdecl bh_pcx_reader_init(BHPcxReader *reader) {
    if (reader)
        memset(reader, 0, sizeof(*reader));
}
int __cdecl bh_pcx_reader_close(BHPcxReader *reader) {
    int result;
    if (!reader)
        return 0;
    result = !reader->file || !bh_vfile_close(reader->file);
    memset(reader, 0, sizeof(*reader));
    if (!result)
        reader->error = BH_PCX_IO;
    return result;
}
int __cdecl bh_pcx_reader_open(BHPcxReader *reader, const char *name) {
    BHPcxReader next;
    unsigned char header[12];
    unsigned long i;
    long end;
    if (!reader || !name || reader->file) {
        if (reader)
            reader->error = BH_PCX_ARGUMENT;
        return 0;
    }
    bh_pcx_reader_init(&next);
    next.file = bh_vfile_open(name, "rb");
    if (!next.file) {
        reader->error = BH_PCX_IO;
        return 0;
    }
    next.error = BH_PCX_IO;
    if (bh_vfile_seek(next.file, 0, SEEK_END) || (end = bh_vfile_tell(next.file)) < 0)
        goto failed;
    if (end < 896) {
        next.error = BH_PCX_TRUNCATED;
        goto failed;
    }
    if ((unsigned long)end > 64UL * 1024 * 1024) {
        next.error = BH_PCX_LIMIT;
        goto failed;
    }
    if (bh_vfile_seek(next.file, 0, SEEK_SET) || bh_vfile_read(header, 1, 12, next.file) != 12)
        goto failed;
    if (header[0] != 10 || header[2] != 1 || header[3] != 8) {
        next.error = BH_PCX_FORMAT;
        goto failed;
    }
    next.width = (unsigned short)((unsigned long)header[8] + ((unsigned long)header[9] << 8) + 1);
    next.height =
        (unsigned short)((unsigned long)header[10] + ((unsigned long)header[11] << 8) + 1);
    if (bh_vfile_seek(next.file, -768, SEEK_END) ||
        bh_vfile_read(next.palette, 1, 768, next.file) != 768 ||
        bh_vfile_seek(next.file, 128, SEEK_SET))
        goto failed;
    next.compressed_left = (unsigned long)end - 896;
    for (i = 0; i < 256; ++i)
        next.map[i] = (unsigned char)i;
    next.error = BH_PCX_OK;
    *reader = next;
    return 1;
failed:
    bh_vfile_close(next.file);
    reader->error = next.error;
    return 0;
}
static int byte(BHPcxReader *reader, unsigned char *value) {
    unsigned long count, got;
    if (reader->cursor == reader->buffered) {
        count = reader->compressed_left > 1024 ? 1024 : reader->compressed_left;
        if (!count) {
            reader->error = BH_PCX_TRUNCATED;
            return 0;
        }
        got = bh_vfile_read(reader->buffer, 1, count, reader->file);
        reader->compressed_left -= got;
        reader->cursor = 0;
        reader->buffered = got;
        if (got != count) {
            reader->error = bh_vfile_error() == BH_VFILE_OK ? BH_PCX_TRUNCATED : BH_PCX_IO;
            return 0;
        }
    }
    *value = reader->buffer[reader->cursor++];
    return 1;
}
int __cdecl bh_pcx_reader_next(BHPcxReader *reader, unsigned char *pixel) {
    unsigned char prefix, value;
    if (!reader || !pixel || !reader->file || reader->error != BH_PCX_OK)
        return 0;
    if (reader->pending) {
        --reader->pending;
        *pixel = reader->map[reader->repeated];
        return 1;
    }
    do {
        if (!byte(reader, &prefix))
            return 0;
    } while (prefix == 0xc0);
    if (prefix < 0xc0) {
        *pixel = reader->map[prefix];
        return 1;
    }
    if (!byte(reader, &value))
        return 0;
    reader->pending = prefix - 0xc1;
    reader->repeated = value;
    *pixel = reader->map[value];
    return 1;
}
