// SDL owns the native window, device, textures and presentation. The adapter
// retains the restored engine's 32-bit data ABI, never creates a COM object.
#define CINTERFACE
#include "bh_sdl_adapter.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <d3dcompiler.h>
#include <map>
#include <string>
#include <vector>

namespace
{
struct Object
{
    unsigned refs = 1;
    virtual ~Object() = default;
};
struct Palette : Object
{
    PALETTEENTRY entries[256]{};
};
struct Surface : Object
{
    DDSURFACEDESC desc{};
    std::vector<unsigned char> pixels, locked_pixels;
    HDC dc = nullptr;
    HBITMAP bitmap = nullptr, previous_bitmap = nullptr;
    void *dib = nullptr;
    Palette *palette = nullptr;
    Surface *back = nullptr;
    SDL_GPUTexture *gpu = nullptr;
    unsigned version = 1, uploaded = 0;
    bool target = false, primary = false, keyed = false;
    ~Surface();
};
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
SDL_Window *window;
SDL_GPUDevice *device;
SDL_GPUShader *vs, *fs;
SDL_GPUSampler *sampler;
SDL_GPUTexture *color_target, *depth_target;
unsigned target_w, target_h;
bool targets_fresh = false;
SDL_GPUCommandBuffer *commands;
SDL_GPURenderPass *pass;
std::map<unsigned long long, SDL_GPUGraphicsPipeline *> pipelines;
std::vector<Vertex> vertices;
std::vector<Batch> batches;
std::vector<Surface *> surfaces;
std::array<unsigned, 256> states{};
Surface *white;
BHSDLMessage message_callback;
void *message_user;
char error_text[1024];
bool quitting, hidden;
int normal_x = SDL_WINDOWPOS_CENTERED, normal_y = SDL_WINDOWPOS_CENTERED, normal_w = 1280, normal_h = 960,
    mode = 0;
bool maximized = false;
std::vector<std::pair<int, int>> modes;
int selected_resolution = 0;
const char *settings_path = "video-sdl3.ini";
void set_error(const char *context)
{
    snprintf(error_text, sizeof(error_text), "%s: %s", context, SDL_GetError());
    FILE *f = fopen("sdl-gpu.log", "a");
    if (f)
    {
        fprintf(f, "%s\n", error_text);
        fclose(f);
    }
}
unsigned addref(void *p)
{
    return p ? ++static_cast<Object *>(p)->refs : 0;
}
unsigned release(void *p)
{
    if (!p)
        return 0;
    auto *o = static_cast<Object *>(p);
    unsigned n = --o->refs;
    if (!n)
        delete o;
    return n;
}
Surface::~Surface()
{
    if (dc)
    {
        SelectObject(dc, previous_bitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
    }
    surfaces.erase(std::remove(surfaces.begin(), surfaces.end(), this), surfaces.end());
    if (gpu && device)
        SDL_ReleaseGPUTexture(device, gpu);
    release(palette);
    release(back);
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
void save_settings()
{
    if (!window || hidden)
        return;
    FILE *f = fopen("video-sdl3.ini.tmp", "w");
    if (!f)
        return;
    fprintf(f, "BHSDL3 1\n%d %d %d %d %d %d %d\n", normal_x, normal_y, normal_w, normal_h, mode, maximized,
            selected_resolution);
    if (fclose(f) == 0)
        MoveFileExA("video-sdl3.ini.tmp", settings_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}
void remember_window()
{
    auto flags = SDL_GetWindowFlags(window);
    if (!(flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED)))
    {
        maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
        if (!maximized)
        {
            SDL_GetWindowPosition(window, &normal_x, &normal_y);
            SDL_GetWindowSize(window, &normal_w, &normal_h);
        }
    }
    save_settings();
}
bool SDLCALL message_hook(void *, MSG *msg)
{
    if (window && msg->hwnd == (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                            SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr))
    {
        if (msg->message == WM_SYSKEYDOWN && msg->wParam == VK_RETURN)
            return true;
        if (message_callback)
            message_callback(msg->hwnd, msg->message, msg->wParam, msg->lParam, message_user);
    }
    return true;
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
        commands = SDL_AcquireGPUCommandBuffer(device);
    return commands != nullptr;
}
bool ensure_targets()
{
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (mode == 1 && !modes.empty())
    {
        w = modes[selected_resolution].first;
        h = modes[selected_resolution].second;
    }
    if (hidden)
    {
        const char *size = SDL_getenv("BH_GPU_TEST_SIZE");
        if (size)
            sscanf(size, "%dx%d", &w, &h);
    }
    w = std::max(1, std::min(w, 3840));
    h = std::max(1, std::min(h, 2160));
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
    depth_target = make_texture(w, h, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
    return color_target && depth_target;
}
bool begin_pass(SDL_GPULoadOp color_load = SDL_GPU_LOADOP_LOAD, SDL_GPULoadOp depth_load = SDL_GPU_LOADOP_LOAD,
                SDL_FColor clear = {0, 0, 0, 1})
{
    if (!ensure_commands() || !ensure_targets())
        return false;
    if (pass && color_load == SDL_GPU_LOADOP_LOAD && depth_load == SDL_GPU_LOADOP_LOAD)
        return true;
    if (targets_fresh)
    {
        color_load = depth_load = SDL_GPU_LOADOP_CLEAR;
        targets_fresh = false;
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
        return false;
    float scale = std::min(target_w / 640.f, target_h / 480.f);
    SDL_GPUViewport viewport{
        (target_w - 640 * scale) / 2, (target_h - 480 * scale) / 2, 640 * scale, 480 * scale, 0, 1};
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
    unsigned long long key =
        b.blend | (b.src << 1) | (b.dst << 5) | (b.depth << 9) | (b.write << 13) | (b.cull << 14);
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
    info.rasterizer_state.cull_mode =
        b.cull == D3DCULL_NONE || !b.cull ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
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
bool flush()
{
    if (vertices.empty())
        return true;
    end_pass();
    if (!ensure_commands() || !ensure_targets())
        return false;
    unsigned size = vertices.size() * sizeof(Vertex);
    SDL_GPUTransferBufferCreateInfo tinfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, size, 0};
    auto *transfer = SDL_CreateGPUTransferBuffer(device, &tinfo);
    SDL_GPUBufferCreateInfo binfo{SDL_GPU_BUFFERUSAGE_VERTEX, size, 0};
    auto *buffer = SDL_CreateGPUBuffer(device, &binfo);
    if (!transfer || !buffer)
        return false;
    auto *mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped)
        return false;
    memcpy(mapped, vertices.data(), size);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    auto *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTransferBufferLocation src{transfer, 0};
    SDL_GPUBufferRegion dst{buffer, 0, size};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    for (auto &b : batches)
    {
        Surface *s = b.texture ? b.texture : white;
        if (!upload(s) || !begin_pass())
            return false;
        auto *p = pipeline(b);
        if (!p)
            return false;
        SDL_BindGPUGraphicsPipeline(pass, p);
        SDL_GPUBufferBinding binding{buffer, 0};
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
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    SDL_ReleaseGPUBuffer(device, buffer);
    return true;
}
bool clear_target(bool color, bool depth, unsigned value = 0)
{
    if (!flush())
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
    HRESULT hr = bh_IDirect3DDevice2_DrawPrimitive(nullptr, D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, v, 4, 0);
    states = saved;
    return SUCCEEDED(hr) && flush();
}
bool present()
{
    if (!flush() || !ensure_commands() || !ensure_targets())
        return false;
    end_pass();
    SDL_GPUTexture *swap = nullptr;
    Uint32 w, h;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window, &swap, &w, &h))
    {
        set_error("Acquire swapchain");
        return false;
    }
    if (swap)
    {
        SDL_GPUBlitInfo blit{};
        blit.source.texture = color_target;
        blit.source.w = target_w;
        blit.source.h = target_h;
        blit.destination.texture = swap;
        blit.destination.w = w;
        blit.destination.h = h;
        blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
        blit.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(commands, &blit);
    }
    bool ok = SDL_SubmitGPUCommandBuffer(commands);
    commands = nullptr;
    return ok;
}
std::vector<unsigned> readback()
{
    if (!flush() || !ensure_commands() || !ensure_targets())
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
SDL_GPUShader *shader(const char *path, const char *entry, const char *profile, SDL_GPUShaderStage stage)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        SDL_SetError("Cannot open shader %s", path);
        return nullptr;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    std::string source(n, '\0');
    fread(source.data(), 1, n, f);
    fclose(f);
    ID3DBlob *code = nullptr, *errors = nullptr;
    HRESULT hr = D3DCompile(source.data(), n, path, nullptr, nullptr, entry, profile,
                            D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);
    if (FAILED(hr))
    {
        if (errors)
        {
            snprintf(error_text, sizeof(error_text), "%s", (char *)errors->lpVtbl->GetBufferPointer(errors));
            errors->lpVtbl->Release(errors);
        }
        return nullptr;
    }
    SDL_GPUShaderCreateInfo info{};
    info.code = (const Uint8 *)code->lpVtbl->GetBufferPointer(code);
    info.code_size = code->lpVtbl->GetBufferSize(code);
    info.entrypoint = entry;
    info.format = SDL_GPU_SHADERFORMAT_DXBC;
    info.stage = stage;
    if (stage == SDL_GPU_SHADERSTAGE_FRAGMENT)
    {
        info.num_samplers = 1;
        info.num_uniform_buffers = 1;
    }
    auto *s = SDL_CreateGPUShader(device, &info);
    code->lpVtbl->Release(code);
    if (errors)
        errors->lpVtbl->Release(errors);
    return s;
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
void apply_mode()
{
    if (mode == 0)
    {
        SDL_SetWindowFullscreen(window, false);
        SDL_SyncWindow(window);
        SDL_SetWindowSize(window, normal_w, normal_h);
        SDL_SetWindowPosition(window, normal_x, normal_y);
        if (maximized)
            SDL_MaximizeWindow(window);
    }
    else
    {
        SDL_SetWindowFullscreenMode(window, nullptr);
        SDL_SetWindowFullscreen(window, true);
    }
    SDL_SyncWindow(window);
    save_settings();
}
} // namespace
extern "C"
{
    HWND __cdecl bh_sdl_open(int hide, BHSDLMessage callback, void *user)
    {
        hidden = hide != 0 || SDL_getenv("BH_MENU_SMOKE_SECONDS") != nullptr;
        message_callback = callback;
        message_user = user;
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            set_error("SDL init");
            return nullptr;
        }
        FILE *f = fopen(settings_path, "r");
        if (f)
        {
            char tag[32] = {};
            int version = 0, m = 0;
            if (fscanf(f, "%31s %d", tag, &version) == 2 && !strcmp(tag, "BHSDL3") && version == 1)
            {
                fscanf(f, "%d %d %d %d %d %d %d", &normal_x, &normal_y, &normal_w, &normal_h, &mode, &m,
                       &selected_resolution);
                maximized = m != 0;
            }
            fclose(f);
        }
        normal_w = std::clamp(normal_w, 640, 3840);
        normal_h = std::clamp(normal_h, 480, 2160);
        mode = std::clamp(mode, 0, 1);
        if (hidden)
        {
            normal_w = 640;
            normal_h = 480;
            mode = 0;
            maximized = false;
        }
        window = SDL_CreateWindow("BHunter", normal_w, normal_h,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
        if (!window)
        {
            set_error("Create window");
            return nullptr;
        }
        SDL_SetWindowMinimumSize(window, 640, 480);
        SDL_SetWindowPosition(window, normal_x, normal_y);
        SDL_SetWindowsMessageHook(message_hook, nullptr);
        int count = 0;
        auto **available = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(window), &count);
        for (int i = 0; i < count; ++i)
        {
            auto *m = available[i];
            std::pair<int, int> size{m->w, m->h};
            if (m->w >= 640 && m->h >= 480 && m->w <= 3840 && m->h <= 2160 &&
                std::find(modes.begin(), modes.end(), size) == modes.end())
                modes.push_back(size);
        }
        SDL_free(available);
        if (modes.empty())
            modes.push_back({normal_w, normal_h});
        std::sort(modes.begin(), modes.end());
        selected_resolution = std::clamp(selected_resolution, 0, (int)modes.size() - 1);
        device =
            SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXBC, SDL_getenv("BH_GPU_DEBUG") != nullptr, "direct3d12");
        if (!device || !SDL_ClaimWindowForGPUDevice(device, window))
        {
            set_error("Create GPU device");
            return nullptr;
        }
        SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
        SDL_SetGPUAllowedFramesInFlight(device, 1);
        vs = shader("shaders/legacy.hlsl", "VSMain", "vs_5_1", SDL_GPU_SHADERSTAGE_VERTEX);
        fs = shader("shaders/legacy.hlsl", "PSMain", "ps_5_1", SDL_GPU_SHADERSTAGE_FRAGMENT);
        if (!vs || !fs)
        {
            set_error("Create shaders");
            return nullptr;
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
        apply_mode();
        clear_target(true, true);
        if (!hidden)
            SDL_ShowWindow(window);
        return (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                            nullptr);
    }
    double __cdecl bh_sdl_seconds()
    {
        return SDL_GetTicksNS() / 1000000000.0;
    }
    void __cdecl bh_sdl_pace()
    {
        static Uint64 deadline = 0;
        const Uint64 interval = 1000000000ULL / 60;
        Uint64 now = SDL_GetTicksNS();
        if (!deadline || now > deadline + interval * 4)
            deadline = now;
        deadline += interval;
        if (deadline > now)
            SDL_DelayPrecise(deadline - now);
    }
    int __cdecl bh_sdl_poll()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                quitting = true;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT) && !event.key.repeat)
                {
                    remember_window();
                    mode = mode ? 0 : 1;
                    apply_mode();
                }
                break;
            case SDL_EVENT_WINDOW_MOVED:
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                remember_window();
                break;
            default:
                break;
            }
        }
        return !quitting;
    }
    const char *__cdecl bh_sdl_error()
    {
        return error_text;
    }
    void __cdecl bh_sdl_close()
    {
        if (window)
            remember_window();
        flush();
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
            if (window)
                SDL_ReleaseWindowFromGPUDevice(device, window);
            SDL_DestroyGPUDevice(device);
            device = nullptr;
        }
        SDL_SetWindowsMessageHook(nullptr, nullptr);
        if (window)
            SDL_DestroyWindow(window);
        window = nullptr;
        SDL_Quit();
    }
    void __cdecl bh_sdl_video_cycle(int row)
    {
        if (row == 0)
        {
            remember_window();
            mode = mode ? 0 : 1;
            apply_mode();
        }
        if (row == 2 && !modes.empty())
        {
            selected_resolution = (selected_resolution + 1) % modes.size();
            if (!mode)
            {
                auto size = modes[selected_resolution];
                normal_w = size.first;
                normal_h = size.second;
                maximized = false;
                SDL_RestoreWindow(window);
                SDL_SetWindowSize(window, normal_w, normal_h);
                remember_window();
            }
        }
        save_settings();
    }
    const char *__cdecl bh_sdl_video_label(int row)
    {
        static char text[64];
        if (row == 0)
            return mode ? "Borderless" : "Windowed";
        if (row == 1)
            return "SDL3 GPU";
        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        if (mode && !modes.empty())
        {
            w = modes[selected_resolution].first;
            h = modes[selected_resolution].second;
        }
        snprintf(text, sizeof(text), "%d x %d", w, h);
        return text;
    }
    int __cdecl bh_sdl_capture(const char *path)
    {
        auto pixels = readback();
        if (pixels.empty())
            return 0;
        auto *s = SDL_CreateSurfaceFrom(target_w, target_h, SDL_PIXELFORMAT_RGBA32, pixels.data(), target_w * 4);
        bool ok = SDL_SavePNG(s, path);
        SDL_DestroySurface(s);
        return ok;
    }
    HRESULT __cdecl bh_gpu_create(GUID *, LPDIRECTDRAW *out, IUnknown *)
    {
        *out = (LPDIRECTDRAW) new Object;
        return DD_OK;
    }
    HRESULT __cdecl bh_gpu_enumerate(LPDDENUMCALLBACKA callback, void *user)
    {
        char name[] = "SDL3 GPU";
        callback(nullptr, name, name, user);
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3D2_CreateDevice(void *self, const GUID *rclsid, IDirectDrawSurface *surface,
                                               struct IDirect3DDevice2 **device)
    {
        ((Surface *)surface)->target = true;
        *device = (IDirect3DDevice2 *)new Object;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3D2_CreateMaterial(void *self, struct IDirect3DMaterial2 **material, IUnknown *outer)
    {
        *material = (IDirect3DMaterial2 *)new Object;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3D2_CreateViewport(void *self, struct IDirect3DViewport2 **viewport, IUnknown *outer)
    {
        *viewport = (IDirect3DViewport2 *)new Object;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3D2_EnumDevices(void *self, LPD3DENUMDEVICESCALLBACK cb, void *ctx)
    {
        D3DDEVICEDESC d{};
        d.dwSize = sizeof(d);
        d.dwFlags = D3DDD_COLORMODEL;
        d.dcmColorModel = D3DCOLOR_RGB;
        char name[] = "SDL3 GPU";
        GUID id{};
        cb(&id, name, name, &d, &d, ctx);
        return DD_OK;
    }
    ULONG __cdecl bh_IDirect3D2_Release(void *self)
    {
        return release(self);
    }
    ULONG __cdecl bh_IDirect3DDevice2_AddRef(void *self)
    {
        return addref(self);
    }
    HRESULT __cdecl bh_IDirect3DDevice2_AddViewport(void *self, IDirect3DViewport2 *viewport)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DDevice2_BeginScene(void *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DDevice2_DeleteViewport(void *self, IDirect3DViewport2 *viewport)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DDevice2_DrawPrimitive(void *self, D3DPRIMITIVETYPE primitive_type,
                                                      D3DVERTEXTYPE vertex_type, void *vertices,
                                                      DWORD vertex_count, DWORD flags)
    {
        if (vertex_type != D3DVT_TLVERTEX || primitive_type != D3DPT_TRIANGLEFAN || vertex_count < 3 ||
            vertex_count > 65536)
            return E_INVALIDARG;
        auto *v = (D3DTLVERTEX *)vertices;
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
    HRESULT __cdecl bh_IDirect3DDevice2_EndScene(void *self)
    {
        return flush() ? DD_OK : E_FAIL;
    }
    HRESULT __cdecl bh_IDirect3DDevice2_EnumTextureFormats(void *self, LPD3DENUMTEXTUREFORMATSCALLBACK cb,
                                                           void *ctx)
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
    ULONG __cdecl bh_IDirect3DDevice2_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirect3DDevice2_SetCurrentViewport(void *self, IDirect3DViewport2 *viewport)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DDevice2_SetLightState(void *self, D3DLIGHTSTATETYPE dwLightStateType,
                                                      DWORD dwLightState)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DDevice2_SetRenderState(void *self, D3DRENDERSTATETYPE dwRenderStateType,
                                                       DWORD dwRenderState)
    {
        if ((unsigned)dwRenderStateType >= states.size())
            return E_INVALIDARG;
        states[dwRenderStateType] = dwRenderState;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DMaterial2_GetHandle(void *self, struct IDirect3DDevice2 *device,
                                                    D3DMATERIALHANDLE *handle)
    {
        *handle = 1;
        return DD_OK;
    }
    ULONG __cdecl bh_IDirect3DMaterial2_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirect3DMaterial2_SetMaterial(void *self, D3DMATERIAL *data)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DTexture2_GetHandle(void *self, struct IDirect3DDevice2 *device,
                                                   D3DTEXTUREHANDLE *handle)
    {
        *handle = (DWORD)self;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirect3DTexture2_PaletteChanged(void *self, DWORD dwStart, DWORD dwCount)
    {
        ++((Surface *)self)->version;
        return DD_OK;
    }
    ULONG __cdecl bh_IDirect3DTexture2_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirect3DViewport2_Clear(void *self, DWORD count, D3DRECT *rects, DWORD flags)
    {
        return clear_target(flags & D3DCLEAR_TARGET, flags & D3DCLEAR_ZBUFFER) ? DD_OK : E_FAIL;
    }
    ULONG __cdecl bh_IDirect3DViewport2_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirect3DViewport2_SetViewport2(void *self, D3DVIEWPORT2 *data)
    {
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDraw2_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirectDraw2_RestoreDisplayMode(void *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDraw2_SetCooperativeLevel(void *self, HWND hWnd, DWORD dwFlags)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDraw2_SetDisplayMode(void *self, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP,
                                                   DWORD dwRefreshRate, DWORD dwFlags)
    {
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDrawClipper_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirectDrawClipper_SetHWnd(void *self, DWORD dwFlags, HWND hWnd)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawPalette_GetEntries(void *self, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries,
                                                     LPPALETTEENTRY lpEntries)
    {
        if (dwBase + dwNumEntries > 256)
            return E_INVALIDARG;
        memcpy(lpEntries, ((Palette *)self)->entries + dwBase, dwNumEntries * sizeof(PALETTEENTRY));
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDrawPalette_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirectDrawPalette_SetEntries(void *self, DWORD dwFlags, DWORD dwStartingEntry,
                                                     DWORD dwCount, LPPALETTEENTRY lpEntries)
    {
        if (dwStartingEntry + dwCount > 256)
            return E_INVALIDARG;
        if (!flush())
            return E_FAIL;
        memcpy(((Palette *)self)->entries + dwStartingEntry, lpEntries, dwCount * sizeof(PALETTEENTRY));
        for (auto *s : surfaces)
            if (s->palette == self)
                ++s->version;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_AddAttachedSurface(void *self, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
    {
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDrawSurface_AddRef(void *self)
    {
        return addref(self);
    }
    HRESULT __cdecl bh_IDirectDrawSurface_Blt(void *self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface,
                                              LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
    {
        auto *dst = (Surface *)self, *src = (Surface *)lpDDSrcSurface;
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
                bh_IDirect3DDevice2_DrawPrimitive(nullptr, D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, v, 4, 0);
                states = saved;
            }
            return present() ? DD_OK : E_FAIL;
        }
        if (dwFlags & DDBLT_DEPTHFILL)
            return clear_target(false, true) ? DD_OK : E_FAIL;
        if ((dwFlags & DDBLT_COLORFILL) && dst->target)
        {
            unsigned value = lpDDBltFx ? lpDDBltFx->dwFillColor : 0;
            unsigned rgb = ((value >> 11) & 31) * 255 / 31 << 16 | ((value >> 5) & 63) * 255 / 63 << 8 |
                           (value & 31) * 255 / 31;
            return clear_target(true, false, rgb) ? DD_OK : E_FAIL;
        }
        if (!flush())
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
    HRESULT __cdecl bh_IDirectDrawSurface_BltFast(void *self, DWORD dwX, DWORD dwY,
                                                  LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
                                                  DWORD dwTrans)
    {
        RECT r = {(LONG)dwX, (LONG)dwY, 0, 0};
        return bh_IDirectDrawSurface_Blt(self, &r, lpDDSrcSurface, lpSrcRect, 0, nullptr);
    }
    HRESULT __cdecl bh_IDirectDrawSurface_DeleteAttachedSurface(void *self, DWORD dwFlags,
                                                                LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_Flip(void *self, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,
                                               DWORD dwFlags)
    {
        return present() ? DD_OK : E_FAIL;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_GetAttachedSurface(void *self, LPDDSCAPS lpDDSCaps,
                                                             LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
    {
        auto *s = (Surface *)self;
        if (!s->back)
            return DDERR_NOTFOUND;
        addref(s->back);
        *lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE)s->back;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_GetDC(void *self, HDC *lphDC)
    {
        auto *s = (Surface *)self;
        if (s->dc)
            return E_FAIL;
        BITMAPINFO b{};
        b.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        b.bmiHeader.biWidth = 640;
        b.bmiHeader.biHeight = -480;
        b.bmiHeader.biPlanes = 1;
        b.bmiHeader.biBitCount = 32;
        b.bmiHeader.biCompression = BI_RGB;
        s->dc = CreateCompatibleDC(nullptr);
        s->bitmap = CreateDIBSection(s->dc, &b, DIB_RGB_COLORS, &s->dib, nullptr, 0);
        if (!s->dc || !s->bitmap)
            return E_FAIL;
        s->previous_bitmap = (HBITMAP)SelectObject(s->dc, s->bitmap);
        memset(s->dib, 0, 640 * 480 * 4);
        *lphDC = s->dc;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_GetPixelFormat(void *self, LPDDPIXELFORMAT lpDDPixelFormat)
    {
        *lpDDPixelFormat = ((Surface *)self)->desc.ddpfPixelFormat;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_IsLost(void *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_Lock(void *self, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc,
                                               DWORD dwFlags, HANDLE hEvent)
    {
        auto *s = (Surface *)self;
        if (s->target)
        {
            auto pixels = readback();
            if (pixels.empty())
                return E_FAIL;
            float scale = std::min(target_w / 640.f, target_h / 480.f);
            unsigned ox = (target_w - 640 * scale) / 2, oy = (target_h - 480 * scale) / 2;
            for (unsigned y = 0; y < s->desc.dwHeight; ++y)
                for (unsigned x = 0; x < s->desc.dwWidth; ++x)
                {
                    unsigned px = std::min(target_w - 1, ox + (unsigned)(x * scale)),
                             py = std::min(target_h - 1, oy + (unsigned)(y * scale));
                    unsigned c = pixels[py * target_w + px];
                    unsigned short v =
                        ((c & 255) >> 3) << 11 | (((c >> 8) & 255) >> 2) << 5 | (((c >> 16) & 255) >> 3);
                    memcpy(s->pixels.data() + y * s->desc.lPitch + x * 2, &v, 2);
                }
        }
        if (s->target)
            s->locked_pixels = s->pixels;
        *lpDDSurfaceDesc = s->desc;
        lpDDSurfaceDesc->lpSurface = s->pixels.data();
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_QueryInterface(void *self, const IID *riid, void **ppvObject)
    {
        addref(self);
        *ppvObject = self;
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDrawSurface_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirectDrawSurface_ReleaseDC(void *self, HDC hDC)
    {
        auto *s = (Surface *)self;
        if (hDC != s->dc)
            return E_INVALIDARG;
        GdiFlush();
        DDSURFACEDESC d{};
        d.dwWidth = 640;
        d.dwHeight = 480;
        d.ddpfPixelFormat = pixel_format(32);
        d.dwFlags = DDSD_CKSRCBLT;
        auto *text = make_surface(d);
        memcpy(text->pixels.data(), s->dib, text->pixels.size());
        bool ok = draw_surface(text, true);
        release(text);
        SelectObject(s->dc, s->previous_bitmap);
        DeleteObject(s->bitmap);
        DeleteDC(s->dc);
        s->dc = nullptr;
        s->bitmap = nullptr;
        s->dib = nullptr;
        return ok ? DD_OK : E_FAIL;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_Restore(void *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_SetClipper(void *self, LPDIRECTDRAWCLIPPER lpDDClipper)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_SetPalette(void *self, LPDIRECTDRAWPALETTE lpDDPalette)
    {
        auto *s = (Surface *)self;
        addref(lpDDPalette);
        release(s->palette);
        s->palette = (Palette *)lpDDPalette;
        ++s->version;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDrawSurface_Unlock(void *self, LPVOID lpSurfaceData)
    {
        auto *s = (Surface *)self;
        ++s->version;
        if (s->target && s->pixels != s->locked_pixels)
        {
            s->locked_pixels.clear();
            return draw_surface(s) ? DD_OK : E_FAIL;
        }
        s->locked_pixels.clear();
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDraw_AddRef(void *self)
    {
        return addref(self);
    }
    HRESULT __cdecl bh_IDirectDraw_CreateClipper(void *self, DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper,
                                                 IUnknown *pUnkOuter)
    {
        *lplpDDClipper = (LPDIRECTDRAWCLIPPER) new Object;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDraw_CreatePalette(void *self, DWORD dwFlags, LPPALETTEENTRY lpColorTable,
                                                 LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter)
    {
        auto *p = new Palette;
        memcpy(p->entries, lpColorTable, sizeof(p->entries));
        *lplpDDPalette = (LPDIRECTDRAWPALETTE)p;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDraw_CreateSurface(void *self, LPDDSURFACEDESC lpDDSurfaceDesc,
                                                 LPDIRECTDRAWSURFACE *lplpDDSurface, IUnknown *pUnkOuter)
    {
        auto *s = make_surface(*lpDDSurfaceDesc);
        if (s->primary && (lpDDSurfaceDesc->dwFlags & DDSD_BACKBUFFERCOUNT))
        {
            DDSURFACEDESC back = *lpDDSurfaceDesc;
            back.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
            back.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_BACKBUFFER;
            s->back = make_surface(back);
        }
        *lplpDDSurface = (LPDIRECTDRAWSURFACE)s;
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDraw_QueryInterface(void *self, const IID *riid, void **ppvObject)
    {
        *ppvObject = new Object;
        return DD_OK;
    }
    ULONG __cdecl bh_IDirectDraw_Release(void *self)
    {
        return release(self);
    }
    HRESULT __cdecl bh_IDirectDraw_RestoreDisplayMode(void *self)
    {
        return DD_OK;
    }
    HRESULT __cdecl bh_IDirectDraw_SetCooperativeLevel(void *self, HWND hWnd, DWORD dwFlags)
    {
        return DD_OK;
    }
}
