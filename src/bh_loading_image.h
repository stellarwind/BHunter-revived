#ifndef BH_LOADING_IMAGE_H
#define BH_LOADING_IMAGE_H
#include <windows.h>
#include "bh_pcx_reader.h"
typedef enum BHLoadingImageError
{
    BH_LOADING_IMAGE_OK,
    BH_LOADING_IMAGE_ARGUMENT,
    BH_LOADING_IMAGE_MEMORY,
    BH_LOADING_IMAGE_IO,
    BH_LOADING_IMAGE_DIMENSION,
    BH_LOADING_IMAGE_SPAN,
    BH_LOADING_IMAGE_STREAM
} BHLoadingImageError;
typedef struct BHLoadingImages
{
    unsigned char *arena;
    unsigned long arena_bytes;
    BHSceneAllocator allocator;
    unsigned long display_width, pixel_bytes, right[3], left[3]; /* RGB order */
    unsigned char palette[768], map[256];
    unsigned char indexed[256], quarter_indexed[256];
    unsigned long native[256], quarter_native[256];
    unsigned char midas_indexed[256], iel_indexed[256];
    unsigned long midas_native[256], iel_native[256];
    unsigned long frame_stride, deadline;
    long stage, redraw_count;
    unsigned long loads, pixels_read, pixels_written, tick_calls, failures;
    unsigned long last_height, last_offset;
    BHLoadingImageError error;
} BHLoadingImages;
/* Fresh owning-context adapter. It never allocates an arena or fabricates a
   display format. Native right/left shifts are supplied RGB display bindings.
   All tables start zero and default names match original startup.
   Dispose before rebinding an existing owner. */
int __cdecl bh_loading_images_bind(BHLoadingImages *, unsigned long display_width, unsigned long pixel_bytes,
                                   const unsigned long *right, const unsigned long *left, const BHSceneAllocator *);
void __cdecl bh_loading_images_dispose(BHLoadingImages *);
/* Complete00449e50. Destination always stores transformed palette INDICES,
   irrespective of display depth. Width640 mode consumes640*height; all other
   display widths select320*floor(height/2), first horizontal pixel of each pair
   and first row of each pair. Final unpaired odd row is not consumed. Native
   and indexed full/quarter LUTs retain separate original arithmetic. Reader
   ownership is always closed; partial stream failure may leave partial pixels
   but does not publish LUTs/stride. Source guards require source width640 and
   sufficient arena span instead of original unchecked writes. */
int __cdecl bh_loading_image_load(BHLoadingImages *, const char *name, unsigned long offset);
/* Complete0044a160 successful path: allocate0x12c000, MIDAS@96000, retain its
   full LUTs, IEL@e1000 retain LUTs, BAD@0; native tick+5000 wrap, redraw2/stage0.
   Source ownership guard makes replacement transactional: failure releases the
   new allocation and preserves the prior arena/tables, reporting explicit error. */
int __cdecl bh_loading_images_init(BHLoadingImages *);
#endif
