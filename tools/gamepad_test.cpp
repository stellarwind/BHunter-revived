// Real SDL virtual controllers drive the production sampler and input packets.
// No window, GPU, desktop input, or diagnostic hooks in game code are needed.
#include "../src/bh_sdl_gpu.cpp"
extern "C"
{
#include "../src/bh_input_devices.h"
}
#include <cassert>

static unsigned checks;
#define CHECK(condition)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        ++checks;                                                                                                      \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            std::fprintf(stderr, "Gamepad check failed at line %d: %s\n", __LINE__, #condition);                       \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

static bool focused = true;
static unsigned legacy_polls, captures, recenters;
static void __cdecl sample_pad(BHGamepadState *state)
{
    read_gamepad(state, focused);
}
static MMRESULT WINAPI legacy_sample(UINT, LPJOYINFOEX)
{
    ++legacy_polls;
    return JOYERR_UNPLUGGED;
}
static MMRESULT WINAPI legacy_capture(HWND, UINT, UINT, BOOL)
{
    ++captures;
    return JOYERR_UNPLUGGED;
}
static BOOL WINAPI recenter(int, int)
{
    ++recenters;
    return TRUE;
}
static float axis_value(const BHPlayerInputState &state, unsigned index)
{
    float result;
    std::memcpy(&result, state.values + index, sizeof(result));
    return result;
}
static bool close_axis(float a, float b)
{
    return std::fabs(a - b) < .0001f;
}
struct VirtualPad
{
    SDL_JoystickID id;
    SDL_Joystick *joystick;
    VirtualPad()
    {
        SDL_VirtualJoystickDesc desc{};
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.axis_mask = (1u << desc.naxes) - 1;
        desc.button_mask = (1u << desc.nbuttons) - 1;
        desc.name = "BHunter isolated gamepad regression";
        id = SDL_AttachVirtualJoystick(&desc);
        CHECK(id && SDL_IsGamepad(id));
        joystick = SDL_OpenJoystick(id);
        CHECK(joystick);
        axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
        axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);
        CHECK(bh_sdl_poll());
    }
    void axis(SDL_GamepadAxis index, Sint16 value)
    {
        CHECK(SDL_SetJoystickVirtualAxis(joystick, index, value));
    }
    void button(SDL_GamepadButton index, bool down)
    {
        CHECK(SDL_SetJoystickVirtualButton(joystick, index, down));
    }
    void disconnect()
    {
        CHECK(SDL_DetachVirtualJoystick(id));
        SDL_CloseJoystick(joystick);
        joystick = nullptr;
    }
};
struct Input
{
    BHPlayerInputOwner owner{};
    BHInputSnapshot snapshot{};
    BHInputCollection collector{};
    BHPlayerInputState decoded{};
    BHPlayerInputList *list;
    Input()
    {
        CHECK(bh_player_input_owner_init(&owner, nullptr));
        CHECK(bh_input_collection_init(&collector, &snapshot, &owner));
        BHInputDevices devices{};
        CHECK(bh_input_devices_bind(&devices, &collector, nullptr));
        devices.api.capture = legacy_capture;
        collector.api.recenter = recenter;
        CHECK(bh_input_devices_start(&devices, nullptr));
        CHECK(!collector.enabled[0] && !collector.enabled[1] && !captures);
        bh_input_devices_dispose(&devices);
        collector.disable_mouse_motion = 1;
        collector.api.joystick = legacy_sample;
        collector.api.gamepad = sample_pad;
        list = bh_player_input_create(&owner, 1);
        CHECK(list);
        BHInputProfile profile{};
        auto *bytes = reinterpret_cast<unsigned char *>(&profile);
        bytes[0x20] = 'W'; // Forward thrust.
        bytes[0x21] = 'S';
        bytes[0x28] = VK_CONTROL; // Primary fire.
        bh_player_input_setup(list, &profile);
    }
    ~Input()
    {
        bh_player_input_owner_dispose(&owner);
    }
    void tick()
    {
        CHECK(bh_sdl_poll());
        unsigned long changed;
        CHECK(bh_input_collect(&collector, &changed));
        CHECK(collector.error == BH_INPUT_COLLECTION_OK);
        CHECK(bh_player_input_apply_packet_checked(&decoded, list->state, sizeof(list->state)));
        CHECK(!legacy_polls);
    }
};

static void disconnect_round(bool keyboard_held)
{
    Input input;
    VirtualPad pad;
    CHECK(gamepad && SDL_GetGamepadID(gamepad) == pad.id);
    input.tick();
    CHECK(close_axis(axis_value(input.decoded, 2), 0));
    pad.axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
    pad.axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
    pad.axis(SDL_GAMEPAD_AXIS_RIGHTX, 16384);
    pad.axis(SDL_GAMEPAD_AXIS_RIGHTY, -16384);
    pad.button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
    pad.button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
    input.tick();
    CHECK(close_axis(axis_value(input.decoded, 2), 1));
    CHECK(close_axis(axis_value(input.decoded, 0), 1));
    CHECK(axis_value(input.decoded, 25) > 0 && axis_value(input.decoded, 26) > 0);
    CHECK(input.decoded.values[22] == 1 && input.decoded.values[3] == 1);
    CHECK(bh_player_input_transition(&input.decoded, 22) == 0);
    if (keyboard_held)
    {
        input.snapshot.current['W'] = 1;
        input.snapshot.current[VK_CONTROL] = 1;
        input.tick();
        CHECK(bh_player_input_transition(&input.decoded, 22) == 2);
    }
    pad.disconnect();
    BHGamepadState immediately{};
    read_gamepad(&immediately, true);
    CHECK(!immediately.buttons && close_axis(immediately.axes[BH_PAD_THRUST], 0));
    input.tick();
    CHECK(!gamepad);
    CHECK(close_axis(axis_value(input.decoded, 0), 0));
    CHECK(close_axis(axis_value(input.decoded, 25), 0) && close_axis(axis_value(input.decoded, 26), 0));
    CHECK(close_axis(axis_value(input.decoded, 2), keyboard_held ? 1 : 0));
    CHECK(input.decoded.values[3] == 0); // Unheld fire cannot remain stuck.
    CHECK(bh_player_input_transition(&input.decoded, 22) == (keyboard_held ? 2 : 1));
    // Keyboard takes over without a restart or a controller-dependent mode.
    input.snapshot.current['W'] = 1;
    input.snapshot.current[VK_CONTROL] = 1;
    input.tick();
    CHECK(close_axis(axis_value(input.decoded, 2), 1));
    CHECK(bh_player_input_transition(&input.decoded, 22) == (keyboard_held ? 2 : 0));
    input.snapshot.current['W'] = 0;
    input.snapshot.current[VK_CONTROL] = 0;
    input.tick();
    CHECK(close_axis(axis_value(input.decoded, 2), 0));
    CHECK(bh_player_input_transition(&input.decoded, 22) == 1);
    for (int i = 0; i < 4; ++i)
    {
        input.tick();
        CHECK(close_axis(axis_value(input.decoded, 2), 0));
        CHECK(bh_player_input_transition(&input.decoded, 22) == 3);
    }
}

int main()
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    CHECK(SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS));
    CHECK(!gamepad);
    // Repeat the complete removal/fallback sequence after a fresh connection.
    disconnect_round(false);
    disconnect_round(true);
    {
        Input input;
        VirtualPad pad;
        pad.axis(SDL_GAMEPAD_AXIS_LEFTX, 2000);
        pad.axis(SDL_GAMEPAD_AXIS_LEFTY, -2000);
        input.tick();
        CHECK(close_axis(axis_value(input.decoded, 0), 0) && close_axis(axis_value(input.decoded, 1), 0));
        pad.axis(SDL_GAMEPAD_AXIS_LEFTX, 0);
        pad.axis(SDL_GAMEPAD_AXIS_LEFTY, -32768);
        pad.axis(SDL_GAMEPAD_AXIS_RIGHTX, 32767);
        pad.axis(SDL_GAMEPAD_AXIS_RIGHTY, 0);
        pad.axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
        input.tick();
        CHECK(close_axis(axis_value(input.decoded, 1), 1));
        CHECK(close_axis(axis_value(input.decoded, 25), 1));
        CHECK(close_axis(axis_value(input.decoded, 2), (.5f - .05f) / .95f));
        pad.axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);
        pad.axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
        pad.button(SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
        pad.button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
        pad.button(SDL_GAMEPAD_BUTTON_BACK, true);
        input.tick();
        CHECK(close_axis(axis_value(input.decoded, 2), -1));
        CHECK(input.decoded.values[27] == 1 && input.decoded.values[28] == 1);
        CHECK(input.decoded.values[6] == 1);
        focused = false;
        input.tick();
        CHECK(close_axis(axis_value(input.decoded, 2), 0) && input.decoded.values[6] == 0);
        focused = true;
        input.tick();
        CHECK(close_axis(axis_value(input.decoded, 2), -1));
        VirtualPad second;
        CHECK(SDL_GetGamepadID(gamepad) == pad.id); // Attaching a second pad cannot steal control.
        pad.disconnect();
        input.tick();
        CHECK(gamepad && SDL_GetGamepadID(gamepad) == second.id);
        CHECK(close_axis(axis_value(input.decoded, 2), 0) && input.decoded.values[6] == 0);
        second.disconnect();
        input.tick();
        CHECK(!gamepad);
    }
    CHECK(recenters == 3); // Startup only; collecting never moves the cursor.
    bh_sdl_close();
    std::printf("Gamepad regression: %u checks passed; disconnect/fallback repeated twice.\n", checks);
}
