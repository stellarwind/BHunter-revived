#include "bh_cd_audio.h"
#include <string.h>
#include <stddef.h>
typedef char auxiliary_caps_layout[sizeof(AUXCAPSA) == 0x30 ? 1 : -1];
typedef char wave_caps_layout[sizeof(WAVEOUTCAPSA) == 0x34 ? 1 : -1];
int __cdecl bh_cd_audio_startup(BHCdAudio *audio, DWORD callback)
{
    MCI_OPEN_PARMSA open;
    MCI_SET_PARMS set;
    AUXCAPSA aux;
    WAVEOUTCAPSA wave;
    UINT count;
    BHCdClose *cd;
    if (!audio || !audio->notify || !(cd = audio->notify->cd) || cd->device)
        return 0;
    audio->notify->callback = callback;
    /* Prefer a mixed-mode CD with the original audio range on track two.
       Enumerate drives so an empty first drive does not hide the game disc. */
    {
        DWORD drives = GetLogicalDrives();
        char drive[] = "A:\\";
        char element[] = "A:", label[64];
        MCI_STATUS_PARMS status;
        for (count = 0; count < 26; ++count)
        {
            drive[0] = (char)('A' + count);
            element[0] = drive[0];
            if (!(drives & (1UL << count)) || GetDriveTypeA(drive) != DRIVE_CDROM)
                continue;
            if (!GetVolumeInformationA(drive, label, sizeof(label), NULL, NULL, NULL, NULL, 0) ||
                lstrcmpiA(label, "BHUNTER"))
                continue;
            memset(&open, 0, sizeof(open));
            open.lpstrDeviceType = "cdaudio";
            open.lpstrElementName = element;
            ++audio->opens;
            audio->open_result = mciSendCommandA(0, MCI_OPEN,
                MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE, (DWORD)&open);
            if (audio->open_result)
                continue;
            cd->device = open.wDeviceID;
            memset(&set, 0, sizeof(set));
            set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
            ++audio->sets;
            audio->set_result = mciSendCommandA(cd->device, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
            memset(&status, 0, sizeof(status));
            status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
            status.dwTrack = 2;
            if (!audio->set_result &&
                !mciSendCommandA(cd->device, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&status) &&
                status.dwReturn == MCI_CDA_TRACK_AUDIO)
            {
                status.dwItem = MCI_STATUS_LENGTH;
                if (!mciSendCommandA(cd->device, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&status) &&
                    status.dwReturn >= 1310000UL)
                    break;
            }
            mciSendCommandA(cd->device, MCI_CLOSE, MCI_WAIT, 0);
            cd->device = 0;
        }
    }
    cd->local_file = 0;
    memset(&set, 0, sizeof(set));
    if (cd->device)
        set.dwTimeFormat = MCI_FORMAT_TMSF;
    else
    {
        MCI_STATUS_PARMS status;
        memset(&open, 0, sizeof(open));
        open.lpstrDeviceType = "mpegvideo";
        open.lpstrElementName = audio->music_path;
        if (!audio->music_path || GetFileAttributesA(audio->music_path) == 0xffffffffUL)
            return 0;
        ++audio->opens;
        audio->open_result = mciSendCommandA(0, MCI_OPEN,
            MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD)&open);
        if (audio->open_result)
            return 0;
        cd->device = open.wDeviceID;
        cd->local_file = 1;
        cd->local_volume = 1.0f;
        set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
        ++audio->sets;
        audio->set_result = mciSendCommandA(cd->device, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
        if (audio->set_result)
        {
            bh_cd_stop_close(cd);
            return 0;
        }
        memset(&status, 0, sizeof(status));
        status.dwItem = MCI_STATUS_LENGTH;
        if (mciSendCommandA(cd->device, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&status) || !status.dwReturn)
        {
            bh_cd_stop_close(cd);
            return 0;
        }
        cd->local_length = status.dwReturn;
    }
    ++audio->sets;
    audio->set_result = mciSendCommandA(cd->device, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
    if (audio->set_result)
    {
        bh_cd_stop_close(cd);
        return 0;
    }
    cd->last_track = -1;
    count = cd->local_file ? 0 : auxGetNumDevs();
    while (count && cd->last_track == -1)
    {
        --count;
        memset(&aux, 0, sizeof(aux));
        ++audio->auxiliary_caps;
        audio->auxiliary_result = auxGetDevCapsA(count, &aux, sizeof(aux));
        if (audio->auxiliary_result == MMSYSERR_NOERROR && aux.wTechnology == AUXCAPS_CDAUDIO)
            cd->last_track = (long)count;
    }
    /* DirectSoundCreate(NULL) uses the preferred output. On a multi-output
       PC the original reverse scan can select a different device, so its
       pause/volume calls never reach the playing effects. Prefer that same
       output; retain the original scan on systems without this mapper query.
       DRVM_MAPPER_PREFERRED_GET is defined in the newer MMDDK header. */
    if (audio->wave_device == (HWAVEOUT)-1)
    {
        DWORD preferred = 0xffffffffUL, flags = 0;
        if (waveOutMessage((HWAVEOUT)WAVE_MAPPER, 0x2015,
                (DWORD)&preferred, (DWORD)&flags) == MMSYSERR_NOERROR &&
            preferred < waveOutGetNumDevs())
        {
            memset(&wave, 0, sizeof(wave));
            ++audio->wave_caps;
            audio->wave_result = waveOutGetDevCapsA(preferred, &wave, sizeof(wave));
            if (audio->wave_result == MMSYSERR_NOERROR && (wave.dwSupport & WAVECAPS_VOLUME))
                audio->wave_device = (HWAVEOUT)preferred;
        }
    }
    count = waveOutGetNumDevs();
    while (count && audio->wave_device == (HWAVEOUT)-1)
    {
        --count;
        memset(&wave, 0, sizeof(wave));
        ++audio->wave_caps;
        audio->wave_result = waveOutGetDevCapsA(count, &wave, sizeof(wave));
        if (audio->wave_result == MMSYSERR_NOERROR && (wave.dwSupport & WAVECAPS_VOLUME))
            audio->wave_device = (HWAVEOUT)count;
    }
    return 1;
}
double __cdecl bh_cd_audio_volume(BHCdAudio *audio)
{
    DWORD value = 0;
    BHCdClose *cd;
    if (!audio || !audio->notify || !(cd = audio->notify->cd))
        return 1.0;
    if (cd->local_file)
        return cd->local_volume;
    if (cd->last_track == -1)
        return 1.0;
    ++audio->auxiliary_reads;
    audio->auxiliary_result = auxGetVolume((UINT)cd->last_track, &value);
    if (audio->auxiliary_result != MMSYSERR_NOERROR)
        return 1.0;
    return (double)(value & 65535UL) * (1.0 / 65535.0);
}
double __cdecl bh_wave_audio_volume(BHCdAudio *audio)
{
    DWORD value = 0;
    if (!audio || audio->wave_device == (HWAVEOUT)-1)
        return 1.0;
    ++audio->wave_reads;
    audio->wave_result = waveOutGetVolume(audio->wave_device, &value);
    if (audio->wave_result != MMSYSERR_NOERROR)
        return 1.0;
    return (double)(value & 65535UL) * (1.0 / 65535.0);
}
