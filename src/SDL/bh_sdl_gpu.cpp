// Native BH renderer resources backed by SDL GPU. The platform owns the window.
#include "../Graphics/bh_renderer.h"
#include "bh_sdl_gpu.h"
#include "bh_sdl_diagnostics.h"
#include "../Graphics/bh_video_layout.h"
#include "bh_sdl_text.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include "../shaders/bh_gpu_shaders.h"
#include <map>
#include <memory>
#include <vector>

struct BHRenderObject
{
    unsigned refs = 1;
    virtual ~BHRenderObject() = default;
};
struct BHRenderPalette : BHRenderObject
{
    PALETTEENTRY entries[256]{};
};
struct BHRenderSurface : BHRenderObject
{
    DDSURFACEDESC desc{};
    std::vector<unsigned char> pixels, locked_pixels;
    BHRenderSurface *text_overlay = nullptr;
    BHRenderPalette *palette = nullptr;
    BHRenderSurface *back = nullptr;
    SDL_GPUTexture *gpu = nullptr;
    unsigned version = 1, uploaded = 0;
    bool target = false, primary = false, keyed = false;
    ~BHRenderSurface();
};
struct BHRenderer : BHRenderObject
{
    SDL_GPUDevice *gpu; // Borrowed; the SDL platform owns GPU shutdown.
};
struct BHRenderDevice : BHRenderObject
{
    BHRenderSurface *target;
    BHRenderer *renderer;
    ~BHRenderDevice();
};

namespace
{
using bh_sdl_diagnostics::set_error;
using Surface = BHRenderSurface;
using Palette = BHRenderPalette;
struct Vertex
{
    float x, y, z, w, u, v;
    unsigned color, specular;
};
struct Batch
{
    unsigned first, count;
    Surface *texture;
    unsigned blend, src, dst, depth, write, key, fog, fog_color, cull;
};
SDL_Window *output_window; // Borrowed from the platform until renderer shutdown.
bh_sdl_gpu::Display (*query_display)();
SDL_GPUDevice *device;
SDL_GPUShader *vs, *fs;
SDL_GPUSampler *sampler;
SDL_GPUTexture *color_target, *depth_target;
unsigned target_w, target_h;
bool targets_fresh = false;
float applied_margin_x = -1, applied_margin_y = -1;
int applied_aspect = -1, applied_mode = -1;
SDL_GPUCommandBuffer *commands;
SDL_GPURenderPass *pass;
std::map<unsigned long long, SDL_GPUGraphicsPipeline *> pipelines;
std::vector<Vertex> vertices;
std::vector<Batch> batches;
std::vector<Surface *> surfaces;
std::array<unsigned, 256> states{};
Surface *white;
void viewport_geometry(float &x, float &y, float &w, float &h, float &scale)
{
    const auto display = query_display();
    const float mx = display.margin_x, my = display.margin_y;
    w = 640 + 2 * mx;
    h = 480 + 2 * my;
    scale = std::min(target_w / w, target_h / h);
    w *= scale;
    h *= scale;
    x = (target_w - w) * .5f;
    y = (target_h - h) * .5f;
}
unsigned addref(BHRenderObject *p)
{
    return p ? ++p->refs : 0;
}
unsigned release(BHRenderObject *p)
{
    if (!p)
        return 0;
    auto *o = p;
    unsigned n = --o->refs;
    if (!n)
        delete o;
    return n;
}
DDPIXELFORMAT pixel_format(unsigned bits = 16)
{
    DDPIXELFORMAT p{};
    p.dwSize = sizeof(p);
    p.dwFlags = bits == 8 ? DDPF_RGB | DDPF_PALETTEINDEXED8 : DDPF_RGB;
    p.dwRGBBitCount = bits;
    if (bits == 16)
    {
        p.dwRBitMask = 0xf800;
        p.dwGBitMask = 0x7e0;
        p.dwBBitMask = 31;
    }
    if (bits == 32)
    {
        p.dwRBitMask = 0xff0000;
        p.dwGBitMask = 0xff00;
        p.dwBBitMask = 255;
    }
    return p;
}
SDL_GPUTexture *make_texture(unsigned w, unsigned h, SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage)
{
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.usage = usage;
    info.width = w;
    info.height = h;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    return SDL_CreateGPUTexture(device, &info);
}
void end_pass()
{
    if (pass)
    {
        SDL_EndGPURenderPass(pass);
        pass = nullptr;
    }
}
bool ensure_commands()
{
    if (!commands)
    {
        commands = SDL_AcquireGPUCommandBuffer(device);
        if (!commands)
            set_error("Acquire GPU commands");
    }
    return commands != nullptr;
}
bool ensure_targets()
{
    const auto display = query_display();
    int w = display.width, h = display.height;
    float limit = std::min(1.f, std::min(3840.f / w, 2160.f / h));
    /* The game uses x87 PC53. Some i386 libcs assume extended precision in lroundf. */
    w = std::max(1, (int)(w * limit + .5f));
    h = std::max(1, (int)(h * limit + .5f));
    if (color_target && target_w == (unsigned)w && target_h == (unsigned)h)
        return true;
    end_pass();
    if (color_target)
        SDL_ReleaseGPUTexture(device, color_target);
    if (depth_target)
        SDL_ReleaseGPUTexture(device, depth_target);
    target_w = w;
    target_h = h;
    targets_fresh = true;
    color_target = make_texture(w, h, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!color_target)
        set_error("Create GPU color target");
    depth_target = make_texture(w, h, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
    if (!depth_target)
        set_error("Create GPU depth target");
    return color_target && depth_target;
}
bool begin_pass(SDL_GPULoadOp color_load = SDL_GPU_LOADOP_LOAD, SDL_GPULoadOp depth_load = SDL_GPU_LOADOP_LOAD,
                SDL_FColor clear = {0, 0, 0, 1})
{
    if (!ensure_commands() || !ensure_targets())
        return false;
    const auto display = query_display();
    const float mx = display.margin_x, my = display.margin_y;
    if (mx != applied_margin_x || my != applied_margin_y || display.aspect != applied_aspect ||
        display.borderless != applied_mode)
        targets_fresh = true;
    if (pass && !targets_fresh && color_load == SDL_GPU_LOADOP_LOAD && depth_load == SDL_GPU_LOADOP_LOAD)
        return true;
    if (targets_fresh)
    {
        if (color_load != SDL_GPU_LOADOP_CLEAR)
            clear = {0, 0, 0, 1};
        color_load = depth_load = SDL_GPU_LOADOP_CLEAR;
    }
    end_pass();
    SDL_GPUColorTargetInfo c{};
    c.texture = color_target;
    c.load_op = color_load;
    c.store_op = SDL_GPU_STOREOP_STORE;
    c.clear_color = clear;
    SDL_GPUDepthStencilTargetInfo d{};
    d.texture = depth_target;
    d.load_op = depth_load;
    d.store_op = SDL_GPU_STOREOP_STORE;
    d.clear_depth = 0;
    d.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    d.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    pass = SDL_BeginGPURenderPass(commands, &c, 1, &d);
    if (!pass)
    {
        set_error("Begin GPU render pass");
        return false;
    }
    targets_fresh = false;
    applied_margin_x = mx;
    applied_margin_y = my;
    applied_aspect = display.aspect;
    applied_mode = display.borderless;
    float x, y, w, h, scale;
    viewport_geometry(x, y, w, h, scale);
    SDL_GPUViewport viewport{x, y, w, h, 0, 1};
    SDL_SetGPUViewport(pass, &viewport);
    return true;
}
SDL_GPUBlendFactor blend_factor(unsigned f)
{
    switch (f)
    {
    case D3DBLEND_ZERO:
        return SDL_GPU_BLENDFACTOR_ZERO;
    case D3DBLEND_ONE:
        return SDL_GPU_BLENDFACTOR_ONE;
    case D3DBLEND_SRCCOLOR:
        return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
        return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case D3DBLEND_DESTALPHA:
        return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case D3DBLEND_INVDESTALPHA:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case D3DBLEND_DESTCOLOR:
        return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    default:
        return SDL_GPU_BLENDFACTOR_ONE;
    }
}
SDL_GPUGraphicsPipeline *pipeline(const Batch &b)
{
    unsigned long long key = b.blend | (b.src << 1) | (b.dst << 5) | (b.depth << 9) | (b.write << 13) | (b.cull << 14);
    auto found = pipelines.find(key);
    if (found != pipelines.end())
        return found->second;
    SDL_GPUVertexBufferDescription buffer{};
    buffer.slot = 0;
    buffer.pitch = sizeof(Vertex);
    buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexAttribute attributes[4] = {{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0},
                                            {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 16},
                                            {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24},
                                            {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28}};
    SDL_GPUColorTargetDescription color{};
    color.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    color.blend_state.enable_blend = b.blend;
    color.blend_state.src_color_blendfactor = blend_factor(b.src);
    color.blend_state.dst_color_blendfactor = blend_factor(b.dst);
    color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vs;
    info.fragment_shader = fs;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.vertex_input_state = {&buffer, 1, attributes, 4};
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = b.cull == D3DCULL_NONE || !b.cull ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face =
        b.cull == D3DCULL_CW ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE : SDL_GPU_FRONTFACE_CLOCKWISE;
    info.rasterizer_state.enable_depth_clip = true;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = b.write;
    info.depth_stencil_state.compare_op =
        b.depth == D3DCMP_ALWAYS ? SDL_GPU_COMPAREOP_ALWAYS : SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    info.target_info.has_depth_stencil_target = true;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    auto *p = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!p)
        set_error("Create pipeline");
    else
        pipelines[key] = p;
    return p;
}
unsigned rgba_pixel(const Surface *s, unsigned x, unsigned y)
{
    unsigned value = 0, bytes = (s->desc.ddpfPixelFormat.dwRGBBitCount + 7) / 8;
    memcpy(&value, &s->pixels[y * s->desc.lPitch + x * bytes], bytes);
    unsigned r, g, b;
    if (bytes == 1)
    {
        auto c = s->palette ? s->palette->entries[value] : PALETTEENTRY{};
        r = c.peRed;
        g = c.peGreen;
        b = c.peBlue;
    }
    else
    {
        auto &p = s->desc.ddpfPixelFormat;
        auto channel = [&](unsigned mask) {
            if (!mask)
                return 0u;
            unsigned shift = 0;
            while (!(mask & 1))
            {
                mask >>= 1;
                ++shift;
            }
            return ((value >> shift) & mask) * 255 / mask;
        };
        r = channel(p.dwRBitMask);
        g = channel(p.dwGBitMask);
        b = channel(p.dwBBitMask);
    }
    return r | (g << 8) | (b << 16) | ((s->keyed && value == 0) ? 0 : 0xff000000u);
}
bool upload(Surface *s)
{
    if (s->uploaded == s->version && s->gpu)
        return true;
    end_pass();
    if (!ensure_commands())
        return false;
    if (!s->gpu)
        s->gpu = make_texture(s->desc.dwWidth, s->desc.dwHeight, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                              SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!s->gpu)
        return false;
    unsigned size = s->desc.dwWidth * s->desc.dwHeight * 4;
    SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, size, 0};
    auto *t = SDL_CreateGPUTransferBuffer(device, &info);
    if (!t)
        return false;
    auto *pixels = (unsigned *)SDL_MapGPUTransferBuffer(device, t, false);
    if (!pixels)
    {
        SDL_ReleaseGPUTransferBuffer(device, t);
        return false;
    }
    for (unsigned y = 0; y < s->desc.dwHeight; ++y)
        for (unsigned x = 0; x < s->desc.dwWidth; ++x)
            pixels[y * s->desc.dwWidth + x] = rgba_pixel(s, x, y);
    SDL_UnmapGPUTransferBuffer(device, t);
    auto *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureTransferInfo source{t, 0, s->desc.dwWidth, s->desc.dwHeight};
    SDL_GPUTextureRegion dest{s->gpu, 0, 0, 0, 0, 0, s->desc.dwWidth, s->desc.dwHeight, 1};
    SDL_UploadToGPUTexture(copy, &source, &dest, true);
    SDL_EndGPUCopyPass(copy);
    SDL_ReleaseGPUTransferBuffer(device, t);
    s->uploaded = s->version;
    return true;
}
bool record_pending_draws()
{
    if (vertices.empty())
        return true;
    end_pass();
    if (!ensure_commands() || !ensure_targets())
        return false;
    unsigned size = vertices.size() * sizeof(Vertex);
    SDL_GPUTransferBufferCreateInfo tinfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, size, 0};
    // unique_ptr calls this lambda to release the transfer buffer when the function exits,
    // including on early returns.
    auto release_transfer = [gpu = device](SDL_GPUTransferBuffer *handle) {
        SDL_ReleaseGPUTransferBuffer(gpu, handle);
    };
    std::unique_ptr<SDL_GPUTransferBuffer, decltype(release_transfer)> transfer(
        SDL_CreateGPUTransferBuffer(device, &tinfo), release_transfer);
    if (!transfer)
        return false;
    SDL_GPUBufferCreateInfo binfo{SDL_GPU_BUFFERUSAGE_VERTEX, size, 0};
    // unique_ptr calls this lambda to release the vertex buffer when the function exits,
    // including on early returns.
    auto release_buffer = [gpu = device](SDL_GPUBuffer *handle) { SDL_ReleaseGPUBuffer(gpu, handle); };
    std::unique_ptr<SDL_GPUBuffer, decltype(release_buffer)> buffer(SDL_CreateGPUBuffer(device, &binfo), release_buffer);
    if (!buffer)
        return false;
    auto *mapped = SDL_MapGPUTransferBuffer(device, transfer.get(), false);
    if (!mapped)
        return false;
    memcpy(mapped, vertices.data(), size);
    SDL_UnmapGPUTransferBuffer(device, transfer.get());
    auto *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTransferBufferLocation src{transfer.get(), 0};
    SDL_GPUBufferRegion dst{buffer.get(), 0, size};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    const auto display = query_display();
    const float mx = display.margin_x, my = display.margin_y;
    float canvas[4] = {320 + mx, 240 + my, 0, 0};
    SDL_PushGPUVertexUniformData(commands, 0, canvas, sizeof(canvas));
    for (auto &b : batches)
    {
        Surface *s = b.texture ? b.texture : white;
        if (!upload(s) || !begin_pass())
            return false;
        auto *p = pipeline(b);
        if (!p)
            return false;
        SDL_BindGPUGraphicsPipeline(pass, p);
        SDL_GPUBufferBinding binding{buffer.get(), 0};
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
        SDL_GPUTextureSamplerBinding texture{s->gpu, sampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
        float uniforms[8] = {(float)b.key,
                             (float)b.fog,
                             0,
                             0,
                             ((b.fog_color >> 16) & 255) / 255.f,
                             ((b.fog_color >> 8) & 255) / 255.f,
                             (b.fog_color & 255) / 255.f,
                             1};
        SDL_PushGPUFragmentUniformData(commands, 0, uniforms, sizeof(uniforms));
        SDL_DrawGPUPrimitives(pass, b.count, 1, b.first, 0);
    }
    vertices.clear();
    batches.clear();
    return true;
}
bool clear_target(bool color, bool depth, unsigned value = 0)
{
    if (!record_pending_draws())
        return false;
    SDL_FColor c{((value >> 16) & 255) / 255.f, ((value >> 8) & 255) / 255.f, (value & 255) / 255.f, 1};
    return begin_pass(color ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
                      depth ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD, c);
}
bool draw_surface(Surface *s, bool overlay = false)
{
    if (!upload(s))
        return false;
    D3DTLVERTEX v[4]{};
    float xy[8] = {0, 0, 640, 0, 640, 480, 0, 480};
    for (int i = 0; i < 4; ++i)
    {
        v[i].sx = xy[i * 2];
        v[i].sy = xy[i * 2 + 1];
        v[i].sz = 1;
        v[i].rhw = 1;
        v[i].color = 0xffffffff;
        v[i].tu = (i == 1 || i == 2);
        v[i].tv = (i >= 2);
    }
    auto saved = states;
    states[D3DRENDERSTATE_TEXTUREHANDLE] = (DWORD)s;
    states[D3DRENDERSTATE_ALPHABLENDENABLE] = 0;
    states[D3DRENDERSTATE_COLORKEYENABLE] = overlay;
    states[D3DRENDERSTATE_ZFUNC] = D3DCMP_ALWAYS;
    states[D3DRENDERSTATE_ZWRITEENABLE] = 0;
    states[D3DRENDERSTATE_CULLMODE] = D3DCULL_NONE;
    states[D3DRENDERSTATE_FOGENABLE] = 0;
    HRESULT hr = bh_renderer_draw_fan(nullptr, v, 4);
    states = saved;
    return SUCCEEDED(hr) && record_pending_draws();
}
void present_target(SDL_GPUTexture *destination, unsigned width, unsigned height)
{
    SDL_GPUBlitInfo blit{};
    blit.source.texture = color_target;
    blit.source.w = target_w;
    blit.source.h = target_h;
    blit.destination.texture = destination;
    video_layout_fit(target_w, target_h, width, height, &blit.destination.x, &blit.destination.y, &blit.destination.w,
                     &blit.destination.h);
    blit.load_op = SDL_GPU_LOADOP_CLEAR;
    blit.clear_color = {0, 0, 0, 1};
    blit.filter = SDL_GPU_FILTER_LINEAR;
    SDL_BlitGPUTexture(commands, &blit);
}
bool present()
{
    if (!record_pending_draws() || !ensure_commands() || !ensure_targets())
        return false;
    end_pass();
    SDL_GPUTexture *swap = nullptr;
    Uint32 w, h;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, output_window, &swap, &w, &h))
    {
        set_error("Acquire swapchain");
        return false;
    }
    if (swap)
        present_target(swap, w, h);
    bool ok = SDL_SubmitGPUCommandBuffer(commands);
    commands = nullptr;
    if (!ok)
        set_error("Submit GPU commands");
    return ok;
}
std::vector<unsigned> readback()
{
    if (!record_pending_draws() || !ensure_commands() || !ensure_targets())
        return {};
    end_pass();
    unsigned size = target_w * target_h * 4;
    SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, size, 0};
    auto *t = SDL_CreateGPUTransferBuffer(device, &info);
    if (!t)
        return {};
    auto *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion src{color_target, 0, 0, 0, 0, 0, target_w, target_h, 1};
    SDL_GPUTextureTransferInfo dst{t, 0, target_w, target_h};
    SDL_DownloadFromGPUTexture(copy, &src, &dst);
    SDL_EndGPUCopyPass(copy);
    auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    commands = nullptr;
    if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
    {
        SDL_ReleaseGPUTransferBuffer(device, t);
        return {};
    }
    SDL_ReleaseGPUFence(device, fence);
    auto *p = (unsigned *)SDL_MapGPUTransferBuffer(device, t, false);
    std::vector<unsigned> result;
    if (p)
    {
        result.assign(p, p + target_w * target_h);
        SDL_UnmapGPUTransferBuffer(device, t);
    }
    SDL_ReleaseGPUTransferBuffer(device, t);
    return result;
}
SDL_GPUShader *shader(const Uint8 *code, size_t size, const char *entry, SDL_GPUShaderStage stage)
{
    SDL_GPUShaderCreateInfo info{};
    info.code = code;
    info.code_size = size;
    info.entrypoint = entry;
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;
    info.num_samplers = stage == SDL_GPU_SHADERSTAGE_FRAGMENT ? 1 : 0;
    info.num_uniform_buffers = 1;
    return SDL_CreateGPUShader(device, &info);
}
Surface *make_surface(const DDSURFACEDESC &d)
{
    auto *s = new Surface;
    s->desc = d;
    s->primary = (d.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) != 0;
    if (!s->desc.dwWidth)
        s->desc.dwWidth = 640;
    if (!s->desc.dwHeight)
        s->desc.dwHeight = 480;
    if (!s->desc.ddpfPixelFormat.dwRGBBitCount)
        s->desc.ddpfPixelFormat = pixel_format();
    s->desc.lPitch = s->desc.dwWidth * ((s->desc.ddpfPixelFormat.dwRGBBitCount + 7) / 8);
    s->pixels.resize(s->desc.lPitch * s->desc.dwHeight);
    s->keyed = (d.dwFlags & DDSD_CKSRCBLT) != 0;
    s->target = (d.ddsCaps.dwCaps & DDSCAPS_3DDEVICE) != 0;
    surfaces.push_back(s);
    return s;
}
} // namespace

BHRenderSurface::~BHRenderSurface()
{
    release(text_overlay);
    surfaces.erase(std::remove(surfaces.begin(), surfaces.end(), this), surfaces.end());
    if (gpu && device)
        SDL_ReleaseGPUTexture(device, gpu);
    release(palette);
    release(back);
}
BHRenderDevice::~BHRenderDevice()
{
    release(target);
    release(renderer);
}

namespace bh_sdl_gpu
{
bool open(SDL_Window *output, Display (*display)())
{
    output_window = output;
    query_display = display;
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    if (!device || !SDL_ClaimWindowForGPUDevice(device, output_window))
    {
        set_error("Create Vulkan device");
        return false;
    }
    SDL_SetGPUSwapchainParameters(device, output_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
    SDL_SetGPUAllowedFramesInFlight(device, 1);
    vs = shader(spirv_vertex, sizeof(spirv_vertex), "VSMain", SDL_GPU_SHADERSTAGE_VERTEX);
    if (vs)
        fs = shader(spirv_fragment, sizeof(spirv_fragment), "PSMain", SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (!vs || !fs)
    {
        set_error("Create shaders");
        return false;
    }
    SDL_GPUSamplerCreateInfo sinfo{};
    sinfo.min_filter = sinfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    sinfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sinfo.address_mode_u = sinfo.address_mode_v = sinfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler = SDL_CreateGPUSampler(device, &sinfo);
    DDSURFACEDESC desc{};
    desc.dwWidth = desc.dwHeight = 1;
    desc.ddpfPixelFormat = pixel_format(32);
    white = make_surface(desc);
    memset(white->pixels.data(), 255, 4);
    return true;
}

void reset_draw_state()
{
    states.fill(0);
    targets_fresh = true;
}

void close()
{
    record_pending_draws();
    end_pass();
    if (commands)
    {
        SDL_SubmitGPUCommandBuffer(commands);
        commands = nullptr;
    }
    if (device)
    {
        SDL_WaitForGPUIdle(device);
        release(white);
        white = nullptr;
        for (auto &p : pipelines)
            SDL_ReleaseGPUGraphicsPipeline(device, p.second);
        pipelines.clear();
        if (color_target)
            SDL_ReleaseGPUTexture(device, color_target);
        if (depth_target)
            SDL_ReleaseGPUTexture(device, depth_target);
        if (vs)
            SDL_ReleaseGPUShader(device, vs);
        if (fs)
            SDL_ReleaseGPUShader(device, fs);
        if (sampler)
            SDL_ReleaseGPUSampler(device, sampler);
        if (output_window)
            SDL_ReleaseWindowFromGPUDevice(device, output_window);
        SDL_DestroyGPUDevice(device);
        device = nullptr;
    }
    color_target = depth_target = nullptr;
    vs = fs = nullptr;
    sampler = nullptr;
    applied_margin_x = applied_margin_y = -1;
    applied_aspect = applied_mode = -1;
    output_window = nullptr;
    query_display = nullptr;
}
} // namespace bh_sdl_gpu

extern "C"
{
    int bh_renderer_surface_text(BHRenderSurface *surface, int x, int y, const char *text, unsigned long rgb)
    {
        auto *destination = surface;
        if (!destination || !text || !memchr(text, 0, 59))
            return 0;
        if (!bh_sdl_text::prepare())
            return 0;
        Surface *output = destination;
        if (destination->target)
        {
            if (!destination->text_overlay)
            {
                DDSURFACEDESC desc{};
                desc.dwWidth = 640;
                desc.dwHeight = 480;
                desc.ddpfPixelFormat = pixel_format(32);
                desc.dwFlags = DDSD_CKSRCBLT;
                destination->text_overlay = make_surface(desc);
            }
            output = destination->text_overlay;
        }
        /* Flush earlier draws before changing CPU storage. This also keeps
           a texture used by an earlier batch coherent with subsequent uploads. */
        if (!record_pending_draws())
            return 0;
        if (destination->target)
            std::fill(output->pixels.begin(), output->pixels.end(), 0);
        std::array<unsigned char, 768> palette{};
        if (output->palette)
            for (unsigned i = 0; i < 256; ++i)
            {
                palette[i * 3] = output->palette->entries[i].peRed;
                palette[i * 3 + 1] = output->palette->entries[i].peGreen;
                palette[i * 3 + 2] = output->palette->entries[i].peBlue;
            }
        const auto &format = output->desc.ddpfPixelFormat;
        BHOverlayCanvas canvas{output->pixels.data(),
                               output->desc.dwWidth,
                               output->desc.dwHeight,
                               (unsigned)output->desc.lPitch,
                               (format.dwRGBBitCount + 7) / 8,
                               {format.dwRBitMask, format.dwGBitMask, format.dwBBitMask},
                               output->palette ? palette.data() : nullptr};
        if (!bh_sdl_text::rasterize(canvas, x, y, text, rgb))
            return 0;
        ++output->version;
        return destination->target ? draw_surface(output, true) : 1;
    }
    const char *bh_sdl_gpu_driver()
    {
        const char *driver = device ? SDL_GetGPUDeviceDriver(device) : nullptr;
        return driver ? driver : "unavailable";
    }
    int bh_sdl_clear_frame()
    {
        return clear_target(true, true);
    }
    HRESULT __cdecl bh_renderer_create(BHRenderer **out)
    {
        if (!out)
            return E_INVALIDARG;
        auto *renderer = new BHRenderer;
        renderer->gpu = device;
        *out = renderer;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_device_create(BHRenderer *self, BHRenderSurface *surface, BHRenderDevice **device)
    {
        if (!self || !surface || !device)
            return E_INVALIDARG;
        surface->target = true;
        auto *binding = new BHRenderDevice;
        binding->target = surface;
        binding->renderer = self;
        addref(surface);
        addref(self);
        *device = binding;
        return DD_OK;
    }
    ULONG __cdecl bh_renderer_device_retain(BHRenderDevice *self)
    {
        return addref(self);
    }
    HRESULT __cdecl bh_renderer_draw_fan(BHRenderDevice *self, const D3DTLVERTEX *vertices, DWORD vertex_count)
    {
        if (!vertices || vertex_count < 3 || vertex_count > 65536)
            return E_INVALIDARG;
        auto *v = vertices;
        Batch b{};
        b.first = ::vertices.size();
        b.texture = (Surface *)states[D3DRENDERSTATE_TEXTUREHANDLE];
        b.blend = states[D3DRENDERSTATE_ALPHABLENDENABLE];
        b.src = states[D3DRENDERSTATE_SRCBLEND];
        b.dst = states[D3DRENDERSTATE_DESTBLEND];
        b.depth = states[D3DRENDERSTATE_ZFUNC];
        b.write = states[D3DRENDERSTATE_ZWRITEENABLE];
        b.key = states[D3DRENDERSTATE_COLORKEYENABLE] && b.texture && b.texture->keyed;
        b.fog = states[D3DRENDERSTATE_FOGENABLE];
        b.fog_color = states[D3DRENDERSTATE_FOGCOLOR];
        b.cull = states[D3DRENDERSTATE_CULLMODE];
        for (unsigned i = 1; i + 1 < vertex_count; ++i)
            for (unsigned j : {0u, i, i + 1})
            {
                auto &p = v[j];
                ::vertices.push_back({p.sx, p.sy, p.sz, p.rhw, p.tu, p.tv, p.color, p.specular});
            }
        b.count = ::vertices.size() - b.first;
        batches.push_back(b);
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_record_draws(BHRenderDevice *self)
    {
        return record_pending_draws() ? DD_OK : E_FAIL;
    }
    HRESULT __cdecl bh_renderer_texture_formats(BHRenderDevice *self, LPD3DENUMTEXTUREFORMATSCALLBACK cb, void *ctx)
    {
        DDSURFACEDESC d{};
        d.dwSize = sizeof(d);
        for (unsigned bits : {8u, 16u, 32u})
        {
            d.ddpfPixelFormat = pixel_format(bits);
            if (cb(&d, ctx) == D3DENUMRET_CANCEL)
                break;
        }
        return DD_OK;
    }
    ULONG __cdecl bh_renderer_device_release(BHRenderDevice *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_renderer_set_state(BHRenderDevice *self, D3DRENDERSTATETYPE dwRenderStateType,
                                          DWORD dwRenderState)
    {
        if ((unsigned)dwRenderStateType >= states.size())
            return E_INVALIDARG;
        states[dwRenderStateType] = dwRenderState;
        return DD_OK;
    }
    ULONG __cdecl bh_renderer_release(BHRenderer *self)
    {
        return release(self);
    }
    ULONG __cdecl bh_renderer_surface_release(BHRenderSurface *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_renderer_surface_texture_handle(BHRenderSurface *self, D3DTEXTUREHANDLE *handle)
    {
        *handle = (DWORD)self;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_palette_get(BHRenderPalette *self, DWORD dwBase, DWORD dwNumEntries,
                                            LPPALETTEENTRY lpEntries)
    {
        if (dwBase + dwNumEntries > 256)
            return E_INVALIDARG;
        memcpy(lpEntries, (self)->entries + dwBase, dwNumEntries * sizeof(PALETTEENTRY));
        return DD_OK;
    }
    ULONG __cdecl bh_renderer_palette_release(BHRenderPalette *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_renderer_palette_set(BHRenderPalette *self, DWORD dwStartingEntry, DWORD dwCount,
                                            LPPALETTEENTRY lpEntries)
    {
        if (dwStartingEntry + dwCount > 256)
            return E_INVALIDARG;
        if (!record_pending_draws())
            return E_FAIL;
        memcpy((self)->entries + dwStartingEntry, lpEntries, dwCount * sizeof(PALETTEENTRY));
        for (auto *s : surfaces)
            if (s->palette == self)
                ++s->version;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_attach_depth(BHRenderSurface *self, BHRenderSurface *lpDDSAttachedSurface)
    {
        return DD_OK;
    }
    ULONG __cdecl bh_renderer_surface_retain(BHRenderSurface *self)
    {
        return addref(self);
    }
    HRESULT __cdecl bh_renderer_surface_blit(BHRenderSurface *self, LPRECT lpDestRect, BHRenderSurface *lpDDSrcSurface,
                                             LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
    {
        auto *dst = self, *src = lpDDSrcSurface;
        if (dst->primary)
        {
            if (src && !src->target)
            {
                if (!clear_target(true, true) || !upload(src) || !begin_pass())
                    return E_FAIL;
                D3DTLVERTEX v[4]{};
                float xy[8] = {0, 0, 640, 0, 640, 480, 0, 480};
                for (int i = 0; i < 4; ++i)
                {
                    v[i].sx = xy[i * 2];
                    v[i].sy = xy[i * 2 + 1];
                    v[i].sz = 1;
                    v[i].rhw = 1;
                    v[i].color = 0xffffffff;
                    v[i].tu = (i == 1 || i == 2);
                    v[i].tv = (i >= 2);
                }
                auto saved = states;
                states[D3DRENDERSTATE_TEXTUREHANDLE] = (DWORD)src;
                states[D3DRENDERSTATE_ALPHABLENDENABLE] = 0;
                states[D3DRENDERSTATE_ZFUNC] = D3DCMP_ALWAYS;
                states[D3DRENDERSTATE_ZWRITEENABLE] = 0;
                bh_renderer_draw_fan(nullptr, v, 4);
                states = saved;
            }
            return present() ? DD_OK : E_FAIL;
        }
        if (dwFlags & DDBLT_DEPTHFILL)
            return clear_target(false, true) ? DD_OK : E_FAIL;
        if ((dwFlags & DDBLT_COLORFILL) && dst->target)
        {
            unsigned value = lpDDBltFx ? lpDDBltFx->dwFillColor : 0;
            unsigned rgb =
                ((value >> 11) & 31) * 255 / 31 << 16 | ((value >> 5) & 63) * 255 / 63 << 8 | (value & 31) * 255 / 31;
            return clear_target(true, false, rgb) ? DD_OK : E_FAIL;
        }
        if (!record_pending_draws())
            return E_FAIL;
        if (dwFlags & DDBLT_COLORFILL)
        {
            unsigned value = lpDDBltFx ? lpDDBltFx->dwFillColor : 0;
            unsigned bytes = (dst->desc.ddpfPixelFormat.dwRGBBitCount + 7) / 8;
            for (unsigned y = 0; y < dst->desc.dwHeight; ++y)
                for (unsigned x = 0; x < dst->desc.dwWidth; ++x)
                    memcpy(dst->pixels.data() + y * dst->desc.lPitch + x * bytes, &value, bytes);
        }
        else if (src)
        {
            if (dst->target)
                return DD_OK;
            unsigned bytes = (dst->desc.ddpfPixelFormat.dwRGBBitCount + 7) / 8;
            if (bytes != (src->desc.ddpfPixelFormat.dwRGBBitCount + 7) / 8)
                return DDERR_UNSUPPORTED;
            RECT from = {0, 0, (LONG)src->desc.dwWidth, (LONG)src->desc.dwHeight};
            if (lpSrcRect)
                from = *lpSrcRect;
            RECT to = {0, 0, (LONG)dst->desc.dwWidth, (LONG)dst->desc.dwHeight};
            if (lpDestRect)
                to = *lpDestRect;
            if (to.right <= to.left)
                to.right = to.left + from.right - from.left;
            if (to.bottom <= to.top)
                to.bottom = to.top + from.bottom - from.top;
            if (from.right <= from.left || from.bottom <= from.top)
                return E_INVALIDARG;
            for (LONG y = std::max(0L, to.top); y < std::min((LONG)dst->desc.dwHeight, to.bottom); ++y)
            {
                LONG sy = from.top + (y - to.top) * (from.bottom - from.top) / (to.bottom - to.top);
                if (sy < 0 || sy >= (LONG)src->desc.dwHeight)
                    continue;
                for (LONG x = std::max(0L, to.left); x < std::min((LONG)dst->desc.dwWidth, to.right); ++x)
                {
                    LONG sx = from.left + (x - to.left) * (from.right - from.left) / (to.right - to.left);
                    if (sx < 0 || sx >= (LONG)src->desc.dwWidth)
                        continue;
                    memcpy(dst->pixels.data() + y * dst->desc.lPitch + x * bytes,
                           src->pixels.data() + sy * src->desc.lPitch + sx * bytes, bytes);
                }
            }
        }
        ++dst->version;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_copy(BHRenderSurface *self, DWORD dwX, DWORD dwY,
                                             BHRenderSurface *lpDDSrcSurface, LPRECT lpSrcRect)
    {
        RECT r = {(LONG)dwX, (LONG)dwY, 0, 0};
        return bh_renderer_surface_blit(self, &r, lpDDSrcSurface, lpSrcRect, 0, nullptr);
    }
    HRESULT __cdecl bh_renderer_surface_detach_depth(BHRenderSurface *self, BHRenderSurface *lpDDSAttachedSurface)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_present(BHRenderSurface *self)
    {
        return present() ? DD_OK : E_FAIL;
    }
    HRESULT __cdecl bh_renderer_surface_backbuffer(BHRenderSurface *self, BHRenderSurface **lplpDDAttachedSurface)
    {
        auto *s = self;
        if (!s->back)
            return DDERR_NOTFOUND;
        addref(s->back);
        *lplpDDAttachedSurface = s->back;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_format(BHRenderSurface *self, LPDDPIXELFORMAT lpDDPixelFormat)
    {
        *lpDDPixelFormat = (self)->desc.ddpfPixelFormat;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_status(BHRenderSurface *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_lock(BHRenderSurface *self, LPDDSURFACEDESC lpDDSurfaceDesc)
    {
        auto *s = self;
        if (s->target)
        {
            auto pixels = readback();
            if (pixels.empty())
                return E_FAIL;
            float vx, vy, vw, vh, scale;
            viewport_geometry(vx, vy, vw, vh, scale);
            const auto display = query_display();
            const float mx = display.margin_x, my = display.margin_y;
            float scale_x = vw / (640 + 2 * mx), scale_y = vh / (480 + 2 * my);
            float ox = vx + mx * scale_x, oy = vy + my * scale_y;
            for (unsigned y = 0; y < s->desc.dwHeight; ++y)
                for (unsigned x = 0; x < s->desc.dwWidth; ++x)
                {
                    unsigned px = std::min(target_w - 1, (unsigned)(ox + x * scale_x)),
                             py = std::min(target_h - 1, (unsigned)(oy + y * scale_y));
                    unsigned c = pixels[py * target_w + px];
                    unsigned short v = ((c & 255) >> 3) << 11 | (((c >> 8) & 255) >> 2) << 5 | (((c >> 16) & 255) >> 3);
                    memcpy(s->pixels.data() + y * s->desc.lPitch + x * 2, &v, 2);
                }
        }
        if (s->target)
            s->locked_pixels = s->pixels;
        *lpDDSurfaceDesc = s->desc;
        lpDDSurfaceDesc->lpSurface = s->pixels.data();
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_restore(BHRenderSurface *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_set_palette(BHRenderSurface *self, BHRenderPalette *lpDDPalette)
    {
        auto *s = self;
        addref(lpDDPalette);
        release(s->palette);
        s->palette = lpDDPalette;
        ++s->version;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_unlock(BHRenderSurface *self)
    {
        auto *s = self;
        ++s->version;
        if (s->target && s->pixels != s->locked_pixels)
        {
            s->locked_pixels.clear();
            return draw_surface(s) ? DD_OK : E_FAIL;
        }
        s->locked_pixels.clear();
        return DD_OK;
    }
    ULONG __cdecl bh_renderer_retain(BHRenderer *self)
    {
        return addref(self);
    }
    HRESULT __cdecl bh_renderer_palette_create(BHRenderer *self, LPPALETTEENTRY lpColorTable,
                                               BHRenderPalette **lplpDDPalette)
    {
        auto *p = new Palette;
        memcpy(p->entries, lpColorTable, sizeof(p->entries));
        *lplpDDPalette = p;
        return DD_OK;
    }
    HRESULT __cdecl bh_renderer_surface_create(BHRenderer *self, LPDDSURFACEDESC lpDDSurfaceDesc,
                                               BHRenderSurface **lplpDDSurface)
    {
        auto *s = make_surface(*lpDDSurfaceDesc);
        if (s->primary && (lpDDSurfaceDesc->dwFlags & DDSD_BACKBUFFERCOUNT))
        {
            DDSURFACEDESC back = *lpDDSurfaceDesc;
            back.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
            back.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_BACKBUFFER;
            s->back = make_surface(back);
        }
        *lplpDDSurface = s;
        return DD_OK;
    }
}
