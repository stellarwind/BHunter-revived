#include "bh_window_dispatch.h"
#include "bh_force_feedback_start.h"
#include "bh_cd_notify.h"

LRESULT __cdecl bh_window_default_message(void *user, HWND window, UINT message, WPARAM key, LPARAM bits)
{
    return DefWindowProcA(window, message, key, bits);
}
static void result(BHWindowDispatch *c, HRESULT hr)
{
    ++c->native_calls;
    c->last_hresult = hr;
    if (FAILED(hr))
        ++c->failures;
}
static void restore(BHWindowDispatch *c, LPDIRECTDRAWSURFACE *slot)
{
    HRESULT hr;
    if (!slot || !*slot)
        return;
    hr = IDirectDrawSurface_IsLost(*slot);
    ++c->native_calls;
    c->last_hresult = hr;
    if (hr == DDERR_SURFACELOST)
    {
        ++c->restores;
        result(c, IDirectDrawSurface_Restore(*slot));
    }
    else if (FAILED(hr))
        ++c->failures;
}
static void focus(BHWindowDispatch *c)
{
    ++c->focus_starts;
    if (!c->force || !bh_force_feedback_start(c->force))
        ++c->failures;
}
static void close_window(BHWindowDispatch *c)
{
    if (*c->thread_active)
    {
        ++c->terminated;
        c->last_terminate = TerminateThread(*c->thread, 0);
        if (!c->last_terminate)
            ++c->failures;
        *c->thread_active = 0;
    }
    if (*c->draw)
        result(c, IDirectDraw_RestoreDisplayMode(*c->draw));
    if (*c->back)
    {
        IDirectDrawSurface_Release(*c->back);
        *c->back = NULL;
        ++c->releases;
    }
    if (*c->primary)
    {
        IDirectDrawSurface_Release(*c->primary);
        *c->primary = NULL;
        ++c->releases;
    }
    if (*c->draw)
    {
        IDirectDraw_Release(*c->draw);
        *c->draw = NULL;
        ++c->releases;
    }
    ++c->quits;
    PostQuitMessage(0);
}
LRESULT __cdecl bh_window_dispatch(BHWindowDispatch *c, HWND window, UINT message, WPARAM key, LPARAM bits)
{
    PAINTSTRUCT paint;
    unsigned long command;
    int hide = 0;
    if (!c)
        return 0;
    ++c->calls;
    if (!c->frames || !c->frames->message || !c->display_state || !c->capture_request || !c->thread_active ||
        !c->hide_cursor || !c->thread || !c->draw || !c->primary || !c->back)
    {
        ++c->failures;
        return 0;
    }
    switch (message)
    {
    case WM_SETFOCUS:
        *c->display_state |= 1;
        focus(c);
        break;
    case WM_KILLFOCUS:
        *c->display_state &= ~1L;
        break;
    case WM_ACTIVATEAPP:
        *c->display_state |= 2;
        focus(c);
        break;
    case WM_PAINT:
        BeginPaint(window, &paint);
        ++c->paints;
        restore(c, c->primary);
        restore(c, c->back);
        EndPaint(window, &paint);
        return 1;
    case WM_DESTROY:
    case WM_CLOSE:
        close_window(c);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (!((unsigned long)bits & 0x40000000UL) && key == 0x7b)
            *c->capture_request = 1;
        hide = 1;
        break;
    case WM_NCMOUSEMOVE:
    case WM_MOUSEMOVE:
        hide = 1;
        break;
    case WM_SYSCOMMAND:
        command = (unsigned long)key & 0xfff0UL;
        if (command == 0xf140 || command == 0xf170)
            return 0;
        if (key == 0xf020)
            *c->display_state &= ~2L;
        else if (key == 0xf120)
        {
            *c->display_state |= 2;
            focus(c);
        }
        break;
    case 0x3b9:
        ++c->cd_notifications;
        if (!c->cd || !bh_cd_notify(c->cd, (short)key, (MCIDEVICEID)bits))
            ++c->failures;
        break;
    }
    if (hide && *c->hide_cursor)
    {
        ++c->cursor_hides;
        c->last_cursor = ShowCursor(FALSE);
    }
    ++c->hooks;
    return c->frames->message(c->frames->message_user, window, message, key, bits);
}
