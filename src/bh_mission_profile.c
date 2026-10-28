#include "bh_mission_profile.h"
#ifdef BH_MISSION_PROFILE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define CAPACITY 65536UL
typedef struct BHMissionMarker
{
    __int64 start, end;
    unsigned long frame, value;
    const char *kind;
    char detail[48];
} BHMissionMarker;
/* Single application/render thread only. Ring keeps the most recent events.
   No allocation, formatting or file I/O while capturing. */
static BHMissionMarker markers[CAPACITY];
static LARGE_INTEGER frequency, origin;
static unsigned long written, frame_number;
static int active;
void bh_mp_start(void)
{
    written = frame_number = 0;
    active = QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0 && QueryPerformanceCounter(&origin);
}
void bh_mp_frame(unsigned long frame) { frame_number = frame; }
__int64 bh_mp_begin(void)
{
    LARGE_INTEGER now;
    if (!active || !QueryPerformanceCounter(&now))
        return 0;
    return now.QuadPart;
}
void bh_mp_end(__int64 start, const char *kind, const char *detail, unsigned long value)
{
    LARGE_INTEGER now;
    BHMissionMarker *marker;
    if (!active || !start || !QueryPerformanceCounter(&now))
        return;
    /* Keep callback detail only above 1 ms; avoid flooding the event ring. */
    if (!strncmp(kind, "entity_", 7) && now.QuadPart - start < frequency.QuadPart / 1000)
        return;
    marker = markers + (written++ % CAPACITY);
    marker->start = start;
    marker->end = now.QuadPart;
    marker->frame = frame_number;
    marker->kind = kind;
    marker->value = value;
    strncpy(marker->detail, detail ? detail : "", sizeof(marker->detail) - 1);
    marker->detail[sizeof(marker->detail) - 1] = 0;
}
void bh_mp_save(void)
{
    FILE *file;
    char path[96];
    unsigned long i, first;
    if (!active)
        return;
    active = 0;
    sprintf(path, "mission-markers-%lu-%lu.csv", GetTickCount(), GetCurrentProcessId());
    file = fopen(path, "w");
    if (!file)
        return;
    fprintf(file, "sequence,frame,start_ms,duration_ms,kind,value,detail\n");
    first = written > CAPACITY ? written - CAPACITY : 0;
    for (i = first; i < written; ++i)
    {
        BHMissionMarker *m = markers + (i % CAPACITY);
        fprintf(file, "%lu,%lu,%.6f,%.6f,%s,%lu,\"%s\"\n", i, m->frame,
                (double)(m->start - origin.QuadPart) * 1000.0 / (double)frequency.QuadPart,
                (double)(m->end - m->start) * 1000.0 / (double)frequency.QuadPart,
                m->kind, m->value, m->detail);
    }
    fclose(file);
}
#endif
