#include "bh_sdl_text.h"
#include "bh_sdl_diagnostics.h"

#include <array>

namespace
{
#include "../Graphics/Textures/bh_overlay_atlas.inc"

constexpr unsigned base64_digit(char digit)
{
    if (digit >= 'A' && digit <= 'Z')
        return digit - 'A';
    if (digit >= 'a' && digit <= 'z')
        return digit - 'a' + 26;
    if (digit >= '0' && digit <= '9')
        return digit - '0' + 52;
    return digit == '+' ? 62 : 63;
}

constexpr auto decode_overlay_font()
{
    constexpr size_t length = sizeof(overlay_font_base64) - 1;
    static_assert(length % 4 == 0);
    constexpr size_t padding = (overlay_font_base64[length - 1] == '=') +
                               (overlay_font_base64[length - 2] == '=');
    std::array<unsigned char, length / 4 * 3 - padding> bytes{};
    unsigned bits = 0, pending = 0;
    size_t out = 0;
    for (size_t i = 0; i < length - padding; ++i)
    {
        bits = (bits << 6) | base64_digit(overlay_font_base64[i]);
        pending += 6;
        if (pending >= 8)
        {
            pending -= 8;
            bytes[out++] = (unsigned char)(bits >> pending);
        }
    }
    return bytes;
}

constexpr auto overlay_font_bytes = decode_overlay_font();

const BHOverlayFont &font()
{
    // The parsed glyphs and their embedded bitmap are independent of the GPU device.
    static const BHOverlayFont atlas = [] {
        BHOverlayFont result{};
        bh_overlay_font_read(&result, overlay_font_bytes.data(), overlay_font_bytes.size());
        return result;
    }();
    return atlas;
}
} // namespace

namespace bh_sdl_text
{
bool prepare()
{
    if (font().mask)
        return true;
    bh_sdl_diagnostics::set_message("Invalid embedded overlay font atlas");
    return false;
}

bool rasterize(BHOverlayCanvas &canvas, int x, int y, const char *text, unsigned long rgb)
{
    return bh_overlay_text(&canvas, &font(), x, y, text, rgb) != 0;
}
} // namespace bh_sdl_text
