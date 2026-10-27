#include "bh_vfile.h"
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char check_vfile_long[sizeof(long) == 4 ? 1 : -1];
typedef char check_vfile_size[sizeof(BHVFile) == 32 ? 1 : -1];
typedef char check_vfile_memory[offsetof(BHVFile, memory) == 24 ? 1 : -1];
typedef char check_archive_size[sizeof(BHArchiveEntry) == 256 ? 1 : -1];
typedef char check_archive_begin[offsetof(BHArchiveEntry, begin) == 248 ? 1 : -1];

/* The wrapper remains 32 bytes. Private tracking makes teardown explicit. */
typedef struct BHVFileOwner
{
    BHVFile handle;
    struct BHVFileOwner *allocated_next;
    int live;
} BHVFileOwner;

typedef struct BHPreloadedName
{
    char name[248];
    BHVFile *handle;
    struct BHPreloadedName *next;
} BHPreloadedName;
typedef char check_preload_size[sizeof(BHPreloadedName) == 256 ? 1 : -1];

/* Owning definitions for the state observed at 004f95f8..004f9618. */
static struct
{
    long *access_order;
    unsigned int debug;
    long first_access_count;
    BHVFile *recycled;
    BHArchiveEntry *directory;
    BHPreloadedName *preloaded;
    FILE *archive;
    unsigned long count;
    BHVFileProgress progress;
    BHVFileOwner *allocated;
    int callback_depth;
} state;
static BHVFileError last_error;

static unsigned long little_u32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static BHVFileOwner *owner_of(BHVFile *handle)
{
    BHVFileOwner *owner;
    for (owner = state.allocated; owner; owner = owner->allocated_next)
        if (&owner->handle == handle && owner->live)
            return owner;
    last_error = BH_VFILE_ARGUMENT;
    return NULL;
}

static BHVFile *new_handle(void)
{
    BHVFileOwner *owner;
    BHVFile *handle;
    if (state.recycled)
    {
        handle = state.recycled;
        state.recycled = handle->next;
        owner = (BHVFileOwner *)handle;
    }
    else
    {
        owner = (BHVFileOwner *)malloc(sizeof(*owner));
        if (!owner)
        {
            last_error = BH_VFILE_MEMORY;
            return NULL;
        }
        owner->allocated_next = state.allocated;
        state.allocated = owner;
        handle = &owner->handle;
    }
    memset(handle, 0, sizeof(*handle));
    owner->live = 1;
    return handle;
}

static int archive_busy(void)
{
    BHVFileOwner *owner;
    for (owner = state.allocated; owner; owner = owner->allocated_next)
        if (owner->live && owner->handle.archive && !owner->handle.memory)
            return 1;
    return 0;
}

static void notify_access(unsigned long index)
{
    if (state.access_order[index] >= 0)
        return;
    if (state.first_access_count == LONG_MAX)
    {
        last_error = BH_VFILE_OVERFLOW;
        return;
    }
    state.access_order[index] = state.first_access_count++;
    if (state.progress)
    {
        BHVFileError saved = last_error;
        ++state.callback_depth;
        state.progress((float)state.first_access_count / (float)state.count);
        --state.callback_depth;
        last_error = saved;
    }
}

#ifdef DEBUGGING
static void diagnostic(unsigned int flag, const char *backend, const char *name, const char *mode)
{
    if (state.debug & flag)
    {
        printf("VFILE:%s:vopen(%s,%s)\n", backend, name, mode);
        fflush(stdout);
    }
}

#else
#define diagnostic(flag, backend, name, mode) ((void)0)
#endif

/* 0047d270, with validation and transactional ownership on malformed input. */
int __cdecl bh_vfile_mount(const char *path)
{
    FILE *file;
    BHArchiveEntry *directory = NULL;
    long *order = NULL;
    long length;
    unsigned long count, i, header_end;
    unsigned char bytes[256];
    last_error = BH_VFILE_OK;
    if (!path || !*path)
    {
        last_error = BH_VFILE_ARGUMENT;
        return 0;
    }
    if (state.callback_depth || archive_busy())
    {
        last_error = BH_VFILE_BUSY;
        return 0;
    }
    file = fopen(path, "rb");
    if (!file)
    {
        last_error = BH_VFILE_IO;
        return 0;
    }
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 4 || fseek(file, 0, SEEK_SET))
    {
        last_error = BH_VFILE_FORMAT;
        goto failed;
    }
    if (fread(bytes, 1, 4, file) != 4)
    {
        last_error = BH_VFILE_IO;
        goto failed;
    }
    count = little_u32(bytes);
    if (count > ((unsigned long)length - 4) / 256)
    {
        last_error = BH_VFILE_FORMAT;
        goto failed;
    }
    header_end = 4 + count * 256;
    if (count)
    {
        directory = (BHArchiveEntry *)malloc(count * sizeof(*directory));
        order = (long *)malloc(count * sizeof(*order));
        if (!directory || !order)
        {
            last_error = BH_VFILE_MEMORY;
            goto failed;
        }
    }
    for (i = 0; i < count; i++)
    {
        if (fread(bytes, 1, 256, file) != 256)
        {
            last_error = BH_VFILE_IO;
            goto failed;
        }
        if (!bytes[0] || !memchr(bytes, 0, 248))
        {
            last_error = BH_VFILE_FORMAT;
            goto failed;
        }
        memcpy(directory[i].name, bytes, 248);
        directory[i].begin = little_u32(bytes + 248);
        directory[i].end = little_u32(bytes + 252);
        if (directory[i].begin < header_end || directory[i].end < directory[i].begin ||
            directory[i].end > (unsigned long)length)
        {
            last_error = BH_VFILE_FORMAT;
            goto failed;
        }
        order[i] = -1;
    }
    if (state.archive)
        fclose(state.archive);
    free(state.directory);
    free(state.access_order);
    state.archive = file;
    state.directory = directory;
    state.access_order = order;
    state.count = count;
    return 1;
failed:
    free(directory);
    free(order);
    fclose(file);
    return 0;
}

/* 0047cbd0: cached -> loose -> embedded. The open-time fseek quirk is observed. */
BHVFile *__cdecl bh_vfile_open(const char *name, const char *mode)
{
    BHPreloadedName *cached;
    BHVFile *handle;
    FILE *loose;
    unsigned long i;
    last_error = BH_VFILE_OK;
    if (!name || !*name || !mode || !*mode)
    {
        last_error = BH_VFILE_ARGUMENT;
        return NULL;
    }
    for (cached = state.preloaded; cached; cached = cached->next)
    {
        if (!_stricmp(name, cached->name))
        {
            diagnostic(1, "PRELOADED", name, mode);
            cached->handle->position = 0;
            cached->handle->eof = 0;
            return cached->handle;
        }
    }
    loose = fopen(name, mode);
    if (loose)
    {
        if (mode[0] == 'r')
            for (i = 0; i < state.count; i++)
                if (!_stricmp(name, state.directory[i].name))
                {
                    notify_access(i);
                    break;
                }
        diagnostic(4, "EXTERNAL ", name, mode);
        handle = new_handle();
        if (!handle)
        {
            fclose(loose);
            return NULL;
        }
        handle->file = loose;
        return handle;
    }
    if (mode[0] == 'r' && state.archive)
    {
        for (i = 0; i < state.count; i++)
            if (!_stricmp(name, state.directory[i].name))
            {
                notify_access(i);
                diagnostic(2, "EMBEDDED ", name, mode);
                handle = new_handle();
                if (!handle)
                    return NULL;
                handle->file = state.archive;
                handle->archive = 1;
                handle->begin = (long)state.directory[i].begin;
                handle->end = (long)state.directory[i].end;
                handle->position = handle->begin;
                /* Original pushes (start,0,FILE): an ignored invalid-origin seek. */
                fseek(state.archive, 0, (int)handle->begin);
                return handle;
            }
    }
    last_error = BH_VFILE_NOT_FOUND;
    return NULL;
}

int __cdecl bh_vfile_close(BHVFile *file)
{
    BHVFileOwner *owner;
    int result = 0;
    last_error = BH_VFILE_OK;
    owner = owner_of(file);
    if (!owner)
        return EOF;
    if (file->memory)
        return 0;
    if (!file->archive)
        result = fclose(file->file);
    if (result)
        last_error = BH_VFILE_IO;
    owner->live = 0;
    file->next = state.recycled;
    state.recycled = file;
    return result;
}

/* 0047ca70: a cached name owns a persistent shared wrapper and byte payload. */
int __cdecl bh_vfile_preload(const char *name, const char *mode)
{
    BHVFile *file;
    BHPreloadedName *cached;
    unsigned char *data;
    long length;
    BHVFileError error;
    last_error = BH_VFILE_OK;
    if (!name || !*name || strlen(name) >= 248)
    {
        last_error = BH_VFILE_ARGUMENT;
        return 0;
    }
    file = bh_vfile_open(name, mode);
    if (!file)
        return 0;
    if (file->memory)
        return 1;
    if (bh_vfile_seek(file, 0, SEEK_END) || (length = bh_vfile_tell(file)) < 0 || bh_vfile_seek(file, 0, SEEK_SET))
    {
        error = last_error;
        bh_vfile_close(file);
        last_error = error;
        return 0;
    }
    data = (unsigned char *)malloc(length ? length : 1);
    cached = (BHPreloadedName *)malloc(sizeof(*cached));
    if (!data || !cached)
    {
        free(data);
        free(cached);
        bh_vfile_close(file);
        last_error = BH_VFILE_MEMORY;
        return 0;
    }
    if (length && bh_vfile_read(data, (unsigned long)length, 1, file) != 1)
    {
        error = BH_VFILE_IO;
        free(data);
        free(cached);
        bh_vfile_close(file);
        last_error = error;
        return 0;
    }
    if (!file->archive)
    {
        int closed = fclose(file->file);
        file->file = NULL;
        if (closed)
        {
            free(data);
            free(cached);
            ((BHVFileOwner *)file)->live = 0;
            file->next = state.recycled;
            state.recycled = file;
            last_error = BH_VFILE_IO;
            return 0;
        }
    }
    file->begin = 0;
    file->end = length;
    file->position = 0;
    file->eof = 0;
    file->memory = data;
    strcpy(cached->name, name);
    cached->handle = file;
    cached->next = state.preloaded;
    state.preloaded = cached;
    return 1;
}

int __cdecl bh_vfile_eof(BHVFile *file)
{
    last_error = BH_VFILE_OK;
    if (!owner_of(file))
        return 0;
    return file->memory || file->archive ? file->eof : (feof(file->file) ? 0x10 : 0);
}

int __cdecl bh_vfile_seek(BHVFile *file, long offset, int origin)
{
    __int64 target;
    last_error = BH_VFILE_OK;
    if (!owner_of(file))
        return -1;
    if (!file->archive && !file->memory)
    {
        if (fseek(file->file, offset, origin))
        {
            last_error = BH_VFILE_IO;
            return -1;
        }
        return 0;
    }
    target = file->position;
    if (origin == SEEK_SET)
        target = (__int64)file->begin + offset;
    else if (origin == SEEK_CUR)
        target = (__int64)file->position + offset;
    else if (origin == SEEK_END)
        target = (__int64)file->end + offset;
    if (target < file->begin)
        target = file->begin;
    if (target > file->end)
        target = file->end;
    file->position = (long)target;
    file->eof = 0;
    if (!file->memory && fseek(file->file, file->position, SEEK_SET))
    {
        last_error = BH_VFILE_IO;
        return -1;
    }
    return 0;
}

unsigned long __cdecl bh_vfile_read(void *buffer, unsigned long size, unsigned long count, BHVFile *file)
{
    unsigned long requested, bytes, actual;
    last_error = BH_VFILE_OK;
    if (!owner_of(file))
        return 0;
    if (!size || !count)
        return 0;
    if (!buffer)
    {
        last_error = BH_VFILE_ARGUMENT;
        return 0;
    }
    if (count > LONG_MAX / size)
    {
        last_error = BH_VFILE_OVERFLOW;
        return 0;
    }
    if (!file->memory && !file->archive)
    {
        actual = fread(buffer, size, count, file->file);
        if (ferror(file->file))
            last_error = BH_VFILE_IO;
        return actual;
    }
    if (file->eof)
        return 0;
    if (file->position < file->begin || file->position > file->end)
    {
        last_error = BH_VFILE_ARGUMENT;
        return 0;
    }
    requested = size * count;
    bytes = requested;
    if (bytes > (unsigned long)(file->end - file->position))
    {
        bytes = (unsigned long)(file->end - file->position);
        file->eof = 1;
    }
    if (file->memory)
    {
        if (bytes)
            memcpy(buffer, file->memory + file->position, bytes);
        actual = bytes;
    }
    else
    {
        if (fseek(file->file, file->position, SEEK_SET))
        {
            last_error = BH_VFILE_IO;
            return 0;
        }
        actual = fread(buffer, 1, bytes, file->file);
        if (actual != bytes)
        {
            last_error = BH_VFILE_IO;
            file->eof = 1;
        }
    }
    file->position += actual;
    return actual / size;
}

/* 0047d050. Bound the observed read-ahead window to this entry's payload. */
char *__cdecl bh_vfile_gets(char *buffer, unsigned long window, BHVFile *file)
{
    unsigned long wanted, actual, used, i;
    int newline = 0;
    last_error = BH_VFILE_OK;
    if (!owner_of(file))
        return NULL;
    if (!buffer || !window)
    {
        last_error = BH_VFILE_ARGUMENT;
        return NULL;
    }
    if (window > LONG_MAX)
    {
        last_error = BH_VFILE_OVERFLOW;
        return NULL;
    }
    if (!file->memory && !file->archive)
    {
        char *result = fgets(buffer, (int)window, file->file);
        if (ferror(file->file))
            last_error = BH_VFILE_IO;
        return result;
    }
    if (file->position < file->begin || file->position > file->end)
    {
        last_error = BH_VFILE_ARGUMENT;
        return NULL;
    }
    if (file->position == file->end)
        file->eof = 1;
    if (file->eof)
        return NULL;
    wanted = window;
    if (wanted > (unsigned long)(file->end - file->position))
        wanted = (unsigned long)(file->end - file->position);
    if (file->memory)
    {
        memcpy(buffer, file->memory + file->position, wanted);
        actual = wanted;
    }
    else
    {
        if (fseek(file->file, file->position, SEEK_SET))
        {
            last_error = BH_VFILE_IO;
            return NULL;
        }
        actual = fread(buffer, 1, wanted, file->file);
        if (actual != wanted)
            last_error = BH_VFILE_IO;
    }
    if (!actual)
    {
        file->eof = 1;
        return NULL;
    }
    used = actual;
    for (i = 0; i < actual; i++)
        if (buffer[i] == '\r' || buffer[i] == '\n')
        {
            used = i + 1;
            newline = 1;
            if (i < window - 1)
            {
                if (i + 1 < actual && buffer[i + 1] == (char)(0x17 - buffer[i]))
                    ++used;
                buffer[i] = '\n';
                buffer[i + 1] = 0;
            }
            else
                buffer[i] = 0;
            break;
        }
    if (!newline)
        buffer[actual] = 0;
    file->position += used;
    file->eof = (!newline && actual < window) || actual != wanted;
    if (!file->memory && fseek(file->file, file->position, SEEK_SET))
        last_error = BH_VFILE_IO;
    return buffer;
}

long __cdecl bh_vfile_tell(BHVFile *file)
{
    long position;
    last_error = BH_VFILE_OK;
    if (!owner_of(file))
        return -1;
    if (file->memory)
        return file->position;
    position = ftell(file->file);
    if (position < 0)
    {
        last_error = BH_VFILE_IO;
        return -1;
    }
    if (file->archive)
    {
        file->position = position;
        return position - file->begin;
    }
    return position;
}

int __cdecl bh_vfile_detach_archive(void)
{
    int result = 0;
    last_error = BH_VFILE_OK;
    if (state.callback_depth || archive_busy())
    {
        last_error = BH_VFILE_BUSY;
        return 0;
    }
    if (state.archive)
        result = fclose(state.archive);
    state.archive = NULL;
    if (result)
    {
        last_error = BH_VFILE_IO;
        return 0;
    }
    return 1;
}

void __cdecl bh_vfile_shutdown(void)
{
    BHVFileOwner *owner, *next;
    BHPreloadedName *cached, *next_cached;
    if (state.callback_depth)
    {
        last_error = BH_VFILE_BUSY;
        return;
    }
    for (owner = state.allocated; owner; owner = next)
    {
        next = owner->allocated_next;
        if (owner->handle.memory)
            free(owner->handle.memory);
        else if (owner->live && !owner->handle.archive)
            fclose(owner->handle.file);
        free(owner);
    }
    for (cached = state.preloaded; cached; cached = next_cached)
    {
        next_cached = cached->next;
        free(cached);
    }
    if (state.archive)
        fclose(state.archive);
    free(state.directory);
    free(state.access_order);
    memset(&state, 0, sizeof(state));
    last_error = BH_VFILE_OK;
}

void __cdecl bh_vfile_set_progress(BHVFileProgress callback)
{
    state.progress = callback;
}
void __cdecl bh_vfile_set_debug(unsigned int flags)
{
    state.debug = flags;
}
BHVFileError __cdecl bh_vfile_error(void)
{
    return last_error;
}
unsigned long __cdecl bh_vfile_entry_count(void)
{
    return state.count;
}
