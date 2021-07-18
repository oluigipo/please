#define SOUND_LATENCY_FPS 10

//~ Globals
internal bool32 global_audio_is_initialized = false;
internal int32 global_channels = 2;
internal int32 global_samples_per_second = 44100;
internal int32 global_latency_frame_count;

internal int32 global_frame_count_to_output;
internal int16* global_audio_buffer;
internal int32 global_audio_buffer_sample_count;

internal snd_pcm_t *global_playback_handle;

//~ Internal API
internal void
Linux_InitAudio(void)
{
    return; // TODO(ljre): Finish audio on linux.
    
    snd_pcm_hw_params_t* params;
    
    if (0 > snd_pcm_open(&global_playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0))
    {
        Platform_DebugLog("Could not initialize ALSA.\n");
        return;
    }
    
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(global_playback_handle, params);
    
    snd_pcm_hw_params_set_access(global_playback_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(global_playback_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(global_playback_handle, params, &global_samples_per_second, 0);
    snd_pcm_hw_params_set_channels(global_playback_handle, params, global_channels);
    
    snd_pcm_hw_params(global_playback_handle, params);
    
    snd_pcm_hw_params_free(params);
    
    //- Audio Buffer
    global_latency_frame_count = global_samples_per_second / global_channels * SOUND_LATENCY_FPS;
    global_audio_buffer_sample_count = global_samples_per_second * SOUND_LATENCY_FPS;
    global_audio_buffer = Platform_HeapAlloc((uintsize)global_audio_buffer_sample_count * sizeof(int16));
    
    global_audio_is_initialized = true;
}

internal void
Linux_UpdateAudio(void)
{
    if (!global_audio_is_initialized)
        return;
    
    snd_pcm_sframes_t avail, delay;
    snd_pcm_avail_delay(global_playback_handle, &avail, &delay);
    snd_pcm_avail_update(global_playback_handle);
    
    global_frame_count_to_output = avail;
    
    if (global_frame_count_to_output > global_latency_frame_count)
        global_frame_count_to_output = global_latency_frame_count;
    
    if (global_frame_count_to_output > global_audio_buffer_sample_count / global_channels)
        global_frame_count_to_output = global_audio_buffer_sample_count / global_channels;
    
    Platform_DebugLog("Avail: %li, Delay: %li, To Output: %i\n", avail, delay, global_frame_count_to_output);
}

internal void
Linux_FillAudioBuffer(void)
{
    if (global_audio_is_initialized && global_frame_count_to_output > 0)
    {
        int32 frame_count = global_frame_count_to_output;
        
        //snd_pcm_drop(global_playback_handle);
        //snd_pcm_prepare(global_playback_handle);
        snd_pcm_sframes_t ret = snd_pcm_writei(global_playback_handle, global_audio_buffer, frame_count);
        //snd_pcm_start(global_playback_handle);
    }
}

internal void
Linux_DeinitAudio(void)
{
    if (global_audio_is_initialized)
    {
        snd_pcm_drain(global_playback_handle);
        snd_pcm_close(global_playback_handle);
        
        Platform_HeapFree(global_audio_buffer);
        global_audio_buffer = NULL;
        global_audio_buffer_sample_count = 0;
    }
}

//~ API
API int16*
Platform_RequestSoundBuffer(int32* out_sample_count, int32* out_channels, int32* out_sample_rate)
{
    if (global_audio_is_initialized)
    {
        *out_sample_count = global_frame_count_to_output * global_channels;
        *out_channels = global_channels;
        *out_sample_rate = global_samples_per_second;
        return global_audio_buffer;
    }
    else
    {
        return NULL;
    }
}

API bool32
Platform_IsAudioAvailable(void)
{
    return global_audio_is_initialized;
}
