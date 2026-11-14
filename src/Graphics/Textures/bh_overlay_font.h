#ifndef BH_OVERLAY_FONT_H
#define BH_OVERLAY_FONT_H
/* Cropped glyph bitmap plus original TextOutA bearings and advance. */
typedef struct BHOverlayGlyph
{
    unsigned short x, y, width, height;
    short offset_x, offset_y, advance, reserved;
} BHOverlayGlyph;
#endif
