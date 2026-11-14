#include "../bh_renderer.h"
#include "bh_atlas_page.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(BHAtlasPage) == 0x28, "BHAtlasPage size");
static_assert(offsetof(BHAtlasPage, last_color) == 0xc, "BHAtlasPage.last_color offset");
static_assert(offsetof(BHAtlasPage, size) == 0x20, "BHAtlasPage.size offset");
static void discard(BHTextureContext *texture_context, BHTextureImage *image)
{
    if (!image)
        return;
    if (image->palette)
        bh_renderer_palette_release(image->palette);
    if (image->source)
        bh_renderer_surface_release(image->source);
    if (image->runs)
        texture_context->allocator.release(image->runs, texture_context->allocator.user);
    if (image->vertices)
        texture_context->allocator.release(image->vertices, texture_context->allocator.user);
    if (image->dither)
        texture_context->allocator.release(image->dither, texture_context->allocator.user);
    if (image->screen)
        texture_context->allocator.release(image->screen, texture_context->allocator.user);
    texture_context->allocator.release(image, texture_context->allocator.user);
}
static int native(BHTextureContext *texture_context, HRESULT hr)
{
    texture_context->last_hresult = hr;
    if (hr != DD_OK)
    {
        texture_context->error = BH_TEXTURE_BACKEND;
        return 0;
    }
    return 1;
}
BHAtlasPage *bh_atlas_page_create(BHAtlasContext *context, const char *name, unsigned long size)
{
    BHTextureContext *texture_context;
    BHTextureImage *image;
    BHAtlasImage *atlas;
    BHAtlasPage *page = NULL;
    DDSURFACEDESC desc;
    PALETTEENTRY entries[256];
    unsigned long i;
    if (!context || !(texture_context = context->textures) || !texture_context->draw || !texture_context->allocator.allocate || !texture_context->allocator.release || !name ||
        !size || size > 4096)
        return NULL;
    texture_context->error = BH_TEXTURE_OK;
    texture_context->last_hresult = DD_OK;
    image = (BHTextureImage *)texture_context->allocator.allocate(sizeof(*image), texture_context->allocator.user);
    if (!image)
    {
        texture_context->error = BH_TEXTURE_MEMORY;
        return NULL;
    }
    memset(image, 0, sizeof(*image));
    image->flags = 1;
    strcpy(image->name, "@@@PAGE@@@");
    image->format = bh_texture_request_format(texture_context, size, size, 8, 1);
    if (!image->format)
        goto failed;
    atlas = bh_atlas_image_load(context, name, &image->format->pixel);
    if (!atlas)
    {
        texture_context->error = context->error;
        goto failed;
    }
    page = (BHAtlasPage *)texture_context->allocator.allocate(sizeof(*page), texture_context->allocator.user);
    if (!page)
    {
        texture_context->error = BH_TEXTURE_MEMORY;
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
    if (!native(texture_context, bh_renderer_surface_create(texture_context->draw, &desc, &image->source)))
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
        if (!native(texture_context, bh_renderer_palette_create(texture_context->draw, entries, &image->palette)) ||
            !native(texture_context, bh_renderer_surface_set_palette(image->source, image->palette)))
            goto failed;
        if (!bh_texture_palette_tables(texture_context, image, (const unsigned char *)entries, 1024))
            goto failed;
    }
    image->runs = (unsigned long *)texture_context->allocator.allocate(0x80, texture_context->allocator.user);
    image->vertices = (D3DTLVERTEX *)texture_context->allocator.allocate(0xc00, texture_context->allocator.user);
    if (!image->runs || !image->vertices)
    {
        texture_context->error = BH_TEXTURE_MEMORY;
        goto failed;
    }
    image->vertex_capacity = 0x60;
    image->next = texture_context->images;
    texture_context->images = image;
    page->image = image;
    page->surface = image->source;
    return page;
failed:
    if (page)
        texture_context->allocator.release(page, texture_context->allocator.user);
    discard(texture_context, image);
    return NULL;
}
int bh_atlas_page_dispose(BHAtlasContext *context, BHAtlasPage *page)
{
    BHTextureContext *texture_context;
    BHTextureImage **at, *image;
    if (!context || !(texture_context = context->textures) || !texture_context->allocator.release || !page || !(image = page->image))
        return 0;
    for (at = &texture_context->images; *at && *at != image; at = &(*at)->next)
    {
    }
    if (!*at)
        return 0;
    *at = image->next;
    if (image->resident && image->resident->image == image)
        image->resident->image = NULL;
    image->resident = NULL;
    discard(texture_context, image);
    texture_context->allocator.release(page, texture_context->allocator.user);
    return 1;
}
