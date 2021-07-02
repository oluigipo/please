#define SOUND_LATENCY_FPS 10
#define REFTIMES_PER_SEC 10000000

#define CoCreateInstance global_proc_CoCreateInstance
#define CoInitializeEx global_proc_CoInitializeEx

typedef HRESULT WINAPI ProcCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
typedef HRESULT WINAPI ProcCoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

//~ Globals
internal ProcCoCreateInstance* CoCreateInstance;
internal ProcCoInitializeEx* CoInitializeEx;
internal bool32 global_audio_is_initialized = false;
internal uint32 global_channels = 2;
internal uint32 global_samples_per_second = 44100;
internal uint32 global_latency_frame_count;
internal uint32 global_buffer_frame_count;
internal REFERENCE_TIME global_sound_buffer_duration;

internal uint32 global_frame_count_to_output;
internal int16* global_audio_buffer;
internal uint32 global_audio_buffer_sample_count;

internal IMMDeviceEnumerator* global_audio_device_enumerator;
internal IMMDevice* global_audio_device;
internal IAudioClient* global_audio_client;
internal IAudioRenderClient* global_audio_render_client;

//~ Functions
internal bool32
LoadComLibrary(void)
{
	HMODULE library = LoadLibraryA("ole32.dll");
	
	if (!library)
		return false;
	
	Platform_DebugLog("Loaded Library: ole32.dll\n");
	
	CoCreateInstance = (void*)GetProcAddress(library, "CoCreateInstance");
	CoInitializeEx = (void*)GetProcAddress(library, "CoInitializeEx");
	
	if (CoCreateInstance && CoInitializeEx)
	{
		CoInitializeEx(NULL, COINIT_SPEED_OVER_MEMORY);
		return true;
	}
	
	return false;
}

//~ Internal API
internal void
Win32_InitAudio(void)
{
	if (!LoadComLibrary())
		return;
	
	HRESULT result;
	global_latency_frame_count = (global_samples_per_second / 2) / SOUND_LATENCY_FPS;
	
	//- Trying to Initialize
	result = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&global_audio_device_enumerator);
	if (result != S_OK)
		return;
	
	result = IMMDeviceEnumerator_GetDefaultAudioEndpoint(global_audio_device_enumerator,
														 eRender, eConsole, &global_audio_device);
	if (result != S_OK)
	{
		IMMDeviceEnumerator_Release(global_audio_device_enumerator);
		return;
	}
	
	result = IMMDevice_Activate(global_audio_device, &IID_IAudioClient, CLSCTX_ALL,
								NULL, (void**)&global_audio_client);
	if (result != S_OK)
	{
		IMMDevice_Release(global_audio_device);
		IMMDeviceEnumerator_Release(global_audio_device_enumerator);
		return;
	}
	
	//- Setting Stuff
	WAVEFORMATEX* wave_format;
	IAudioClient_GetMixFormat(global_audio_client, &wave_format);
	
	const REFERENCE_TIME requested_sound_duration = REFTIMES_PER_SEC * 2;
	uint16 bytes_per_sample = sizeof(int16);
	uint16 bits_per_sample = bytes_per_sample * 8;
	uint16 block_align = (uint16)global_channels * bytes_per_sample;
	uint32 average_bytes_per_second = (uint32)block_align * global_samples_per_second;
	
	WAVEFORMATEX new_wave_format = {
		WAVE_FORMAT_PCM,
		(uint16)global_channels,
		global_samples_per_second,
		average_bytes_per_second,
		block_align,
		bits_per_sample,
		0
	};
	
	result = IAudioClient_Initialize(global_audio_client,
									 AUDCLNT_SHAREMODE_SHARED,
									 AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
									 requested_sound_duration,
									 0,
									 &new_wave_format,
									 NULL);
	if (result != S_OK)
	{
		IAudioClient_Release(global_audio_client);
		IMMDevice_Release(global_audio_device);
		IMMDeviceEnumerator_Release(global_audio_device_enumerator);
		return;
	}
	
	result = IAudioClient_GetService(global_audio_client,
									 &IID_IAudioRenderClient,
									 (void**)&global_audio_render_client);
	if (result != S_OK)
	{
		IAudioClient_Release(global_audio_client);
		IMMDevice_Release(global_audio_device);
		IMMDeviceEnumerator_Release(global_audio_device_enumerator);
		return;
	}
	
	IAudioClient_GetBufferSize(global_audio_client, &global_buffer_frame_count);
	
	global_sound_buffer_duration = (REFERENCE_TIME)((float64)REFTIMES_PER_SEC *
													global_buffer_frame_count / global_samples_per_second);
	
	global_audio_buffer_sample_count = global_samples_per_second * 2;
	global_audio_buffer = Platform_HeapAlloc(global_audio_buffer_sample_count * sizeof(int16));
	
	IAudioClient_Start(global_audio_client);
	
	global_audio_is_initialized = true;
}

internal void
Win32_UpdateAudio(void)
{
	if (!global_audio_is_initialized)
		return;
	
	global_frame_count_to_output = 0;
	
	uint32 padding;
	if (SUCCEEDED(IAudioClient_GetCurrentPadding(global_audio_client, &padding)))
	{
		global_frame_count_to_output = (uint32)(global_latency_frame_count - padding);
		
		if (global_frame_count_to_output > global_latency_frame_count)
			global_frame_count_to_output = global_latency_frame_count;
		
		if (global_frame_count_to_output > global_audio_buffer_sample_count / 2)
			global_frame_count_to_output = global_audio_buffer_sample_count / 2;
	}
	
	memset(global_audio_buffer, 0, global_audio_buffer_sample_count * sizeof(int16));
}

internal void
Win32_FillAudioBuffer(void)
{
	if (!global_audio_is_initialized)
		return;
	
	if (global_frame_count_to_output > 0)
	{
		BYTE* data = NULL;
		IAudioRenderClient_GetBuffer(global_audio_render_client, global_frame_count_to_output, &data);
		
		if (data)
		{
			memcpy(data, global_audio_buffer, global_frame_count_to_output * global_channels * sizeof(int16));
		}
		
		IAudioRenderClient_ReleaseBuffer(global_audio_render_client, global_frame_count_to_output, 0);
	}
}

internal void
Win32_DeinitAudio(void)
{
	if (!global_audio_is_initialized)
		return;
	
	IAudioClient_Stop(global_audio_client);
	
	IAudioRenderClient_Release(global_audio_render_client);
	IAudioClient_Release(global_audio_client);
	IMMDevice_Release(global_audio_device);
	IMMDeviceEnumerator_Release(global_audio_device_enumerator);
}

//~ API
API int16*
Platform_RequestSoundBuffer(uint32* out_sample_count, uint32* out_channels, uint32* out_sample_rate)
{
	if (global_frame_count_to_output > 0)
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
