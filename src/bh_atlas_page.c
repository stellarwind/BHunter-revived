#include "bh_atlas_page.h"
#include <stddef.h>
#include <string.h>
typedef char page_size[sizeof(BHAtlasPage) == 0x28 ? 1 : -1];
typedef char page_color[offsetof(BHAtlasPage, last_color) == 0xc ? 1 : -1];
typedef char page_extent[offsetof(BHAtlasPage, size) == 0x20 ? 1 : -1];
static void discard(BHTextureContext *q, BHTextureImage *image)
{
    if (!image)
        return;
    if (image->palette)
        IDirectDrawPalette_Release(image->palette);
    if (image->source)
        IDirectDrawSurface_Release(image->source);
    if (image->runs)
        q->allocator.release(image->runs, q->allocator.user);
    if (image->vertices)
        q->allocator.release(image->vertices, q->allocator.user);
    if (image->dither)
        q->allocator.release(image->dither, q->allocator.user);
    if (image->screen)
        q->allocator.release(image->screen, q->allocator.user);
    q->allocator.release(image, q->allocator.user);
}
static int native(BHTextureContext *q, HRESULT hr)
{
    q->last_hresult = hr;
    if (hr != DD_OK)
    {
        q->error = BH_TEXTURE_BACKEND;
        return 0;
    }
    return 1;
}
BHAtlasPage *__cdecl bh_atlas_page_create(BHAtlasContext *context, const char *name, unsigned long size)
{
    BHTextureContext *q;
    BHTextureImage *image;
    BHAtlasImage *atlas;
    BHAtlasPage *page = NULL;
    DDSURFACEDESC desc;
    PALETTEENTRY entries[256];
    unsigned long i;
    if (!context || !(q = context->textures) || !q->draw || !q->allocator.allocate || !q->allocator.release || !name ||
        !size || size > 4096)
        return NULL;
    q->error = BH_TEXTURE_OK;
    q->last_hresult = DD_OK;
    image = (BHTextureImage *)q->allocator.allocate(sizeof(*image), q->allocator.user);
    if (!image)
    {
        q->error = BH_TEXTURE_MEMORY;
        return NULL;
    }
    memset(image, 0, sizeof(*image));
    image->flags = 1;
    strcpy(image->name, "@@@PAGE@@@");
    image->format = bh_texture_request_format(q, size, size, 8, 1);
    if (!image->format)
        goto failed;
    atlas = bh_atlas_image_load(context, name, &image->format->pixel);
    if (!atlas)
    {
        q->error = context->error;
        goto failed;
    }
    page = (BHAtlasPage *)q->allocator.allocate(sizeof(*page), q->allocator.user);
    if (!page)
    {
        q->error = BH_TEXTURE_MEMORY;
        goto failed;
    }
    memset(page, 0, sizeof(*page));
    page->atlas = atlas;
    page->last_color = 0xffffffffUL;
    page->size = size;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x1007;
    desc.dwWidth = desc.dwHeight = size;
    desc.ddsCaps.dwCaps = 0x1800;
    desc.ddpfPixelFormat = image->format->pixel;
    if (!native(q, IDirectDraw_CreateSurface(q->draw, &desc, &image->source, NULL)))
        goto failed;
    if (image->format->pixel.dwRGBBitCount < 9)
    {
        for (i = 0; i < 256; ++i)
        {
            entries[i].peRed = atlas->palette[i * 3];
            entries[i].peGreen = atlas->palette[i * 3 + 1];
            entries[i].peBlue = atlas->palette[i * 3 + 2];
            entries[i].peFlags = 0;
        }
        if (!native(q, IDirectDraw_CreatePalette(q->draw, 0x44, entries, &image->palette, NULL)) ||
            !native(q, IDirectDrawSurface_SetPalette(image->source, image->palette)))
            goto failed;
        if (!bh_texture_palette_tables(q, image, (const unsigned char *)entries, 1024))
            goto failed;
    }
    image->runs = (unsigned long *)q->allocator.allocate(0x80, q->allocator.user);
    image->vertices = (D3DTLVERTEX *)q->allocator.allocate(0xc00, q->allocator.user);
    if (!image->runs || !image->vertices)
    {
        q->error = BH_TEXTURE_MEMORY;
        goto failed;
    }
    image->vertex_capacity = 0x60;
    image->next = q->images;
    q->images = image;
    page->image = image;
    page->surface = image->source;
    return page;
failed:
    if (page)
        q->allocator.release(page, q->allocator.user);
    discard(q, image);
    return NULL;
}
int __cdecl bh_atlas_page_dispose(BHAtlasContext *context, BHAtlasPage *page)
{
    BHTextureContext *q;
    BHTextureImage **at, *image;
    if (!context || !(q = context->textures) || !q->allocator.release || !page || !(image = page->image))
        return 0;
    for (at = &q->images; *at && *at != image; at = &(*at)->next)
    {
    }
    if (!*at)
        return 0;
    *at = image->next;
    if (image->resident && image->resident->image == image)
        image->resident->image = NULL;
    image->resident = NULL;
    discard(q, image);
    q->allocator.release(page, q->allocator.user);
    return 1;
}
