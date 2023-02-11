
#define CoCreateInstance global_proc_CoCreateInstance
#define CoInitializeEx global_proc_CoInitializeEx

typedef HRESULT WINAPI ProcCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
typedef HRESULT WINAPI ProcCoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#   define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif

#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#   define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

//~ Globals
static ProcCoCreateInstance* CoCreateInstance;
static ProcCoInitializeEx* CoInitializeEx;
static int32 global_channels = 2;
static int32 global_latency_frame_count;
static int32 global_samples_per_second = 48000;
static OS_AudioThreadProc* global_audio_thread_userproc;
static void* global_audio_thread_userdata;

static IMMDeviceEnumerator* global_audio_device_enumerator;
static IMMDevice* global_audio_device;
static IAudioClient* global_audio_client;
static IAudioRenderClient* global_audio_render_client;
static HANDLE global_audio_event;
static HANDLE global_audio_thread;

//~ Functions
static bool
Win32_LoadComLibrary_(void)
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

static DWORD WINAPI
Win32_AudioThreadProc_(void* user_data)
{
	const int32 frame_count = global_latency_frame_count;
	const int32 channels = global_channels;
	const int32 sample_rate = global_samples_per_second;
	const int32 sample_count = frame_count * channels;
	OS_AudioThreadProc* const userproc = global_audio_thread_userproc;
	void* const userdata = global_audio_thread_userdata;
	
	for (;;)
	{
		DWORD result = WaitForSingleObject(global_audio_event, INFINITE);
		if (result != WAIT_OBJECT_0)
			return 1;
		
		Trace();
		
		BYTE* buffer;
		HRESULT errcode = IAudioRenderClient_GetBuffer(global_audio_render_client, frame_count, &buffer);
		
		if (Likely(SUCCEEDED(errcode)))
		{
			userproc(userdata, (int16*)buffer, channels, sample_rate, sample_count);
			IAudioRenderClient_ReleaseBuffer(global_audio_render_client, frame_count, 0);
		}
	}
	
	return 0;
}

//~ Internal API
static bool
Win32_InitAudio(OS_AudioThreadProc* proc, void* user_data)
{
	Trace();
	
	if (!Win32_LoadComLibrary_())
		return false;
	
	HRESULT hr;
	
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&global_audio_device_enumerator);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(global_audio_device_enumerator, eRender, eConsole, &global_audio_device);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	hr = IMMDevice_Activate(global_audio_device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&global_audio_client);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	WAVEFORMATEX* wave_format;
	hr = IAudioClient_GetMixFormat(global_audio_client, &wave_format);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	const REFERENCE_TIME requested_sound_duration = 10000000;
	uint16 bytes_per_sample = sizeof(int16);
	uint16 bits_per_sample = bytes_per_sample * 8;
	uint16 block_align = (uint16)global_channels * bytes_per_sample;
	uint32 average_bytes_per_second = (uint32)block_align * (uint32)global_samples_per_second;
	
	WAVEFORMATEX new_wave_format = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = (uint16)global_channels,
		.nSamplesPerSec = (uint32)global_samples_per_second,
		.nAvgBytesPerSec = average_bytes_per_second,
		.nBlockAlign = block_align,
		.wBitsPerSample = bits_per_sample,
		.cbSize = 0,
	};
	
	hr = IAudioClient_Initialize(
		global_audio_client, AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST |
		AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
		requested_sound_duration, 0, &new_wave_format, NULL);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	hr = IAudioClient_GetService(global_audio_client, &IID_IAudioRenderClient, (void**)&global_audio_render_client);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	REFERENCE_TIME reftime;
	hr = IAudioClient_GetDevicePeriod(global_audio_client, &reftime, NULL);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	global_latency_frame_count = 2 * (int32)(reftime * (int64)(global_samples_per_second / global_channels) / (int64)10000000);
	
	OS_DebugLog("global_latency_frame_count: %i\n", global_latency_frame_count);
	
	global_audio_event = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!global_audio_event)
		goto lbl_error;
	
	hr = IAudioClient_SetEventHandle(global_audio_client, global_audio_event);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	hr = IAudioClient_Start(global_audio_client);
	if (!SUCCEEDED(hr))
		goto lbl_error;
	
	global_audio_thread_userproc = proc;
	global_audio_thread_userdata = user_data;
	global_audio_thread = CreateThread(NULL, 0, Win32_AudioThreadProc_, NULL, 0, NULL);
	if (!global_audio_thread)
		goto lbl_error;
	
	SetThreadPriority(global_audio_thread, THREAD_PRIORITY_TIME_CRITICAL);
	
	return true;
	
	// Errors
	lbl_error:
	{
		if (global_audio_event)
			CloseHandle(global_audio_event);
		if (global_audio_client)
			IAudioClient_Release(global_audio_client);
		if (global_audio_device)
			IMMDevice_Release(global_audio_device);
		if (global_audio_device_enumerator)
			IMMDeviceEnumerator_Release(global_audio_device_enumerator);
		
		global_audio_event = NULL;
		global_audio_client = NULL;
		global_audio_device = NULL;
		global_audio_device_enumerator = NULL;
		
		return false;
	}
}

static void
Win32_DeinitAudio(void)
{
	if (global_audio_thread)
	{
		TerminateThread(global_audio_thread, 0);
		CloseHandle(global_audio_thread);
		IAudioClient_Stop(global_audio_client);
		CloseHandle(global_audio_event);
		IAudioClient_Release(global_audio_client);
		IMMDevice_Release(global_audio_device);
		IMMDeviceEnumerator_Release(global_audio_device_enumerator);
	}
}
