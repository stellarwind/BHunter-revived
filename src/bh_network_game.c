#include "bh_network_game.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static const GUID application = {0xfe8d1c00, 0xabfa, 0x11d2, {0x96, 0x35, 0, 0x20, 0x18, 0x2a, 0x83, 0x3a}};
typedef char player_layout[sizeof(BHNetworkPlayer) == 40 && offsetof(DPMSG_CREATEPLAYERORGROUP, dpId) == 8 &&
                                   offsetof(DPMSG_CREATEPLAYERORGROUP, dpnName) == 24
                               ? 1
                               : -1];
static void *__cdecl allocate(unsigned long n, void *user)
{
    (void)user;
    return malloc(n);
}
static void __cdecl release(void *p, void *user)
{
    (void)user;
    free(p);
}
int __cdecl bh_network_game_init(BHNetworkGame *g, BHNetworkSessions *s, const BHSceneAllocator *a)
{
    if (!g || !s || (a && (!a->allocate || !a->release)))
        return 0;
    memset(g, 0, sizeof(*g));
    g->sessions = s;
    if (a)
        g->allocator = *a;
    else
    {
        g->allocator.allocate = allocate;
        g->allocator.release = release;
    }
    return 1;
}
void __cdecl bh_network_game_dispose(BHNetworkGame *g)
{
    if (g && g->buffer)
    {
        g->allocator.release(g->buffer, g->allocator.user);
        g->buffer = NULL;
    }
}
static void description(DPSESSIONDESC2 *d)
{
    memset(d, 0, sizeof(*d));
    d->dwSize = sizeof(*d);
    d->guidApplication = application;
}
static void stop(BHNetworkGame *g)
{
    BHNetworkSessions *s = g->sessions;
    DPSESSIONDESC2 d;
    if (s->enumerating)
    {
        description(&d);
        s->enumerating = 0;
        ++s->enum_calls;
        s->enumeration_result = IDirectPlay3_EnumSessions(s->peer, &d, 10, bh_network_session_continue, s, 0xa1);
    }
}
static int valid(BHNetworkGame *g, const char *name)
{
    return g && g->sessions && g->allocator.allocate && g->allocator.release && name && memchr(name, 0, 32) != NULL;
}
static int ready(BHNetworkGame *g, const char *name, DPID id)
{
    int i;
    if (!g->buffer)
    {
        g->capacity = 1024;
        g->buffer = g->allocator.allocate(1024, g->allocator.user);
        if (!g->buffer)
            return 0;
    }
    g->player_count = 1;
    for (i = 1; i < 8; ++i)
        g->players[i].active = 0;
    g->players[0].active = 1;
    strcpy(g->players[0].name, name);
    g->players[0].id = id;
    g->local_player = id;
    return 1;
}
int __cdecl bh_network_host(BHNetworkGame *g, const char *name)
{
    DPSESSIONDESC2 d;
    DPNAME n;
    DPID id = 0;
    LPDIRECTPLAY3A peer;
    if (!valid(g, name) || !(peer = g->sessions->peer))
        return 0;
    stop(g);
    description(&d);
    d.dwFlags = 0xc4;
    d.dwMaxPlayers = 8;
    d.lpszSessionNameA = (char *)name;
    ++g->opens;
    g->open_result = IDirectPlay3_Open(peer, &d, 0x82);
    if (g->open_result != DP_OK)
        return 0;
    memset(&n, 0, sizeof(n));
    n.dwSize = sizeof(n);
    n.lpszShortNameA = (char *)name;
    ++g->creations;
    g->player_result = IDirectPlay3_CreatePlayer(peer, &id, &n, NULL, NULL, 0, 0x100);
    if (g->player_result != DP_OK)
    {
        ++g->closes;
        g->close_result = IDirectPlay3_Close(peer);
        return 0;
    }
    return ready(g, name, id);
}
int __cdecl bh_network_join(BHNetworkGame *g, const GUID *instance, const char *name)
{
    DPSESSIONDESC2 d;
    DPNAME n;
    DPID id = 0;
    LPDIRECTPLAY3A peer;
    if (!valid(g, name) || !instance || !(peer = g->sessions->peer))
        return 0;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    d.guidInstance = *instance;
    do
    {
        ++g->opens;
        g->open_result = IDirectPlay3_Open(peer, &d, 0x81);
    } while (g->open_result == DPERR_CONNECTING);
    if (g->open_result != DP_OK)
        return 0;
    memset(&n, 0, sizeof(n));
    n.dwSize = sizeof(n);
    n.lpszShortNameA = (char *)name;
    ++g->creations;
    g->player_result = IDirectPlay3_CreatePlayer(peer, &id, &n, NULL, NULL, 0, 0);
    if (g->player_result != DP_OK)
    {
        ++g->closes;
        g->close_result = IDirectPlay3_Close(peer);
        return 0;
    }
    return ready(g, name, id);
}
static void callback(BHNetworkGame *g, BHNetworkMessage fn, void *user, DWORD bytes)
{
    ++g->callbacks;
    fn(g->buffer, bytes, user);
}
int __cdecl bh_network_receive(BHNetworkGame *g, BHNetworkMessage fn, void *user)
{
    DPID from = 0, to;
    DWORD bytes, type;
    unsigned long i;
    LPDIRECTPLAY3A peer;
    if (!g || !g->buffer)
        return 0;
    if (!fn || !g->sessions || !(peer = g->sessions->peer))
    {
        g->error = 1;
        return 0;
    }
    for (;;)
    {
        do
        {
            bytes = g->capacity;
            to = g->local_player;
            ++g->receives;
            g->receive_result = IDirectPlay3_Receive(peer, &from, &to, 2, g->buffer, &bytes);
            if (g->receive_result == DPERR_BUFFERTOOSMALL)
            {
                g->allocator.release(g->buffer, g->allocator.user);
                g->buffer = g->allocator.allocate(bytes, g->allocator.user);
                if (!g->buffer)
                    return 0;
                g->capacity = bytes;
            }
        } while (g->receive_result == DPERR_BUFFERTOOSMALL);
        if (g->receive_result != DP_OK)
            return 1;
        if (from)
        {
            callback(g, fn, user, bytes);
            continue;
        }
        if (bytes < 4)
        {
            g->error = 2;
            return 0;
        }
        memcpy(&type, g->buffer, 4);
        switch (type)
        {
        case 3: {
            DPMSG_CREATEPLAYERORGROUP *m = (DPMSG_CREATEPLAYERORGROUP *)g->buffer;
            if (bytes < sizeof(*m) || !m->dpnName.lpszShortNameA)
            {
                g->error = 2;
                return 0;
            }
            for (i = 0; i < 8; ++i)
                if (!g->players[i].active)
                {
                    unsigned short slot = (unsigned short)i;
                    g->players[i].id = m->dpId;
                    strncpy(g->players[i].name, m->dpnName.lpszShortNameA, 31);
                    g->players[i].name[31] = 0;
                    g->players[i].active = 1;
                    ((unsigned char *)g->buffer)[0] = 0;
                    memcpy((char *)g->buffer + 4, &slot, 2);
                    callback(g, fn, user, bytes);
                    ++g->player_count;
                    break;
                }
            break;
        }
        case 5: {
            DPID id;
            if (bytes < 12)
            {
                g->error = 2;
                return 0;
            }
            memcpy(&id, (char *)g->buffer + 8, 4);
            for (i = 0; i < 8; ++i)
                if (g->players[i].active && g->players[i].id == id)
                {
                    unsigned short slot = (unsigned short)i;
                    g->players[i].active = 0;
                    ((unsigned char *)g->buffer)[0] = 0;
                    memcpy((char *)g->buffer + 4, &slot, 2);
                    callback(g, fn, user, bytes);
                    --g->player_count;
                    break;
                }
            break;
        }
        case 0x31:
            ((unsigned char *)g->buffer)[0] = 2;
            callback(g, fn, user, bytes);
            break;
        case 0x101:
            ((unsigned char *)g->buffer)[0] = 1;
            callback(g, fn, user, bytes);
            break;
        default:
            break;
        }
    }
}
