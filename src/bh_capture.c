#include "bh_capture.h"
#include "bh_texture_draw.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
static const unsigned char original_header[54] = {0x42, 0x4d, 0x36, 0x10, 0x0e, 0, 0, 0,    0,    0, 0x36, 0,    0, 0,
                                                  0x28, 0,    0,    0,    0x80, 2, 0, 0,    0xe0, 1, 0,    0,    1, 0,
                                                  0x18, 0,    0,    0,    0,    0, 0, 0x10, 0x0e, 0, 0xc4, 0x0e, 0, 0,
                                                  0xc4, 0x0e, 0,    0,    0,    0, 0, 0,    0,    0, 0,    0};
static int result(BHCapture *c, HRESULT hr)
{
    ++c->native_calls;
    c->last_hresult = hr;
    if (FAILED(hr))
    {
        ++c->failures;
        return 0;
    }
    return 1;
}
static int dimensions(BHCapture *c, long *left, long *bottom, long *width, long *height)
{
    long top, right;
    unsigned long i;
    if (!_finite(c->left) || !_finite(c->right) || !_finite(c->top) || !_finite(c->bottom) || c->left < 0 ||
        c->top < 0 || c->left > 65535 || c->top > 65535 || c->right > 65535 || c->bottom > 65535)
        return 0;
    *left = (long)c->left;
    *bottom = (long)c->bottom;
    right = (long)c->right;
    top = (long)c->top;
    *width = right - *left;
    *height = *bottom - top;
    if (*width <= 0 || *width > 1280 || *height <= 0 ||
        !(c->pixel_bytes == 1 || c->pixel_bytes == 2 || c->pixel_bytes == 4))
        return 0;
    for (i = 0; i < 3; ++i)
        if (c->extract[i] > 31 || c->expand[i] > 31)
            return 0;
    return 1;
}
static int write_pixels(BHCapture *c, const char *path, const unsigned char *pixels, long pitch, int indexed)
{
    unsigned char header[54], row[3840];
    FILE *file;
    long left, bottom, width, height, y, x;
    unsigned long raw, i, part;
    const unsigned char *in;
    int ok = 1;
    if (!path || !pixels || !dimensions(c, &left, &bottom, &width, &height) ||
        pitch < (left + width) * (long)c->pixel_bytes)
        return 0;
    memcpy(header, original_header, 54);
    header[18] = (unsigned char)width;
    header[19] = (unsigned char)(width / 256);
    header[22] = (unsigned char)height;
    header[23] = (unsigned char)(height / 256);
    file = fopen(path, "wb");
    if (!file)
    {
        ++c->failures;
        return 0;
    }
    ++c->writes;
    if (fwrite(header, 54, 1, file) != 1)
        ok = 0;
    for (y = 0; y < height; ++y)
    {
        in = pixels + (bottom - y - 1) * pitch + left * c->pixel_bytes;
        for (x = 0; x < width; ++x, in += c->pixel_bytes)
        {
            raw = 0;
            for (i = 0; i < c->pixel_bytes; ++i)
                raw |= (unsigned long)in[i] << (i * 8);
            if (indexed && c->pixel_bytes == 1)
            {
                part = raw % 42;
                row[x * 3] = (unsigned char)((part % 6) * 255 / 5);
                row[x * 3 + 1] = (unsigned char)((part / 6) * 255 / 6);
                row[x * 3 + 2] = (unsigned char)((raw / 42) * 255 / 5);
            }
            else
                for (i = 0; i < 3; ++i)
                    row[x * 3 + i] = (unsigned char)(((raw & c->masks[i]) >> c->extract[i]) << c->expand[i]);
        }
        if (fwrite(row, width * 3, 1, file) != 1)
            ok = 0;
        ++c->rows;
    }
    if (fclose(file))
        ok = 0;
    if (!ok)
        ++c->failures;
    return ok;
}
int __cdecl bh_capture_software(BHCapture *c, const char *path)
{
    if (!c || !c->present || !c->present->fade)
        return 0;
    return write_pixels(c, path, c->present->fade->pixels, c->present->fade->pitch, 1);
}
int __cdecl bh_capture_write(BHCapture *c, const char *path)
{
    BHTextureContext *textures;
    BHTextureImage *image;
    BHPresentContext *p;
    DDSURFACEDESC desc;
    int ok;
    long left, bottom, width, height;
    if (!c || !(p = c->present) || !p->fade || !p->fade->render || !(textures = p->fade->render->textures))
        return 0;
    if (textures->software)
        return bh_capture_software(c, path);
    if (!textures->device || !p->color || !(c->pixel_bytes == 2 || c->pixel_bytes == 4))
        return 0;
    textures->frame = (long)((unsigned long)textures->frame + 1UL);
    for (image = textures->images; image; image = image->next)
        bh_texture_draw_flush(textures, image, NULL);
    result(c, IDirect3DDevice2_EndScene(textures->device));
    result(c, IDirect3DDevice2_BeginScene(textures->device));
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    if (!result(c, IDirectDrawSurface_Lock(p->color, NULL, &desc, 0, NULL)))
        return 0;
    if (!dimensions(c, &left, &bottom, &width, &height) || (unsigned long)(left + width) > desc.dwWidth ||
        (unsigned long)bottom > desc.dwHeight)
    {
        result(c, IDirectDrawSurface_Unlock(p->color, NULL));
        ++c->failures;
        return 0;
    }
    ok = write_pixels(c, path, (const unsigned char *)desc.lpSurface, desc.lPitch, 0);
    if (!result(c, IDirectDrawSurface_Unlock(p->color, NULL)))
        ok = 0;
    return ok;
}
