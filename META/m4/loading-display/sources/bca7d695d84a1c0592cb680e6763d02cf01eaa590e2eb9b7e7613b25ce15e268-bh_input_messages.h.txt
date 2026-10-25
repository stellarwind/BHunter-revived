#ifndef BH_INPUT_MESSAGES_H
#define BH_INPUT_MESSAGES_H
#include <windows.h>
#include "bh_input_evaluate.h"
/* Complete 00473c40 over the same explicit snapshot consumed by00474020.
   Key0xff deliberately aliases previous[0]. Initial keydown appends low-byte
   VK to32-byte history, wraps head with&31, and sets key state; repeats are
   swallowed. Keyup clears only when lParam bit30 says previously down. Left,
   right,middle buttons write VK1,2,4. Other messages call real DefWindowProcA.
   Snapshot storage must remain live; no input is sent to any external window.
   NULL snapshot source guard delegates without mutation. */
LRESULT __cdecl bh_input_message(BHInputSnapshot *,HWND,UINT,WPARAM,LPARAM);
#endif
