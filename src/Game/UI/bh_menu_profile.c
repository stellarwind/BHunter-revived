#include "bh_menu_profile.h"
#include <string.h>
static void refresh(BHMenuSettingsMenu *menu, long index)
{
    BHMenuSettingsRefresh callback;
    if (!menu || index < 0 || (unsigned long)index >= menu->row_count || !menu->rows || !menu->rows[index])
        return;
    callback = menu->rows[index]->refresh;
    if (callback)
        callback(menu, index);
}
int bh_menu_profile_commit(BHMenuProfileContext *context, BHMenuSettingsMenu *menu, const char *name, long index)
{
    BHInputProfile *current, *input_profile, *found = NULL;
    unsigned long i, n;
    int okay = 1;
    if (!context || !context->config || !context->current || !context->pending_selection || !menu || !name)
        return 0;
    context->error = 0;
    if (!memchr(name, 0, 16))
    {
        context->error = 1;
        return 0;
    }
    n = (unsigned long)strlen(name);
    if (n)
    {
        if (!bh_config_valid(context->config) || !(*context->current))
        {
            context->error = 1;
            return 0;
        }
        current = *context->current;
        input_profile = current;
        /* Require the actual current pointer to belong to the real config ring. */
        found = context->config->profiles;
        for (i = 0; i < context->config->profile_count && found != current; ++i)
            found = found->next;
        if (i == context->config->profile_count)
        {
            context->error = 1;
            return 0;
        }
        found = NULL;
        do
        {
            if (!_stricmp(name, (const char *)input_profile->payload))
            {
                found = input_profile;
                break;
            }
            input_profile = input_profile->next;
        } while (input_profile != current);
        if (found)
        {
            *context->current = found;
            refresh(menu, 0);
        }
        else
        {
            found = bh_config_profile_clone_after(context->config, current);
            if (found)
            {
                memcpy(found->payload, name, n + 1);
                found->flags = 1;
                *context->current = found;
                refresh(menu, 0);
            }
            else
            {
                context->error = 1;
                okay = 0;
            }
        }
        *context->pending_selection = 0;
    }
    refresh(menu, index);
    return okay;
}
