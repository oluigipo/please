//
// NOTE(ljre): All of this is not used anymore! We are using 'os_win32_audio.c' instead.
//

#define CoCreateInstance global_proc_CoCreateInstance
#define CoInitializeEx global_proc_CoInitializeEx

typedef HRESULT WINAPI ProcCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
typedef HRESULT WINAPI ProcCoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#	define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif

#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#	define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

//~ Globals
static ProcCoCreateInstance* CoCreateInstance;
static ProcCoInitializeEx* CoInitializeEx;
static bool global_audio_is_initialized = false;
static int32 global_channels = 2;
static int32 global_samples_per_second = 48000;
static int32 global_latency_frame_count;

static uint8* volatile global_audio_buffer_cold;
static uint8* volatile global_audio_buffer_hot;
static uint8* global_audio_buffer;
static int32 global_audio_buffer_sample_count;
static int32 volatile global_audio_elapsed_frames;
static int32 global_audio_to_override;

static IMMDeviceEnumerator* global_audio_device_enumerator;
static IMMDevice* global_audio_device;
static IAudioClient* global_audio_client;
static IAudioRenderClient* global_audio_render_client;
static HANDLE global_audio_event;
static HANDLE global_audio_thread;
static OS_RWLock global_audio_mutex;

//~ Functions
static bool
LoadComLibrary(void)
{
	HMODULE library = Win32_LoadLibrary("ole32.dll");
	
	if (!library)
		return false;
	
	CoCreateInstance = (void*)GetProcAddress(library, "CoCreateInstance");
	CoInitializeEx = (void*)GetProcAddress(library, "CoInitializeEx");
	
	if (CoCreateInstance && CoInitializeEx)
	{
		CoInitializeEx(NULL, COINIT_SPEED_OVER_MEMORY);
		return true;
	}
	
	return false;
}

//#include <avrt.h>
//#pragma comment(lib, "avrt.lib")

static DWORD WINAPI
AudioThreadProc(void* data)
{
	//DWORD task_index;
	//AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);
	
	while (true)
	{
		DWORD result = WaitForSingleObject(global_audio_event, 2000);
		if (result != WAIT_OBJECT_0)
			return 1;
		
		Trace();
		
		int32 frame_count = global_latency_frame_count;
		
		uint8* buffer;
		int32 elapsed_frames;
		OS_LockExclusive(&global_audio_mutex);
		{
			buffer = global_audio_buffer_hot;
			elapsed_frames = global_audio_elapsed_frames;
			global_audio_elapsed_frames += frame_count;
		}
		OS_UnlockExclusive(&global_audio_mutex);
		
		uintsize offset = (uintsize)((elapsed_frames * global_channels) % global_audio_buffer_sample_count);
		
		BYTE* dest = NULL;
		void* source = buffer + offset * sizeof(int16);
		
		HRESULT errcode = IAudioRenderClient_GetBuffer(global_audio_render_client, frame_count, &dest);
		(void)errcode;
		
		if (dest)
			MemoryCopy(dest, source, (uintsize)(frame_count * global_channels) * sizeof(int16));
		
		IAudioRenderClient_ReleaseBuffer(global_audio_render_client, frame_count, 0);
	}
}

//~ Internal API
static bool
Win32_InitSimpleAudio(void)
{
	Trace();
	
	if (!LoadComLibrary())
		return false;
	
	// NOTE(ljre): Error handling. At the beginning so you know what errorN means :)
	if (false)
	{
		error6: OS_HeapFree(global_audio_buffer);
		error5: CloseHandle(global_audio_event);
		error4: OS_DeinitRWLock(&global_audio_mutex);
		error3: IAudioClient_Release(global_audio_client);
		error2: IMMDevice_Release(global_audio_device);
		error1: IMMDeviceEnumerator_Release(global_audio_device_enumerator);
		error0: return false;
	}
	
	HRESULT result;
	
	//- Trying to Initialize
	result = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&global_audio_device_enumerator);
	if (result != S_OK)
		goto error0;
	
	result = IMMDeviceEnumerator_GetDefaultAudioEndpoint(global_audio_device_enumerator, eRender, eConsole, &global_audio_device);
	if (result != S_OK)
		goto error1;
	
	result = IMMDevice_Activate(global_audio_device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&global_audio_client);
	if (result != S_OK)
		goto error2;
	
	//- Setting Stuff
	WAVEFORMATEX* wave_format;
	IAudioClient_GetMixFormat(global_audio_client, &wave_format);
	
	const REFERENCE_TIME requested_sound_duration = 10000000;
	uint16 bytes_per_sample = sizeof(int16);
	uint16 bits_per_sample = bytes_per_sample * 8;
	uint16 block_align = (uint16)global_channels * bytes_per_sample;
	uint32 average_bytes_per_second = (uint32)block_align * (uint32)global_samples_per_second;
	
	WAVEFORMATEX new_wave_format = {
		WAVE_FORMAT_PCM,
		(uint16)global_channels,
		(uint32)global_samples_per_second,
		average_bytes_per_second,
		block_align,
		bits_per_sample,
		0
	};
	
	result = IAudioClient_Initialize(global_audio_client,
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
		requested_sound_duration,
		0,
		&new_wave_format,
		NULL);
	if (result != S_OK)
		goto error3;
	
#if 0
	result = IAudioClient_GetBufferSize(global_audio_client, &global_audio_buffer_sample_count);
	if (result != S_OK)
		goto error3;
	
	global_audio_buffer_sample_count *= global_channels;
#endif
	
	result = IAudioClient_GetService(global_audio_client, &IID_IAudioRenderClient, (void**)&global_audio_render_client);
	if (result != S_OK)
		goto error3;
	
	REFERENCE_TIME reftime;
	result = IAudioClient_GetDevicePeriod(global_audio_client, &reftime, NULL);
	if (result != S_OK)
		goto error3;
	
	global_latency_frame_count = (int32)(reftime * (int64)(global_samples_per_second / global_channels) / (int64)10000000);
	global_latency_frame_count *= 2;
	
	//OS_DebugLog("global_latency_frame_count: %i\n", global_latency_frame_count);
	
	OS_InitRWLock(&global_audio_mutex);
	
	global_audio_event = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!global_audio_event)
		goto error4;
	
	result = IAudioClient_SetEventHandle(global_audio_client, global_audio_event);
	if (result != S_OK)
		goto error5;
	
	result = IAudioClient_Start(global_audio_client);
	if (result != S_OK)
		goto error5;
	
	// NOTE(ljre): make sure 'global_audio_buffer_sample_count' is multiple of 'global_latency_frame_count'.
	{
		global_audio_buffer_sample_count = global_samples_per_second / global_channels;
		
		int32 remainder = global_audio_buffer_sample_count % global_latency_frame_count;
		remainder = global_latency_frame_count - remainder;
		global_audio_buffer_sample_count += remainder;
		
		global_audio_buffer_sample_count *= global_channels;
	}
	
	uintsize size = (uintsize)global_audio_buffer_sample_count * sizeof(int16);
	global_audio_buffer = OS_HeapAlloc(size * 2);
	global_audio_buffer_hot = global_audio_buffer;
	global_audio_buffer_cold = global_audio_buffer_hot + size;
	MemorySet(global_audio_buffer, 0, size * 2);
	
	global_audio_thread = CreateThread(NULL, 0, AudioThreadProc, NULL, 0, NULL);
	if (!global_audio_thread)
		goto error6;
	
	global_audio_is_initialized = true;
	return true;
}

static void
Win32_DeinitSimpleAudio(void)
{
	Trace();
	
	if (!global_audio_is_initialized)
		return;
	
	TerminateThread(global_audio_thread, 0);
	CloseHandle(global_audio_event);
	
	OS_HeapFree(global_audio_buffer);
	OS_DeinitRWLock(&global_audio_mutex);
	
	IAudioClient_Stop(global_audio_client);
	
	IAudioRenderClient_Release(global_audio_render_client);
	IAudioClient_Release(global_audio_client);
	IMMDevice_Release(global_audio_device);
	IMMDeviceEnumerator_Release(global_audio_device_enumerator);
}

//~ API
API int16*
OS_RequestSoundBuffer(int32* out_sample_count, int32* out_channels, int32* out_sample_rate, int32* out_elapsed_frames)
{
	if (global_audio_is_initialized)
	{
		*out_sample_count = global_audio_buffer_sample_count;
		*out_channels = global_channels;
		*out_sample_rate = global_samples_per_second;
		
		OS_LockExclusive(&global_audio_mutex);
		{
			*out_elapsed_frames = global_audio_to_override = global_audio_elapsed_frames;
		}
		OS_UnlockExclusive(&global_audio_mutex);
		
		MemorySet(global_audio_buffer_cold, 0, (uintsize)global_audio_buffer_sample_count * sizeof(int16));
		return (int16*)global_audio_buffer_cold;
	}
	else
	{
		return NULL;
	}
}

API void
OS_CloseSoundBuffer(int16* sound_buffer)
{
	Assert(sound_buffer == (int16*)global_audio_buffer_cold);
	
	OS_LockExclusive(&global_audio_mutex);
	{
		void* temp = global_audio_buffer_hot;
		global_audio_buffer_hot = global_audio_buffer_cold;
		global_audio_buffer_cold = temp;
		//global_audio_elapsed_frames = 0;
		global_audio_elapsed_frames -= global_audio_to_override;
	}
	OS_UnlockExclusive(&global_audio_mutex);
}
