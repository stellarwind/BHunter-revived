#ifndef BH_GAMEPAD_H
#define BH_GAMEPAD_H

/* Shared C ABI between the SDL adapter and the restored input collector.
   Values are normalized and describe controls independently of pad labels. */
typedef enum BHGamepadAxis
{
    BH_PAD_STEER,
    BH_PAD_LIFT,
    BH_PAD_THRUST,
    BH_PAD_AIM_X,
    BH_PAD_AIM_Y,
    BH_PAD_MENU_Y,
    BH_PAD_AXIS_COUNT
} BHGamepadAxis;

typedef enum BHGamepadButton
{
    BH_PAD_SOUTH,
    BH_PAD_EAST,
    BH_PAD_WEST,
    BH_PAD_NORTH,
    BH_PAD_LEFT_SHOULDER,
    BH_PAD_RIGHT_SHOULDER,
    BH_PAD_START,
    BH_PAD_BACK,
    BH_PAD_UP,
    BH_PAD_DOWN,
    BH_PAD_LEFT,
    BH_PAD_RIGHT
} BHGamepadButton;

#define BH_INPUT_GAMEPAD 0x80
#define BH_INPUT_GAMEPAD_INDEX 0x7f

typedef struct BHGamepadState
{
    float axes[BH_PAD_AXIS_COUNT];
    unsigned long buttons;
} BHGamepadState;

#endif
