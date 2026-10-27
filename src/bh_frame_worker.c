#include "bh_frame_worker.h"
#include <stdio.h>
#include <string.h>

static void failure(BHFrameWorker *w, BHFrameWorkerError e)
{
    ++w->failures;
    if (w->error == BH_FRAME_WORKER_OK)
        w->error = e;
}
static void pause_worker(BHFrameWorker *w, DWORD delay)
{
    ++w->sleeps;
    Sleep(delay);
}
static void result(BHFrameWorker *w, HRESULT hr)
{
    ++w->native_calls;
    w->last_hresult = hr;
    if (FAILED(hr))
        failure(w, BH_FRAME_WORKER_NATIVE);
}
static void restore(BHFrameWorker *w, LPDIRECTDRAWSURFACE surface)
{
    HRESULT hr;
    if (!surface)
        return;
    hr = IDirectDrawSurface_IsLost(surface);
    ++w->native_calls;
    w->last_hresult = hr;
    if (hr == DDERR_SURFACELOST)
    {
        ++w->restores;
        result(w, IDirectDrawSurface_Restore(surface));
    }
    else if (FAILED(hr))
        failure(w, BH_FRAME_WORKER_NATIVE);
}
static void capture(BHFrameWorker *w)
{
    char filename[512];
    FILE *file = NULL;
    int index;
    const char *prefix = w->dump_prefix ? w->dump_prefix : "c:\\DUMP";
    if (strlen(prefix) > 480)
    {
        failure(w, BH_FRAME_WORKER_CAPTURE);
        w->request_capture = 0;
        return;
    }
    /* At the cap the original closes the final readable file, then overwrites
       that same filename. An unreadable file is treated as available too. */
    for (index = 0; index < 10000; ++index)
    {
        if (file)
            fclose(file);
        sprintf(filename, "%s%.4d.BMP", prefix, index);
        file = fopen(filename, "rb");
        if (!file)
            break;
    }
    if (file)
        fclose(file);
    if (!bh_view_reset_viewport(w->viewport, w->width, w->height))
        failure(w, BH_FRAME_WORKER_RENDER);
    w->render->display->left = w->viewport->left;
    w->render->display->top = w->viewport->top;
    w->capture->left = w->viewport->left;
    w->capture->top = w->viewport->top;
    w->capture->right = w->viewport->right;
    w->capture->bottom = w->viewport->bottom;
    pause_worker(w, 250);
    ++w->captures;
    if (!bh_capture_write(w->capture, filename))
        failure(w, BH_FRAME_WORKER_CAPTURE);
    w->request_capture = 0;
}
DWORD WINAPI bh_frame_worker_run(void *user)
{
    BHFrameWorker *w = (BHFrameWorker *)user;
    BHFrameBuffers *f;
    BHDrawQueue *saved;
    BHFrameDisplay *display;
    BHPresentContext *present;
    BHFadeContext *fade;
    long best, age, i;
    BOOL released;
    if (!w)
        return 0;
    w->done = 0;
    w->error = BH_FRAME_WORKER_OK;
    w->selected = -1;
    if (!(f = w->frames) || !f->mutex || !w->render || !w->callback || !w->callback->render || !w->begin ||
        !w->begin->present || !w->primary || !w->viewport || w->width <= 0 || w->height <= 0 ||
        !(display = w->render->display) || !(present = w->begin->present) || !(fade = present->fade) || !fade->render ||
        !display->textures || display->textures != fade->render->textures || display->color != present->color ||
        display->auxiliary != present->auxiliary)
    {
        failure(w, BH_FRAME_WORKER_ARGUMENT);
        w->done = 1;
        return 0;
    }
    while (!w->ready)
        pause_worker(w, 10);
    do
    {
        ++w->iterations;
        do
        {
            ++w->waits;
            w->last_wait = WaitForSingleObject(f->mutex, 5);
            if (w->last_wait == WAIT_TIMEOUT)
                ++w->timeouts;
        } while (w->last_wait == WAIT_TIMEOUT);
        if (w->last_wait != WAIT_OBJECT_0 && w->last_wait != WAIT_ABANDONED_0)
        {
            failure(w, BH_FRAME_WORKER_WAIT);
            break;
        }
        best = -1;
        age = 4;
        for (i = 0; i < 3; ++i)
            if (!f->slots[i].busy && f->slots[i].age < age)
            {
                best = i;
                age = f->slots[i].age;
            }
        w->selected = best;
        if (best >= 0 && age != -9999)
            f->slots[best].busy = 1;
        released = ReleaseMutex(f->mutex);
        if (!released)
        {
            failure(w, BH_FRAME_WORKER_WAIT);
            break;
        }
        if (best < 0)
        {
            ++w->skipped;
            failure(w, BH_FRAME_WORKER_EMPTY);
        }
        else if (age == -9999)
            ++w->skipped;
        else
        {
            if (!w->queues[best] || w->queues[best]->commands != f->slots[best].data)
            {
                failure(w, BH_FRAME_WORKER_BINDING);
                f->slots[best].busy = 0;
                break;
            }
            if (!bh_present_begin(w->begin))
                failure(w, BH_FRAME_WORKER_RENDER);
            if (display->textures->software)
            {
                display->color_pixels = fade->pixels;
                display->color_pitch = fade->pitch;
                display->auxiliary_pixels = w->begin->auxiliary_pixels;
                display->auxiliary_pitch = w->begin->auxiliary_pitch;
                display->locked = present->locked;
            }
            saved = w->render->draw;
            w->render->draw = w->queues[best];
            if (!w->callback->render(w->render))
                failure(w, BH_FRAME_WORKER_RENDER);
            w->render->draw = saved;
            ++w->rendered;
            if (display->textures->software)
            {
                fade->pixels = display->color_pixels;
                fade->pitch = display->color_pitch;
                present->locked = display->locked;
            }
            if (w->request_capture)
            {
                if (w->capture)
                    capture(w);
                else
                {
                    failure(w, BH_FRAME_WORKER_CAPTURE);
                    w->request_capture = 0;
                }
            }
            if (!bh_present_end(w->begin->present))
                failure(w, BH_FRAME_WORKER_RENDER);
            display->locked = present->locked;
            /* The producer may reuse this allocation while presentation waits. */
            f->slots[best].busy = 0;
            if (w->display_state != 3)
            {
                do
                {
                    pause_worker(w, 10);
                } while (w->display_state != 3);
                restore(w, w->primary);
                restore(w, w->back);
            }
            ++w->flips;
            result(w, IDirectDrawSurface_Flip(w->primary, NULL, DDFLIP_WAIT));
        }
    } while (!w->stop);
    w->done = 1;
    return 0;
}
