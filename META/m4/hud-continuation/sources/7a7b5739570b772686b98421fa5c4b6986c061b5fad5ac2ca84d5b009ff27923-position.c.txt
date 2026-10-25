#include "../src/bh_position.h"
#include "position_cases.h"
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
            printf("FAIL position line %d: %s\n", line, label);
    }
}
static void put(unsigned char *p, unsigned long value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}
static unsigned long crc(unsigned long value, const BHVec3 *point) {
    const unsigned char *bytes = (const unsigned char *)point;
    unsigned long i, bit;
    value ^= 0xffffffffUL;
    for (i = 0; i < 12; ++i) {
        value ^= bytes[i];
        for (bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ ((value & 1) ? 0xedb88320UL : 0);
    }
    return value ^ 0xffffffffUL;
}
/* Called only after constructing/indexing the actual source scene. */
int bh_position_real_check(const BHNavigation *navigation) {
    unsigned char record[8];
    BHPackedPositions stream;
    BHWorldGeometry *selected;
    BHVec3 point;
    unsigned long i, axis, checksum = 0;
    memset(&stream, 0, sizeof(stream));
    stream.bytes = record;
    stream.size = 8;
    if (navigation->node_count != 1454)
        return 0;
    for (i = 0; i < navigation->node_count; ++i) {
        put(record, i);
        for (axis = 0; axis < 3; ++axis)
            put(record + 2 + axis * 2, (i * 997 + axis * 16384) & 65535);
        stream.cursor = 0;
        if (bh_position_read(&stream, navigation, &selected, &point) != BH_POSITION_OK ||
            selected != navigation->nodes[i].placement || stream.cursor != 8)
            return 0;
        checksum = crc(checksum, &point);
    }
    return checksum == POSITION_REAL_CRC;
}
static void cases(void) {
    BHWorldGeometry world, *selected;
    BHNavigationNode node;
    BHNavigation navigation;
    BHPackedPositions stream, saved;
    BHVec3 point, unchanged = {17, 18, 19};
    unsigned char record[16];
    unsigned long i, checksum = 0, nan = 0x7fc00000UL;
    memset(&world, 0, sizeof(world));
    memset(&navigation, 0, sizeof(navigation));
    memset(&stream, 0, sizeof(stream));
    memset(record, 0, sizeof(record));
    node.placement = &world;
    navigation.nodes = &node;
    navigation.node_count = 1;
    stream.bytes = record;
    stream.size = 8;
    world.minimum.x = -123.125f;
    world.minimum.y = 0.0009765625f;
    world.minimum.z = -10000000000.0f;
    world.maximum.x = 456.75f;
    world.maximum.y = 98765.5f;
    world.maximum.z = 10000001024.0f;
    for (i = 0; i < 65536; ++i) {
        put(record + 2, i);
        put(record + 4, i ^ 0x5555);
        put(record + 6, 65535 - i);
        stream.cursor = 0;
        CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_OK &&
                  selected == &world && stream.cursor == 8,
              "every coordinate word decodes with exact stream consumption");
        checksum = crc(checksum, &point);
    }
    CHECK(checksum == POSITION_SYNTHETIC_CRC, "all coordinate words match independent PC53 bits");
    put(record + 2, 16384);
    put(record + 4, 49152);
    put(record + 6, 32768);
    stream.cursor = 0;
    CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_OK &&
              point.x == world.minimum.x && point.y == world.maximum.y && point.z == 512,
          "minimum maximum midpoint mapping retains signed cancellation");
    for (i = 0; i < 8; ++i) {
        stream.cursor = 0;
        stream.size = i;
        saved = stream;
        selected = NULL;
        point = unchanged;
        CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_ERROR &&
                  !memcmp(&stream, &saved, sizeof(stream)) && !selected &&
                  !memcmp(&point, &unchanged, sizeof(point)),
              "each truncation preserves stream and outputs");
    }
    stream.size = 2;
    put(record, 1);
    stream.accepted = 0;
    stream.rejected = 0xffffffffUL;
    CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_REJECTED &&
              stream.cursor == 2 && stream.accepted == 0xffffffffUL && !stream.rejected && !selected,
          "invalid index consumes one word and wraps original counters");
    stream.cursor = 0;
    stream.size = 8;
    put(record, 0);
    node.placement = NULL;
    saved = stream;
    CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_ERROR &&
              !memcmp(&stream, &saved, sizeof(stream)),
          "missing borrowed placement rejects");
    node.placement = &world;
    memcpy(&world.minimum.x, &nan, 4);
    CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_ERROR &&
              !memcmp(&stream, &saved, sizeof(stream)),
          "nonfinite bounds reject transactionally");
    navigation.node_count = 0;
    stream.size = 2;
    CHECK(bh_position_read(&stream, &navigation, &selected, &point) == BH_POSITION_REJECTED,
          "empty navigation still consumes rejected index");
}
int bh_position_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    *count = checks;
    printf("{\"scenario\":\"position\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":"
           "false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
