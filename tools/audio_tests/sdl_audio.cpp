// Deterministic mixer checks plus the production scheduler and media owners.
#include <SDL3/SDL.h>
static bool fail_output_open;
static SDL_AudioStream *test_open(SDL_AudioDeviceID id, const SDL_AudioSpec *spec, SDL_AudioStreamCallback cb, void *user)
{
    return fail_output_open ? nullptr : SDL_OpenAudioDeviceStream(id, spec, cb, user);
}
#define SDL_OpenAudioDeviceStream test_open
#include "../../src/bh_sdl_audio.cpp"
#undef SDL_OpenAudioDeviceStream
extern "C" {
#include "../../src/bh_sound_queue.h"
#include "../../src/bh_fp.h"
#include "../../src/bh_media_control.h"
}
#include <cassert>
#include <cstdio>
#include <fstream>

static SDL_AudioStream *sink;
static std::vector<float> render(int frames)
{
    SDL_ClearAudioStream(sink);
    mix(nullptr, sink, frames * 8, 0);
    std::vector<float> pcm(frames * 2);
    assert(SDL_GetAudioStreamData(sink, pcm.data(), frames * 8) == frames * 8);
    return pcm;
}
static bool approximately(float a, float b) { return std::fabs(a-b) < .0001f; }
static void quiet_output()
{
    if (output) assert(SDL_PauseAudioStreamDevice(output));
}
static unsigned long request(BHSoundQueue *q, unsigned long id, const BHVec3 *p = nullptr)
{
    auto h = bh_sound_request(q, id, 1, 1, p, nullptr);
    assert(h != BH_SOUND_INVALID); return h;
}
static void tick(BHSoundQueue *q) { assert(bh_sound_tick(q, .25f, 0, nullptr)); }
static void write_wave(const char *path)
{
    std::ofstream f(path, std::ios::binary);
    auto u16 = [&](unsigned n) { f.put(n & 255); f.put((n>>8)&255); };
    auto u32 = [&](unsigned n) { u16(n & 65535); u16(n>>16); };
    f.write("RIFF",4); u32(36+96000*2); f.write("WAVEfmt ",8); u32(16);
    u16(1); u16(1); u32(48000); u32(96000); u16(2); u16(16);
    f.write("data",4); u32(96000*2);
    for (int i=0;i<96000;++i) u16(16384);
}
int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    auto d = bh_audio_open(); assert(d && output); quiet_output();
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 48000 };
    sink = SDL_CreateAudioStream(&spec, &spec); assert(sink);
    const short pcm[] = {0,8192,16384,24576};
    BHAudioBuffer *v = nullptr, *copy = nullptr;
    assert(!bh_audio_create(d, pcm, sizeof(pcm), 1, 16, 48000, &v));
    assert(!bh_audio_pitch(v, 24000)); assert(!bh_audio_play(v,0,0,0));
    auto x = render(8);
    assert(approximately(x[2], .125f) && approximately(x[4], .25f) && approximately(x[8], .5f));
    render(1); unsigned long status; assert(!bh_audio_status(v,&status) && !status);
    assert(!bh_audio_duplicate(d,v,&copy) && copy->clip == v->clip);
    assert(!bh_audio_pitch(v,0) && v->frequency == 48000);
    assert(!bh_audio_seek(v, 4)); assert(!bh_audio_pan(v,10000));
    assert(!bh_audio_volume(v,-2000)); assert(!bh_audio_play(v,0,0,1));
    x=render(1); assert(approximately(x[0],0) && approximately(x[1],.05f));
    bh_audio_effects_gain(.5f); x=render(1); assert(approximately(x[1],.0375f));
    bh_audio_stop(v); assert(!copy->playing && copy->cursor == 0);
    bh_audio_buffer_release(v); bh_audio_buffer_release(copy); bh_audio_effects_gain(1);
    const unsigned char p8[] = {128,192};
    assert(!bh_audio_create(d,p8,2,1,8,48000,&v));
    bh_audio_play(v,0,0,1); x=render(5); assert(approximately(x[0],0) && approximately(x[2],.5f) && approximately(x[6],.5f));
    bh_audio_buffer_release(v);
    const short stereo[] = {8192,-16384};
    assert(!bh_audio_create(d,stereo,4,2,16,48000,&v)); bh_audio_play(v,0,0,1);
    x=render(1); assert(approximately(x[0],.25f) && approximately(x[1],-.5f)); bh_audio_buffer_release(v);
    puts("PASS: PCM8/16, stereo, fractional pitch, pan, dB gain, independent cursors, completion and loops");

    BHMathEnvironment env; assert(bh_math_environment_enter(&env));
    BHSoundQueue q; assert(bh_sound_queue_init(&q,d,nullptr,nullptr)); q.listener_active=1; q.listener_right.x=1;
    short constant[480] = {}; std::fill(constant, constant+480, short(16384));
    BHWave wave = {}; wave.samples=(unsigned char *)constant; wave.sample_bytes=sizeof(constant);
    wave.format_tag=1; wave.channels=1; wave.bits=16; wave.sample_rate=48000;
    auto id = bh_sound_register_wave(&q,&wave,1,2,0); assert(id != BH_SOUND_INVALID);
    auto r=q.resources[id]; r->time_scale=.125f; r->audible_range=10;
    auto a=request(&q,id), b=request(&q,id); tick(&q);
    assert(r->buffer_count==2 && r->playing[0] && r->playing[1]);
    assert(bh_sound_stop(&q,a)); tick(&q); assert(r->playing[0] && !r->playing[1]);
    q.listener_active=0; r->phase=100;
    assert(bh_sound_stop(&q,b)); assert(!r->buffers[0]->playing && !bh_sound_voice(&q,b));
    assert(!bh_sound_stop(&q,b));
    q.listener_active=1;
    BHVec3 right={20,0,0}; a=request(&q,id,&right); r->phase=0; tick(&q);
    assert(r->cached_volume[0]==-1000 && r->cached_pan[0]==10000);
    x=render(1); assert(approximately(x[0],0) && approximately(x[1],float(.5*std::pow(10.0,-.5))));
    BHVec3 velocity={-330,0,0};
    assert(bh_sound_update(&q,a,1,2,&right,&velocity));
    tick(&q); assert(r->cached_frequency[0] == 192000);
    assert(bh_sound_stop(&q,a));
    id=bh_sound_register_wave(&q,&wave,0,1,0); r=q.resources[id]; r->audible_range=10;
    a=request(&q,id); tick(&q); render(481); tick(&q); assert(!bh_sound_voice(&q,a));
    a=request(&q,id); tick(&q);
    BHVec3 distant={100,0,0}; assert(bh_sound_request(&q,id,1,1,&distant,nullptr)==BH_SOUND_INVALID);
    b=request(&q,id); assert(!bh_sound_voice(&q,a) && bh_sound_voice(&q,b)); tick(&q);
    bh_audio_effects_gain(0); x=render(1); assert(approximately(x[0],0) && approximately(x[1],0));
    assert(bh_sound_stop_all(&q) && !bh_sound_voice(&q,b));
    bh_audio_effects_gain(1);
    puts("PASS: shared loop selection, immediate last stop, inactive listener, spatial attenuation, priority, recycling and stale handles");

    write_wave("audio-fixture.wav");
    BHCdClose cd={}; BHCdNotify retained={}; retained.cd=&cd;
    BHCdAudio audio={}; audio.notify=&retained; audio.music_path="audio-fixture.wav";
    BHMediaControl media={}; media.audio=&audio;
    assert(bh_cd_audio_startup(&audio)); quiet_output();
    assert(cd.local_length==2000 && cd.device->clip->pcm.empty());
    bh_wave_set_volume(&media,0); bh_cd_set_volume(&media,.5f);
    assert(bh_cd_play_range(&media,2,2,0,1,1)); // Original packed endpoint: one second.
    x=render(50000); assert(x[0]>.24f && x[0]<.26f && approximately(x[0],x[4000]));
    assert(cd.device->playing && retained.last.dwTo==1000);
    bh_cd_stop(&media); x=render(1); assert(approximately(x[0],0));
    assert(bh_cd_play_range(&media,2,2,0,255,0)); assert(retained.last.dwTo==2000);
    render(96001); assert(!cd.device->playing);
    assert(bh_cd_play_range(&media,2,2,0,1,1)); render(10);
    auto cursor=cd.device->cursor; auto old_gain=cd.device->gain;
    bh_audio_device_changed(); bh_audio_poll(); quiet_output();
    assert(output && cd.device->cursor>=cursor && cd.device->gain==old_gain && cd.device->loop);
    // Fail the actual opening call, verify backoff, then recover the same voices.
    fail_output_open = true;
    bh_audio_device_changed(); bh_audio_poll();
    assert(!output && cd.device->playing && retry_at > SDL_GetTicks());
    fail_output_open = false; bh_audio_poll(); assert(!output);
    retry_at=0; bh_audio_poll(); quiet_output(); assert(output && cd.device->playing);
    bh_cd_stop_close(&cd); bh_cd_stop_close(&cd);
    // Mutate voice state while the real dummy-device callback is running.
    assert(SDL_ResumeAudioStreamDevice(output));
    for (int i=0; i<2000; ++i) {
        assert(!bh_audio_create(d, pcm, sizeof(pcm), 1, 16, 48000, &v));
        assert(!bh_audio_play(v,0,0,1));
        bh_audio_volume(v,-1000); bh_audio_pan(v,-5000); bh_audio_stop(v);
        bh_audio_buffer_release(v);
    }
    quiet_output();
    SDL_DestroyAudioStream(sink); sink = nullptr;
    bh_sound_queue_dispose(&q); bh_audio_release(d);
    assert(voices.empty() && !output && !device.refs);
    bh_math_environment_leave(&env);
    std::remove("audio-fixture.wav");
    puts("PASS: streamed music, retained ranges, music/effects volume isolation, stop, output reopen/retry and complete shutdown");
}
