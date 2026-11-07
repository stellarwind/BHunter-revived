#ifndef BH_PLATFORM_H
#define BH_PLATFORM_H
#include <stdint.h>
#include "bh_gamepad.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Binding IDs retain original virtual-key values on every platform.
   Text conversion is separate from state; the game's text format is ASCII. */
    typedef enum BHPlatformEventType
    {
        BH_PLATFORM_KEY,
        BH_PLATFORM_FOCUS
    } BHPlatformEventType;
    typedef struct BHPlatformEvent
    {
        BHPlatformEventType type;
        unsigned key;
        int down, repeat;
    } BHPlatformEvent;
    typedef void (*BHPlatformInput)(const BHPlatformEvent *, void *);
    typedef enum BHPlatformTextEvent
    {
        BH_PLATFORM_TEXT_INSERT,
        BH_PLATFORM_TEXT_BACKSPACE,
        BH_PLATFORM_TEXT_SUBMIT,
        BH_PLATFORM_TEXT_CANCEL
    } BHPlatformTextEvent;
    typedef void (*BHPlatformTextInput)(BHPlatformTextEvent, const char *, void *);
#ifdef _WIN32
    void bh_sdl_use_dx12(int enabled);
#endif
    int bh_sdl_open(int hidden, BHPlatformInput, void *);
    int bh_sdl_poll(void);
    int bh_sdl_active(void);
    int bh_sdl_mouse(int *x, int *y);
    int bh_sdl_warp(int x, int y);
    unsigned bh_sdl_key_character(unsigned key);
    int bh_sdl_text_begin(BHPlatformTextInput, void *);
    void bh_sdl_text_end(void);
    void bh_sdl_gamepad(BHGamepadState *);
    /* Milliseconds wrap at 2^32; subtract unsigned values for elapsed intervals.
   The profiler uses monotonic nanoseconds without 32-bit truncation. */
    uint32_t bh_sdl_ticks(void);
    uint64_t bh_sdl_nanoseconds(void);
    void bh_sdl_delay(uint32_t milliseconds);
    double bh_sdl_seconds(void);
    void bh_sdl_pace(void);
    void bh_sdl_close(void);
    const char *bh_sdl_error(void);
    const char *bh_sdl_gpu_driver(void);
    int bh_sdl_clear_frame(void);
    /* Resolve paths before changing cwd. Defaults preserve assets/settings beside
   the executable, independent of launch cwd. Explicit paths are cwd-relative. */
    int bh_sdl_paths(const char *assets, const char *settings);
    const char *bh_sdl_settings_file(const char *name);
    void bh_sdl_report_error(const char *details);
    void bh_sdl_error_dialog(void);
    void bh_sdl_video_cycle(int row);
    const char *bh_sdl_video_label(int row);
    void bh_sdl_canvas_margins(float *x, float *y);
#ifdef __cplusplus
}
#endif
#endif
