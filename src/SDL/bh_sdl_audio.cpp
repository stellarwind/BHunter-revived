#include "bh_sdl_audio.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>
#include <new>

#define STB_VORBIS_NO_PUSHDATA_API
#include "../../third_party/stb/stb_vorbis.c"
#undef STB_VORBIS_NO_PUSHDATA_API

/* All clip/voice state is protected by mixer_mutex. Never destroy or lock an
   SDL stream while holding it: SDL invokes the mixer under the stream lock. */
struct BHAudioDevice
{
    unsigned refs = 0;
};
struct MusicFile
{
    static constexpr size_t block_frames = 4096;
    SDL_IOStream *io = nullptr;
    stb_vorbis *vorbis = nullptr;
    Uint64 offset = 0, cached = ~Uint64(0);
    unsigned bits = 0, count = 0;
    size_t next_frame = 0, previous_frame = SIZE_MAX;
    unsigned char raw[block_frames * 4];
    float decoded[block_frames * 2], first[2], previous[2];
    ~MusicFile()
    {
        if (vorbis)
            stb_vorbis_close(vorbis);
        if (io)
            SDL_CloseIO(io);
    }

    bool refill_vorbis(size_t frame, size_t frames, unsigned channels)
    {
        // Interpolation can revisit the last sample of the old block after a refill.
        previous_frame = SIZE_MAX;
        if (count && frame == cached + count)
        {
            previous_frame = frame - 1;
            std::copy_n(decoded + (count - 1) * channels, channels, previous);
        }
        cached = frame;
        count = unsigned(std::min(block_frames, frames - frame));
        int got = 0;
        if (frame == next_frame || stb_vorbis_seek64(vorbis, frame))
            got = stb_vorbis_get_samples_float_interleaved(vorbis, int(channels), decoded, int(count * channels));
        // Keep failed or short reads bounded to one attempt per block.
        if (got != int(count))
        {
            std::fill(decoded + std::max(got, 0) * channels, decoded + count * channels, 0.0f);
            next_frame = SIZE_MAX;
            return false;
        }
        next_frame = frame + count;
        return true;
    }
};
struct AudioClip
{
    std::vector<float> pcm;
    unsigned rate, channels, stride;
    size_t frames;
    std::unique_ptr<MusicFile> file;
    float sample(size_t frame, unsigned channel)
    {
        if (!file)
            return pcm[frame * channels + channel];
        auto &f = *file;
        if (frame < f.cached || frame >= f.cached + f.count)
        {
            if (f.vorbis)
            {
                // The loop endpoint can interpolate against the first frame of the track.
                if (!frame)
                    return f.first[channel];
                if (frame == f.previous_frame)
                    return f.previous[channel];
                f.refill_vorbis(frame, frames, channels);
                return f.decoded[channel];
            }
            f.cached = frame;
            f.count = unsigned(std::min(MusicFile::block_frames, frames - frame));
            size_t bytes = f.count * stride;
            if (SDL_SeekIO(f.io, Sint64(f.offset + frame * stride), SDL_IO_SEEK_SET) < 0 ||
                SDL_ReadIO(f.io, f.raw, bytes) != bytes)
            {
                f.count = 0;
                return 0;
            }
            for (unsigned i = 0; i < f.count * channels; ++i)
            {
                if (f.bits == 8)
                    f.decoded[i] = (int(f.raw[i]) - 128) / 128.0f;
                else
                {
                    int n = f.raw[i * 2] | (unsigned(f.raw[i * 2 + 1]) << 8);
                    if (n >= 32768)
                        n -= 65536;
                    f.decoded[i] = n / 32768.0f;
                }
            }
        }
        return f.decoded[(frame - f.cached) * channels + channel];
    }
};
struct BHAudioBuffer
{
    std::shared_ptr<AudioClip> clip;
    unsigned refs = 1;
    double cursor = 0, begin = 0, end = 0;
    unsigned frequency;
    float gain = 1, left = 1, right = 1;
    bool playing = false, loop = false, music = false;
};
namespace
{
BHAudioDevice device;
std::mutex mixer_mutex;
std::vector<BHAudioBuffer *> voices;
SDL_AudioStream *output;
float effects_gain = 1;
bool reopen_output;
Uint64 retry_at;
constexpr unsigned mix_rate = 48000;
void SDLCALL mix(void *, SDL_AudioStream *stream, int additional, int)
{
    float pcm[512 * 2];
    while (additional > 0)
    {
        int frames = std::min(512, (additional + 7) / 8);
        std::fill(pcm, pcm + frames * 2, 0.0f);
        {
            std::lock_guard<std::mutex> guard(mixer_mutex);
            for (auto *voice : voices)
            {
                if (!voice->playing)
                    continue;
                auto &clip = *voice->clip;
                const double step = double(voice->frequency) / mix_rate;
                float gain = voice->gain * (voice->music ? 1 : effects_gain);
                for (int i = 0; i < frames; ++i)
                {
                    if (voice->cursor >= voice->end)
                    {
                        if (!voice->loop)
                        {
                            voice->playing = false;
                            break;
                        }
                        voice->cursor =
                            voice->begin + std::fmod(voice->cursor - voice->begin, voice->end - voice->begin);
                    }
                    size_t sample_index = size_t(voice->cursor), next_sample_index = sample_index + 1;
                    if (double(next_sample_index) >= voice->end)
                        next_sample_index = voice->loop ? size_t(voice->begin) : sample_index;
                    float fraction = float(voice->cursor - sample_index);
                    for (unsigned output_channel = 0; output_channel < 2; ++output_channel)
                    {
                        unsigned channel = clip.channels == 1 ? 0 : output_channel;
                        float current_sample = clip.sample(sample_index, channel);
                        float next_sample = clip.sample(next_sample_index, channel);
                        pcm[i * 2 + output_channel] += (current_sample + (next_sample - current_sample) * fraction) *
                                                       gain * (output_channel ? voice->right : voice->left);
                    }
                    voice->cursor += step;
                }
            }
        }
        for (int i = 0; i < frames * 2; ++i)
            pcm[i] = std::clamp(pcm[i], -1.0f, 1.0f);
        if (!SDL_PutAudioStreamData(stream, pcm, frames * 8))
            return;
        additional -= frames * 8;
    }
}
BHAudioBuffer *publish(std::shared_ptr<AudioClip> clip, bool music = false)
{
    auto v = std::make_unique<BHAudioBuffer>();
    v->clip = std::move(clip);
    v->frequency = v->clip->rate;
    v->end = double(v->clip->frames);
    v->music = music;
    std::lock_guard<std::mutex> guard(mixer_mutex);
    voices.push_back(v.get());
    return v.release();
}
BHAudioBuffer *publish_music(const std::shared_ptr<AudioClip> &clip, unsigned long *ms)
{
    auto v = publish(clip, true);
    *ms = static_cast<unsigned long>(Uint64(clip->frames) * 1000 / clip->rate);
    return v;
}
} // namespace
extern "C"
{
    void bh_audio_poll(void)
    {
        if (!device.refs)
            return;
        if (reopen_output)
        {
            SDL_DestroyAudioStream(output);
            output = nullptr;
            reopen_output = false;
            retry_at = 0;
        }
        if (!output && SDL_GetTicks() >= retry_at)
        {
            SDL_AudioSpec spec = {SDL_AUDIO_F32, 2, int(mix_rate)};
            output = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, mix, nullptr);
            if (output && !SDL_ResumeAudioStreamDevice(output))
            {
                SDL_DestroyAudioStream(output);
                output = nullptr;
            }
            retry_at = SDL_GetTicks() + 1000;
        }
    }
    void bh_audio_device_changed(void)
    {
        reopen_output = true;
    }
    BHAudioDevice *bh_audio_open(void)
    {
        if (!device.refs && !SDL_InitSubSystem(SDL_INIT_AUDIO))
            return nullptr;
        ++device.refs;
        bh_audio_poll();
        return &device;
    }
    void bh_audio_retain(BHAudioDevice *d)
    {
        if (d)
            ++d->refs;
    }
    void bh_audio_release(BHAudioDevice *d)
    {
        if (!d || !d->refs || --d->refs)
            return;
        SDL_DestroyAudioStream(output);
        output = nullptr;
        reopen_output = false;
        retry_at = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    int bh_audio_create(BHAudioDevice *d, const void *data, unsigned long bytes, unsigned channels, unsigned bits,
                        unsigned long rate, BHAudioBuffer **out)
    {
        if (out)
            *out = nullptr;
        unsigned stride = channels * (bits / 8);
        if (!d || !out || !data || !bytes || (channels != 1 && channels != 2) || (bits != 8 && bits != 16) ||
            rate < 100 || rate > 192000 || bytes % stride)
            return -1;
        try
        {
            auto c = std::make_shared<AudioClip>();
            c->rate = rate;
            c->channels = channels;
            c->stride = stride;
            c->frames = bytes / stride;
            c->pcm.resize(bytes / (bits / 8));
            auto p = static_cast<const unsigned char *>(data);
            for (size_t i = 0; i < c->pcm.size(); ++i)
            {
                if (bits == 8)
                    c->pcm[i] = (int(p[i]) - 128) / 128.0f;
                else
                {
                    int n = p[i * 2] | (unsigned(p[i * 2 + 1]) << 8);
                    if (n >= 32768)
                        n -= 65536;
                    c->pcm[i] = n / 32768.0f;
                }
            }
            *out = publish(c);
            return 0;
        }
        catch (...)
        {
            return -1;
        }
    }
    int bh_audio_duplicate(BHAudioDevice *d, BHAudioBuffer *v, BHAudioBuffer **out)
    {
        if (!d || !v || !out)
            return -1;
        *out = nullptr;
        try
        {
            *out = publish(v->clip);
            return 0;
        }
        catch (...)
        {
            return -1;
        }
    }
    void bh_audio_buffer_retain(BHAudioBuffer *v)
    {
        if (v)
        {
            std::lock_guard<std::mutex> guard(mixer_mutex);
            ++v->refs;
        }
    }
    void bh_audio_buffer_release(BHAudioBuffer *v)
    {
        if (!v)
            return;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        if (--v->refs)
            return;
        voices.erase(std::remove(voices.begin(), voices.end(), v), voices.end());
        delete v;
    }
    int bh_audio_stop(BHAudioBuffer *v)
    {
        if (!v)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        v->playing = false;
        return 0;
    }
    int bh_audio_frequency(BHAudioBuffer *v, unsigned long *hz)
    {
        if (!v || !hz)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        *hz = v->frequency;
        return 0;
    }
    int bh_audio_position(BHAudioBuffer *v, unsigned long *play, unsigned long *write)
    {
        if (!v || !play || !write)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        // Completed buffers report their last frame, keeping priority division defined.
        *play = static_cast<unsigned long>(std::min(v->cursor, v->end - 1)) * v->clip->stride;
        *write = *play;
        return 0;
    }
    int bh_audio_volume(BHAudioBuffer *v, long db)
    {
        if (!v || db < -10000 || db > 0)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        v->gain = db == -10000 ? 0 : float(std::pow(10.0, db / 2000.0));
        return 0;
    }
    int bh_audio_pan(BHAudioBuffer *v, long pan)
    {
        if (!v || pan < -10000 || pan > 10000)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        v->left = pan > 0 ? (pan == 10000 ? 0 : float(std::pow(10.0, -pan / 2000.0))) : 1;
        v->right = pan < 0 ? (pan == -10000 ? 0 : float(std::pow(10.0, pan / 2000.0))) : 1;
        return 0;
    }
    int bh_audio_pitch(BHAudioBuffer *v, unsigned long hz)
    {
        if (!v || (hz && hz < 100) || hz > 200000)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        v->frequency = hz ? hz : v->clip->rate;
        return 0;
    }
    int bh_audio_seek(BHAudioBuffer *v, unsigned long bytes)
    {
        if (!v || bytes >= v->clip->frames * v->clip->stride)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        v->cursor = bytes / v->clip->stride;
        return 0;
    }
    int bh_audio_play(BHAudioBuffer *v, unsigned long, unsigned long, unsigned long flags)
    {
        if (!v)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        v->loop = (flags & 1) != 0;
        v->playing = true;
        return 0;
    }
    int bh_audio_status(BHAudioBuffer *v, unsigned long *status)
    {
        if (!v || !status)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        *status = v->playing ? 1 : 0;
        return 0;
    }
    BHAudioBuffer *bh_audio_load_music(const char *path, unsigned long *ms)
    {
        if (!path || !ms)
            return nullptr;
        // Keep long CD tracks on disk. The callback uses a fixed 4096-frame
        // decode cache, so a twenty-minute track does not consume hundreds of MB.
        try
        {
            auto c = std::make_shared<AudioClip>();
            c->file = std::make_unique<MusicFile>();
            auto &f = *c->file;
            f.io = SDL_IOFromFile(path, "rb");
            if (!f.io)
                return nullptr;
            unsigned char header[12], fmt[16];
            if (SDL_ReadIO(f.io, header, 12) != 12)
                return nullptr;
            if (!std::memcmp(header, "OggS", 4))
            {
                SDL_CloseIO(f.io);
                f.io = nullptr;
                f.vorbis = stb_vorbis_open_filename(path, nullptr, nullptr);
                if (!f.vorbis)
                    return nullptr;
                auto info = stb_vorbis_get_info(f.vorbis);
                auto frames = stb_vorbis_stream_length_in_samples64(f.vorbis);
                if ((info.channels != 1 && info.channels != 2) || info.sample_rate < 100 ||
                    info.sample_rate > 192000 || frames <= 0 || Uint64(frames) > SIZE_MAX ||
                    Uint64(frames) > std::numeric_limits<Uint64>::max() / 1000 ||
                    Uint64(frames) * 1000 / info.sample_rate > std::numeric_limits<unsigned long>::max())
                    return nullptr;
                c->channels = unsigned(info.channels);
                c->rate = info.sample_rate;
                c->stride = c->channels * sizeof(float);
                c->frames = size_t(frames);
                if (!f.refill_vorbis(0, c->frames, c->channels))
                    return nullptr;
                std::copy_n(f.decoded, c->channels, f.first);
                return publish_music(c, ms);
            }
            if (std::memcmp(header, "RIFF", 4) || std::memcmp(header + 8, "WAVE", 4))
                return nullptr;
            auto u32 = [](const unsigned char *p) {
                return Uint32(p[0]) | (Uint32(p[1]) << 8) | (Uint32(p[2]) << 16) | (Uint32(p[3]) << 24);
            };
            Uint64 extent = Uint64(u32(header + 4)) + 8;
            Sint64 size = SDL_GetIOSize(f.io);
            if (size < 12 || extent > Uint64(size))
                return nullptr;
            bool format = false, data = false;
            Uint32 sample_bytes = 0;
            for (Uint64 at = 12; at + 8 <= extent;)
            {
                unsigned char chunk[8];
                if (SDL_SeekIO(f.io, at, SDL_IO_SEEK_SET) < 0 || SDL_ReadIO(f.io, chunk, 8) != 8)
                    return nullptr;
                Uint32 length = u32(chunk + 4);
                if (Uint64(length) > extent - at - 8)
                    return nullptr;
                if (!format && !std::memcmp(chunk, "fmt ", 4))
                {
                    if (length < 16 || SDL_ReadIO(f.io, fmt, 16) != 16 || fmt[0] != 1 || fmt[1] != 0)
                        return nullptr;
                    c->channels = fmt[2] | (unsigned(fmt[3]) << 8);
                    c->rate = u32(fmt + 4);
                    f.bits = fmt[14] | (unsigned(fmt[15]) << 8);
                    c->stride = c->channels * (f.bits / 8);
                    if ((c->channels != 1 && c->channels != 2) || (f.bits != 8 && f.bits != 16) || c->rate < 100 ||
                        c->rate > 192000 || (fmt[12] | (unsigned(fmt[13]) << 8)) != c->stride)
                        return nullptr;
                    format = true;
                }
                else if (!data && !std::memcmp(chunk, "data", 4))
                {
                    f.offset = at + 8;
                    sample_bytes = length;
                    data = true;
                }
                at += 8 + Uint64(length) + (length & 1);
            }
            if (!format || !data || !sample_bytes || sample_bytes % c->stride)
                return nullptr;
            c->frames = sample_bytes / c->stride;
            return publish_music(c, ms);
        }
        catch (...)
        {
            return nullptr;
        }
    }

    int bh_audio_music_range(BHAudioBuffer *v, unsigned long from, unsigned long to, int loop)
    {
        if (!v)
            return -1;
        std::lock_guard<std::mutex> guard(mixer_mutex);
        double begin = double(from) * v->clip->rate / 1000;
        double end = std::min(double(to) * v->clip->rate / 1000, double(v->clip->frames));
        if (begin >= end)
            return -1;
        v->begin = begin;
        v->end = end;
        v->cursor = begin;
        v->loop = loop != 0;
        v->playing = true;
        return 0;
    }
    void bh_audio_effects_gain(float gain)
    {
        std::lock_guard<std::mutex> guard(mixer_mutex);
        effects_gain = std::isfinite(gain) ? std::clamp(gain, 0.0f, 1.0f) : 0;
    }
    float bh_audio_get_effects_gain(void)
    {
        std::lock_guard<std::mutex> guard(mixer_mutex);
        return effects_gain;
    }
}
