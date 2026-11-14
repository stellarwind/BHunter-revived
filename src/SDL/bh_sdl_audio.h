#ifndef BH_SDL_AUDIO_H
#define BH_SDL_AUDIO_H
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct BHAudioDevice BHAudioDevice;
    typedef struct BHAudioBuffer BHAudioBuffer;
    BHAudioDevice *bh_audio_open(void);
    void bh_audio_retain(BHAudioDevice *);
    void bh_audio_release(BHAudioDevice *);
    int bh_audio_create(BHAudioDevice *, const void *, unsigned long, unsigned, unsigned, unsigned long,
                        BHAudioBuffer **);
    int bh_audio_duplicate(BHAudioDevice *, BHAudioBuffer *, BHAudioBuffer **);
    void bh_audio_buffer_retain(BHAudioBuffer *);
    void bh_audio_buffer_release(BHAudioBuffer *);
    int bh_audio_stop(BHAudioBuffer *);
    int bh_audio_frequency(BHAudioBuffer *, unsigned long *);
    int bh_audio_position(BHAudioBuffer *, unsigned long *, unsigned long *);
    int bh_audio_volume(BHAudioBuffer *, long);
    int bh_audio_pan(BHAudioBuffer *, long);
    int bh_audio_pitch(BHAudioBuffer *, unsigned long);
    int bh_audio_seek(BHAudioBuffer *, unsigned long);
    int bh_audio_play(BHAudioBuffer *, unsigned long, unsigned long, unsigned long);
    int bh_audio_status(BHAudioBuffer *, unsigned long *);
    BHAudioBuffer *bh_audio_load_music(const char *, unsigned long *);
    int bh_audio_music_range(BHAudioBuffer *, unsigned long, unsigned long, int);
    void bh_audio_effects_gain(float);
    float bh_audio_get_effects_gain(void);
    void bh_audio_device_changed(void);
    void bh_audio_poll(void);
#ifdef __cplusplus
}
#endif
#endif
