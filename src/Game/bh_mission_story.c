#include "bh_mission_story.h"
#include <string.h>
typedef enum StoryAction
{
    STORY_MESSAGE,
    STORY_COMPLETE,
    STORY_UNLOCK_1,
    STORY_UNLOCK_2,
    STORY_UNLOCK_3,
    STORY_UNLOCK_4,
    STORY_UNLOCK_5,
    STORY_UNLOCK_6,
    STORY_PROGRESS_ZERO,
    STORY_PROGRESS_SEVEN,
    STORY_PROGRESS_EIGHT,
    STORY_CLEAR,
    STORY_END_ONE,
    STORY_FINALE
} StoryAction;
typedef struct StoryMessage
{
    long id;
    StoryAction action;
    const char *text;
} StoryMessage;
#include "bh_mission_story_data.h"
static void cue(BHMissionStory *story, long progression)
{
    if (!story->progress)
    {
        story->error = 1;
        return;
    }
    ++story->cues;
    bh_mission_progress(story->progress, progression);
    if (story->progress->error)
        story->error = 3;
}
static void clear(BHMissionStory *story)
{
    unsigned long visited;
    if (!story->player || !story->actors || !story->actor_limit)
    {
        story->error = 1;
        return;
    }
    story->player->player.bounty_pending = 0;
    story->player->player.fine = 0;
    ++story->broadcasts;
    if (!bh_player_input_broadcast(*story->actors, story->actor_limit, &visited))
        story->error = 4;
}
static void enable(BHMissionStory *story, unsigned long row, unsigned long index)
{
    if (!story->availability[row][index])
    {
        story->availability[row][index] = 1;
        ++story->availability[row][31];
    }
}
int bh_mission_story_dispatch(BHMissionStory *story, const long packet[3])
{
    const StoryMessage *message = 0;
    unsigned long message_index, weapon_slot;
    BHEntity *weapon, *actual;
    BHWeaponObjectState *weapon_state;
    unsigned long handle;
    if (!story || !packet)
    {
        if (story)
            story->error = 1;
        return 0;
    }
    story->error = 0;
    ++story->calls;
    if (packet[0] != 14)
        return 1;
    for (message_index = 0; message_index < sizeof(messages) / sizeof(messages[0]); ++message_index)
        if (messages[message_index].id == packet[2])
        {
            message = messages + message_index;
            break;
        }
    if (message)
    {
        if (!story->messages)
        {
            story->error = 1;
            return 0;
        }
        ++story->queued;
        if (!bh_message_enqueue(story->messages, message->text))
            story->error = 2;
        switch (message->action)
        {
        case STORY_MESSAGE:
            break;
        case STORY_COMPLETE:
        case STORY_PROGRESS_ZERO:
            cue(story, 0);
            break;
        case STORY_UNLOCK_1:
        case STORY_UNLOCK_2:
        case STORY_UNLOCK_3:
        case STORY_UNLOCK_4:
        case STORY_UNLOCK_5:
        case STORY_UNLOCK_6:
            cue(story, (long)message->action - STORY_UNLOCK_1 + 1);
            break;
        case STORY_PROGRESS_SEVEN:
            cue(story, 7);
            break;
        case STORY_PROGRESS_EIGHT:
            cue(story, 8);
            break;
        case STORY_CLEAR:
            clear(story);
            break;
        case STORY_END_ONE:
            clear(story);
            cue(story, 0);
            if (story->ending)
                *story->ending = 1;
            else
                story->error = 1;
            break;
        case STORY_FINALE:
            clear(story);
            if (!story->player || !story->camera || story->camera != &story->player->player.camera ||
                !story->force_control || !story->force_gate)
            {
                story->error = 1;
                break;
            }
            if (!bh_camera_state_set_mode(story->camera, 8))
                story->error = 5;
            *story->force_control = bh_player_force_control_a;
            *story->force_gate = 0;
            break;
        }
        return !story->error;
    }
    switch (packet[2])
    {
    case 0x960:
        if (!story->ending)
        {
            story->error = 1;
            break;
        }
        *story->ending = 2;
        break;
    case 3000:
        cue(story, 0);
        break;
    case 3001:
    case 3002:
    case 3003:
    case 3004:
    case 3005:
        if (!story->availability)
        {
            story->error = 1;
            break;
        }
        switch (packet[2])
        {
        case 3001:
            enable(story, 0, 0);
            enable(story, 0, 10);
            enable(story, 1, 10);
            enable(story, 1, 3);
            enable(story, 2, 10);
            enable(story, 2, 3);
            enable(story, 3, 5);
            break;
        case 3002:
            enable(story, 0, 3);
            enable(story, 1, 11);
            enable(story, 1, 5);
            enable(story, 2, 11);
            enable(story, 2, 5);
            enable(story, 3, 7);
            break;
        case 3003:
            enable(story, 0, 1);
            enable(story, 1, 7);
            enable(story, 2, 6);
            break;
        case 3004:
            enable(story, 0, 2);
            enable(story, 1, 12);
            enable(story, 2, 12);
            enable(story, 3, 12);
            enable(story, 3, 8);
            break;
        case 3005:
            enable(story, 0, 6);
            break;
        }
        break;
    case 3006:
        if (!story->sound)
        {
            story->error = 1;
            return 0;
        }
        handle = bh_sound_request(story->sound, story->pickup_sound, 1, .7f, 0, 0);
        if (handle == BH_SOUND_INVALID)
            story->error = 6;
        if (!story->player || !story->weapons)
        {
            story->error = 1;
            break;
        }
        for (weapon_slot = 5; weapon_slot < 10; ++weapon_slot)
        {
            if (!bh_weapon_registry_resolve(story->weapons, weapon_slot, &weapon, &weapon_state))
            {
                story->error = 1;
                break;
            }
            actual = story->player->player.weapons[weapon_slot];
            if (actual != weapon)
            {
                story->error = 1;
                break;
            }
            weapon_state->common.ammo = 0;
            if (weapon_state->common.ammo_limit < 0)
                weapon_state->common.ammo = weapon_state->common.ammo_limit;
        }
        break;
    }
    return !story->error;
}
void bh_mission_story_query(BHEntity *entity, void *packet)
{
    BHMissionStory *story;
    memcpy(&story, &entity->bindings.mission_story, 4);
    if (story)
        bh_mission_story_dispatch(story, (const long *)packet);
}
int bh_mission_story_bind(BHMissionStory *story, BHEntity *entity)
{
    BHEntity *active_entity;
    BHEntityPool *pool;
    unsigned long visited = 0;
    if (!story || !entity || !story->progress || !story->progress->lifetime ||
        !(pool = story->progress->lifetime->timers.pool))
        return 0;
    for (active_entity = pool->active; active_entity && visited++ < pool->capacity;
         active_entity = active_entity->next)
        if (active_entity == entity)
        {
            memcpy(&entity->bindings.mission_story, &story, 4);
            return bh_entity_set_query(pool, entity, bh_mission_story_query);
        }
    return 0;
}
