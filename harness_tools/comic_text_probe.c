/* Production text-command consumer to CPU-owned surface; no window capture. */
#include "bh_sdl_adapter.h"
#include "bh_gdi_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
    LPDIRECTDRAW draw = NULL;
    LPDIRECTDRAWSURFACE surface = NULL;
    DDSURFACEDESC desc;
    BHFrameDisplay display;
    BHGdiText consumer;
    BHDrawQueue queue;
    unsigned long next;
    char text[59];
    unsigned i, length;
    FILE *file;
    if (argc != 6 || (length = strlen(argv[5])) % 2 || length / 2 > 58)
        return 2;
    for (i = 0; i < length / 2; ++i)
    {
        unsigned code;
        if (sscanf(argv[5] + i * 2, "%2x", &code) != 1)
            return 2;
        text[i] = (char)code;
    }
    text[length / 2] = 0;
    memset(&display, 0, sizeof(display));
    memset(&consumer, 0, sizeof(consumer));
    memset(&queue, 0, sizeof(queue));
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwWidth = 640;
    desc.dwHeight = 96;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = 32;
    desc.ddpfPixelFormat.dwRBitMask = 0xff0000;
    desc.ddpfPixelFormat.dwGBitMask = 0xff00;
    desc.ddpfPixelFormat.dwBBitMask = 255;
    if (DirectDrawCreate(NULL, &draw, NULL) != DD_OK ||
        IDirectDraw_CreateSurface(draw, &desc, &surface, NULL) != DD_OK || !bh_draw_queue_init(&queue, 12, NULL))
        return 3;
    display.color = surface;
    consumer.display = &display;
    if (bh_gdi_text_queue(&queue, (short)atoi(argv[2]), (short)atoi(argv[3]), text, strtoul(argv[4], NULL, 16)) != 1 ||
        !bh_gdi_text_execute(&consumer, queue.commands + 9, &next) || consumer.failures)
        return 4;
    if (IDirectDrawSurface_Lock(surface, NULL, &desc, 0, NULL) != DD_OK)
        return 5;
    file = fopen(argv[1], "wb");
    if (!file)
        return 6;
    for (i = 0; i < 96; ++i)
        if (fwrite((unsigned char *)desc.lpSurface + i * desc.lPitch, 4, 640, file) != 640)
            return 7;
    fclose(file);
    IDirectDrawSurface_Unlock(surface, NULL);
    IDirectDrawSurface_Release(surface);
    IDirectDraw_Release(draw);
    bh_draw_queue_dispose(&queue);
    return 0;
}
