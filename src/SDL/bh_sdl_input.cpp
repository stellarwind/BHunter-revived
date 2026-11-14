#include "bh_sdl_input.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

namespace
{
SDL_Window *window;
SDL_Gamepad *gamepad;
BHPlatformInput input_callback;
void *message_user;
BHPlatformTextInput text_callback;
void *text_user;
unsigned long text_gamepad_buttons;
std::array<unsigned, SDL_SCANCODE_COUNT> held_keys{};
void input_event(BHPlatformEventType type, unsigned key, bool down, bool repeat = false)
{
    BHPlatformEvent event{type, key, down, repeat};
    if (input_callback)
        input_callback(&event, message_user);
}
void lose_focus()
{
    bh_sdl_set_relative_mouse(0);
    held_keys.fill(0);
    input_event(BH_PLATFORM_FOCUS, 0, false);
}
} // namespace

namespace bh_sdl_input
{
/* Special and OEM keys keep the original saved binding numbers. Printable
   letters follow the layout's keycode; physical OEM positions retain VK IDs. */
static unsigned binding_key(SDL_Scancode scan, SDL_Keycode key)
{
    if (key >= SDLK_A && key <= SDLK_Z)
        return 'A' + key - SDLK_A;
    if (key >= SDLK_0 && key <= SDLK_9)
        return '0' + key - SDLK_0;
    /* Non-Latin layouts still need usable legacy letter/digit bindings. */
    if (scan >= SDL_SCANCODE_A && scan <= SDL_SCANCODE_Z)
        return 'A' + scan - SDL_SCANCODE_A;
    if (scan >= SDL_SCANCODE_1 && scan <= SDL_SCANCODE_9)
        return '1' + scan - SDL_SCANCODE_1;
    if (scan == SDL_SCANCODE_0)
        return '0';
    if (scan >= SDL_SCANCODE_F1 && scan <= SDL_SCANCODE_F12)
        return 112 + scan - SDL_SCANCODE_F1;
    if (scan >= SDL_SCANCODE_F13 && scan <= SDL_SCANCODE_F24)
        return 124 + scan - SDL_SCANCODE_F13;
    if (scan >= SDL_SCANCODE_KP_1 && scan <= SDL_SCANCODE_KP_9)
        return 97 + scan - SDL_SCANCODE_KP_1;
    switch (scan)
    {
    case SDL_SCANCODE_BACKSPACE:
        return 8;
    case SDL_SCANCODE_TAB:
        return 9;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        return 13;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        return 16;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        return 17;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
        return 18;
    case SDL_SCANCODE_PAUSE:
        return 19;
    case SDL_SCANCODE_CAPSLOCK:
        return 20;
    case SDL_SCANCODE_ESCAPE:
        return 27;
    case SDL_SCANCODE_SPACE:
        return 32;
    case SDL_SCANCODE_PAGEUP:
        return 33;
    case SDL_SCANCODE_PAGEDOWN:
        return 34;
    case SDL_SCANCODE_END:
        return 35;
    case SDL_SCANCODE_HOME:
        return 36;
    case SDL_SCANCODE_LEFT:
        return 37;
    case SDL_SCANCODE_UP:
        return 38;
    case SDL_SCANCODE_RIGHT:
        return 39;
    case SDL_SCANCODE_DOWN:
        return 40;
    case SDL_SCANCODE_PRINTSCREEN:
        return 44;
    case SDL_SCANCODE_INSERT:
        return 45;
    case SDL_SCANCODE_DELETE:
        return 46;
    case SDL_SCANCODE_LGUI:
        return 91;
    case SDL_SCANCODE_RGUI:
        return 92;
    case SDL_SCANCODE_APPLICATION:
        return 93;
    case SDL_SCANCODE_KP_0:
        return 96;
    case SDL_SCANCODE_KP_MULTIPLY:
        return 106;
    case SDL_SCANCODE_KP_PLUS:
        return 107;
    case SDL_SCANCODE_KP_MINUS:
        return 109;
    case SDL_SCANCODE_KP_PERIOD:
        return 110;
    case SDL_SCANCODE_KP_DIVIDE:
        return 111;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return 144;
    case SDL_SCANCODE_SCROLLLOCK:
        return 145;
    case SDL_SCANCODE_SEMICOLON:
        return 186;
    case SDL_SCANCODE_EQUALS:
        return 187;
    case SDL_SCANCODE_COMMA:
        return 188;
    case SDL_SCANCODE_MINUS:
        return 189;
    case SDL_SCANCODE_PERIOD:
        return 190;
    case SDL_SCANCODE_SLASH:
        return 191;
    case SDL_SCANCODE_GRAVE:
        return 192;
    case SDL_SCANCODE_LEFTBRACKET:
        return 219;
    case SDL_SCANCODE_BACKSLASH:
        return 220;
    case SDL_SCANCODE_RIGHTBRACKET:
        return 221;
    case SDL_SCANCODE_APOSTROPHE:
        return 222;
    case SDL_SCANCODE_NONUSBACKSLASH:
        return 226;
    default:
        return 0;
    }
}

static void find_gamepad()
{
    if (gamepad && SDL_GamepadConnected(gamepad))
        return;
    if (gamepad)
        SDL_CloseGamepad(gamepad);
    gamepad = nullptr;
    text_gamepad_buttons = 0;
    int count = 0;
    auto *ids = SDL_GetGamepads(&count);
    for (int i = 0; i < count && !gamepad; ++i)
        gamepad = SDL_OpenGamepad(ids[i]);
    SDL_free(ids);
}
static void stick(SDL_GamepadAxis horizontal, SDL_GamepadAxis vertical, float &x, float &y)
{
    const int raw_x = SDL_GetGamepadAxis(gamepad, horizontal);
    const int raw_y = SDL_GetGamepadAxis(gamepad, vertical);
    x = raw_x / (raw_x < 0 ? 32768.0f : 32767.0f);
    y = raw_y / (raw_y < 0 ? 32768.0f : 32767.0f);
    const float length = std::sqrt(x * x + y * y);
    const float scale = length > .18f ? (std::min(length, 1.0f) - .18f) / (.82f * length) : 0.0f;
    x *= scale;
    y *= scale;
}
static float trigger(SDL_GamepadAxis axis)
{
    return std::clamp((SDL_GetGamepadAxis(gamepad, axis) / 32767.0f - .05f) / .95f, 0.0f, 1.0f);
}
static void read_gamepad(BHGamepadState *state, bool focused)
{
    *state = {};
    if (!focused || !gamepad || !SDL_GamepadConnected(gamepad))
        return;
    float x, y;
    stick(SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, x, y);
    state->axes[BH_PAD_STEER] = x;
    state->axes[BH_PAD_LIFT] = -y;
    state->axes[BH_PAD_MENU_Y] = y;
    stick(SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY, x, y);
    state->axes[BH_PAD_AIM_X] = x;
    state->axes[BH_PAD_AIM_Y] = -y;
    state->axes[BH_PAD_THRUST] = trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) - trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    static const SDL_GamepadButton buttons[] = {
        SDL_GAMEPAD_BUTTON_SOUTH,     SDL_GAMEPAD_BUTTON_EAST,          SDL_GAMEPAD_BUTTON_WEST,
        SDL_GAMEPAD_BUTTON_NORTH,     SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
        SDL_GAMEPAD_BUTTON_START,     SDL_GAMEPAD_BUTTON_BACK,          SDL_GAMEPAD_BUTTON_DPAD_UP,
        SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT,     SDL_GAMEPAD_BUTTON_DPAD_RIGHT};
    for (unsigned i = 0; i < std::size(buttons); ++i)
        if (SDL_GetGamepadButton(gamepad, buttons[i]))
            state->buttons |= 1UL << i;
    // Keep a text editor's confirm/cancel press out of the menu below it.
    text_gamepad_buttons &= state->buttons;
    state->buttons &= ~text_gamepad_buttons;
}

void reset(BHPlatformInput callback, void *user)
{
    input_callback = callback;
    held_keys.fill(0);
    message_user = user;
}

void set_window(SDL_Window *output)
{
    window = output;
}

void open_gamepad()
{
    /* A missing controller subsystem must not prevent keyboard play. */
    if (SDL_InitSubSystem(SDL_INIT_GAMEPAD))
        find_gamepad();
}

void close()
{
    bh_sdl_text_end();
    if (gamepad)
    {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }
    window = nullptr;
}

void handle_event(const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
        find_gamepad();
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (gamepad && event.gbutton.which == SDL_GetGamepadID(gamepad) &&
            (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH ||
             event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST))
        {
            const bool submit = event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH;
            const unsigned long button = 1UL << (submit ? BH_PAD_SOUTH : BH_PAD_EAST);
            if (event.type == SDL_EVENT_GAMEPAD_BUTTON_UP)
                text_gamepad_buttons &= ~button;
            else if (text_callback)
            {
                text_gamepad_buttons |= button;
                text_callback(submit ? BH_PLATFORM_TEXT_SUBMIT : BH_PLATFORM_TEXT_CANCEL, nullptr, text_user);
            }
        }
        break;
    case SDL_EVENT_TEXT_INPUT:
        if (text_callback && event.text.windowID == SDL_GetWindowID(window))
            text_callback(BH_PLATFORM_TEXT_INSERT, event.text.text, text_user);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        if (event.key.windowID != SDL_GetWindowID(window))
            break;
        auto scan = event.key.scancode;
        if (scan <= SDL_SCANCODE_UNKNOWN || scan >= SDL_SCANCODE_COUNT)
            break;
        if (event.key.down)
        {
            if (text_callback)
            {
                const auto key = event.key.key;
                if (key == SDLK_BACKSPACE)
                {
                    text_callback(BH_PLATFORM_TEXT_BACKSPACE, nullptr, text_user);
                    break;
                }
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_ESCAPE)
                {
                    if (!event.key.repeat)
                        text_callback(key == SDLK_ESCAPE ? BH_PLATFORM_TEXT_CANCEL : BH_PLATFORM_TEXT_SUBMIT,
                                      nullptr, text_user);
                    break;
                }
            }
            if (!held_keys[scan])
                held_keys[scan] = binding_key(scan, event.key.key);
            if (held_keys[scan])
                input_event(BH_PLATFORM_KEY, held_keys[scan], true, event.key.repeat);
        }
        else
        {
            const unsigned key = held_keys[scan];
            held_keys[scan] = 0;
            /* Two physical modifiers may share a saved binding ID. */
            if (key && std::find(held_keys.begin(), held_keys.end(), key) == held_keys.end())
                input_event(BH_PLATFORM_KEY, key, false);
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.windowID == SDL_GetWindowID(window))
        {
            unsigned key = event.button.button == SDL_BUTTON_LEFT     ? 1
                           : event.button.button == SDL_BUTTON_RIGHT  ? 2
                           : event.button.button == SDL_BUTTON_MIDDLE ? 4
                           : event.button.button == SDL_BUTTON_X1     ? 5
                           : event.button.button == SDL_BUTTON_X2     ? 6
                                                                      : 0;
            if (key)
                input_event(BH_PLATFORM_KEY, key, event.button.down);
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WINDOW_MINIMIZED:
        if (event.window.windowID == SDL_GetWindowID(window))
            lose_focus();
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        if (event.window.windowID == SDL_GetWindowID(window))
            input_event(BH_PLATFORM_FOCUS, 0, true);
        break;
    default:
        break;
    }
}
} // namespace bh_sdl_input

extern "C"
{
    int bh_sdl_active()
    {
        return window && (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) &&
               !(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED);
    }
    void bh_sdl_set_relative_mouse(int enabled)
    {
        const bool relative = enabled && bh_sdl_active();
        if (window && SDL_GetWindowRelativeMouseMode(window) != relative &&
            SDL_SetWindowRelativeMouseMode(window, relative))
            SDL_GetRelativeMouseState(nullptr, nullptr);
    }
    void bh_sdl_relative_mouse(float *x, float *y)
    {
        *x = *y = 0;
        if (bh_sdl_active() && SDL_GetWindowRelativeMouseMode(window))
            SDL_GetRelativeMouseState(x, y);
    }
    unsigned bh_sdl_key_character(unsigned key)
    {
        if (key >= 'A' && key <= 'Z')
            return key;
        if (key >= '0' && key <= '9')
            return key;
        if (key == 32)
            return 32;
        if (key >= 96 && key <= 105)
            return '0' + key - 96;
        for (int i = 1; i < SDL_SCANCODE_COUNT; ++i)
        {
            auto scan = (SDL_Scancode)i;
            auto code = SDL_GetKeyFromScancode(scan, SDL_KMOD_NONE, false);
            if (bh_sdl_input::binding_key(scan, code) == key && code >= 32 && code < 127)
                return (unsigned)code;
        }
        return 0;
    }
    int bh_sdl_text_begin(BHPlatformTextInput callback, void *user)
    {
        if (!window || !callback || text_callback)
            return 0;
        const auto properties = SDL_CreateProperties();
        const bool started = properties &&
                             SDL_SetBooleanProperty(properties, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN, false) &&
                             SDL_StartTextInputWithProperties(window, properties);
        SDL_DestroyProperties(properties);
        if (!started)
            return 0;
        text_callback = callback;
        text_user = user;
        return 1;
    }
    void bh_sdl_text_end()
    {
        if (!text_callback)
            return;
        text_callback = nullptr;
        text_user = nullptr;
        SDL_StopTextInput(window);
    }
    void __cdecl bh_sdl_gamepad(BHGamepadState *state)
    {
        bh_sdl_input::read_gamepad(state,
                                   window && (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) &&
                                       !(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED));
    }
}
