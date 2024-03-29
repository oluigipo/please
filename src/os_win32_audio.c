
#define CoCreateInstance g_proc_CoCreateInstance
#define CoInitializeEx g_proc_CoInitializeEx
#define CoTaskMemFree g_proc_CoTaskMemFree
#define PropVariantClear g_proc_PropVariantClear

typedef HRESULT WINAPI ProcCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
typedef HRESULT WINAPI ProcCoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
typedef void WINAPI ProcCoTaskMemFree(LPVOID pv);
typedef HRESULT WINAPI ProcPropVariantClear(PROPVARIANT* pvar);

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#	define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif

#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#	define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

struct Win32_AudioDevice
{
	IMMDevice* immdevice;
	LPWSTR dev_id;
	uint32 id;
	
	uint8 interface_name[64];
	uint8 description[64];
	uint8 name[64];
}
typedef Win32_AudioDevice;

//~ Globals
static ProcCoCreateInstance* CoCreateInstance;
static ProcCoInitializeEx* CoInitializeEx;
static ProcCoTaskMemFree* CoTaskMemFree;
static ProcPropVariantClear* PropVariantClear;

struct
{
	IMMDeviceEnumerator* device_enumerator;
	
	// NOTE(ljre): If the device we're outputting sound to is removed, this boolean is set. It means that we
	//             should output to the default device provided by IMMDeviceEnumerator_GetDefaultAudioEndpoint.
	//
	//             We probably need a way to tell if the device we were outputting to is back, and so set this
	//             to false.
	bool should_try_to_keep_on_default_endpoint;
	
	uint32 device_id_counter; // NOTE(ljre): Generative counter for device's IDs.
	
	int32 active_device_index;
	int32 device_count;
	Win32_AudioDevice devices[32];
	OS_AudioDeviceInfo devices_info[32];
	
	OS_AudioThreadProc* thread_proc;
	void* thread_userdata;
	HANDLE thread;
	
	OS_RWLock client_lock;
	
	// NOTE(ljre): All of these will change if the output device has changed. The audio thread might be sad for
	//             a bit because the event it was waiting on was suddenly closed, but then it should try again
	//             with the new event handle set by the main thread.
	
	HANDLE volatile event;
	int32 volatile sample_rate;
	int32 volatile channels;
	int32 volatile frame_pull_rate;
	IAudioClient* volatile client;
	IAudioRenderClient* volatile render_client;
}
static g_audio;

//~ Functions
static bool
Win32_LoadComLibrary_(void)
{
	HMODULE library = Win32_LoadLibrary("ole32.dll");
	
	if (!library)
		return false;
	
	CoCreateInstance = (void*)GetProcAddress(library, "CoCreateInstance");
	CoInitializeEx = (void*)GetProcAddress(library, "CoInitializeEx");
	CoTaskMemFree = (void*)GetProcAddress(library, "CoTaskMemFree");
	PropVariantClear = (void*)GetProcAddress(library, "PropVariantClear");
	
	if (CoCreateInstance && CoInitializeEx && CoTaskMemFree && PropVariantClear)
	{
		CoInitializeEx(NULL, COINIT_SPEED_OVER_MEMORY);
		return true;
	}
	
	return false;
}

static DWORD WINAPI
Win32_AudioThreadProc_(void* user_data)
{
	OS_AudioThreadProc* const userproc = g_audio.thread_proc;
	void* const userdata = g_audio.thread_userdata;
	
	for (;;)
	{
		DWORD result = WaitForSingleObject(g_audio.event, 1000);
		if (result != WAIT_OBJECT_0)
			continue;
		
		Trace();
		
		OS_LockExclusive(&g_audio.client_lock);
		
		const int32 frame_count = g_audio.frame_pull_rate;
		const int32 channels = g_audio.channels;
		const int32 sample_rate = g_audio.sample_rate;
		const int32 sample_count = frame_count * channels;
		IAudioRenderClient* render_client = g_audio.render_client;
		
		BYTE* buffer;
		HRESULT hr = IAudioRenderClient_GetBuffer(render_client, frame_count, &buffer);
		
		if (Likely(SUCCEEDED(hr)))
		{
			userproc(userdata, (int16*)buffer, channels, sample_rate, sample_count);
			IAudioRenderClient_ReleaseBuffer(render_client, frame_count, 0);
		}
		
		OS_UnlockExclusive(&g_audio.client_lock);
	}
	
	return 0;
}

static bool
Win32_AreSameImmDevices_(LPCWSTR id1, LPCWSTR id2)
{
	while (*id1 == *id2)
	{
		if (!*id1)
			return true;
		
		++id1;
		++id2;
	}
	
	return false;
}

static bool
Win32_EnumerateAudioEndpoints_(void)
{
	Trace();
	bool result = true;
	HRESULT hr;
	
	//- Check if the devices we already know are still available
	for (int32 i = 0; i < g_audio.device_count; ++i)
	{
		IMMDevice* immdevice = g_audio.devices[i].immdevice;
		LPWSTR dev_id = g_audio.devices[i].dev_id;
		bool should_remove = false;
		
		// NOTE(ljre): If a device was unplugged, IMMDevice_GetState() on that device will segfault.
		//             Getting a new IMMDevice from the enumerator _and then_ calling IMMDevice_GetState()
		//             works as expected. I don't know why this happens.
		
		IMMDevice* tmp;
		if (!SUCCEEDED(IMMDeviceEnumerator_GetDevice(g_audio.device_enumerator, dev_id, &tmp)))
			should_remove = true;
		else
		{
			DWORD state;
			hr = IMMDevice_GetState(tmp, &state);
			
			if (!SUCCEEDED(hr) || state != DEVICE_STATE_ACTIVE)
				should_remove = true;
			
			IMMDevice_Release(tmp);
		}
		
		if (should_remove)
		{
			// NOTE(ljre): If uncommented, segfault. Maybe the string is already free'd when the device is
			//             unplugged? Either way, this code should run so infrequently that it shouldn't be
			//             a problem.
			
			//CoTaskMemFree(dev_id);
			
			IMMDevice_Release(immdevice);
			
			if (g_audio.active_device_index == i)
			{
				g_audio.active_device_index = UINT32_MAX;
				g_audio.should_try_to_keep_on_default_endpoint = true;
				
				g_os.audio.current_device_id = 0;
			}
			else if (g_audio.active_device_index > i && g_audio.active_device_index != UINT32_MAX)
				--g_audio.active_device_index;
			
			intsize remaining = g_audio.device_count - i - 1;
			
			if (remaining > 0)
			{
				MemoryMove(&g_audio.devices[i], &g_audio.devices[i + 1], sizeof(g_audio.devices[0]) * remaining);
				MemoryMove(&g_audio.devices_info[i], &g_audio.devices_info[i + 1], sizeof(g_audio.devices_info[0]) * remaining);
			}
			
			--g_audio.device_count;
			--i;
		}
	}
	
	//- Enum available devices
	IMMDeviceCollection* collection;
	hr = IMMDeviceEnumerator_EnumAudioEndpoints(g_audio.device_enumerator, eRender, DEVICE_STATE_ACTIVE, &collection);
	
	if (!SUCCEEDED(hr))
		result = false;
	else
	{
		uint32 count;
		if (!SUCCEEDED(IMMDeviceCollection_GetCount(collection, &count)))
			count = 0;
		
		for (uint32 i = 0; i < count; ++i)
		{
			IMMDevice* immdevice = NULL;
			LPWSTR dev_id;
			
			if (!SUCCEEDED(IMMDeviceCollection_Item(collection, i, &immdevice)) ||
				!SUCCEEDED(IMMDevice_GetId(immdevice, &dev_id)))
			{
				if (immdevice)
					IMMDevice_Release(immdevice);
				
				result = false;
				break;
			}
			
			int32 found_index = -1;
			for (int32 j = 0; j < g_audio.device_count; ++j)
			{
				bool are_same_device = Win32_AreSameImmDevices_(dev_id, g_audio.devices[j].dev_id);
				
				if (are_same_device)
				{
					found_index = j;
					break;
				}
			}
			
			bool successfully_added = false;
			
			switch (0) case 0:
			if (found_index == -1 && g_audio.device_count < ArrayLength(g_audio.devices))
			{
				Win32_AudioDevice this_device = { 0 };
				
				IPropertyStore* property_store;
				hr = IMMDevice_OpenPropertyStore(immdevice, STGM_READ, &property_store);
				if (!SUCCEEDED(hr))
					break;
				
				PROPVARIANT property_interface_name = { 0 };
				PROPVARIANT property_description = { 0 };
				PROPVARIANT property_name = { 0 };
				PropVariantInit(&property_interface_name);
				PropVariantInit(&property_description);
				PropVariantInit(&property_name);
				
				if (!SUCCEEDED(IPropertyStore_GetValue(property_store, &PKEY_DeviceInterface_FriendlyName, &property_interface_name))
					|| !SUCCEEDED(IPropertyStore_GetValue(property_store, &PKEY_Device_DeviceDesc, &property_description))
					|| !SUCCEEDED(IPropertyStore_GetValue(property_store, &PKEY_Device_FriendlyName, &property_name)))
				{
					PropVariantClear(&property_interface_name);
					PropVariantClear(&property_description);
					PropVariantClear(&property_name);
					IPropertyStore_Release(property_store);
					break;
				}
				
				WideCharToMultiByte(CP_UTF8, 0, property_interface_name.pwszVal, -1, (char*)this_device.interface_name, ArrayLength(this_device.interface_name), NULL, NULL);
				WideCharToMultiByte(CP_UTF8, 0, property_description.pwszVal, -1, (char*)this_device.description, ArrayLength(this_device.description), NULL, NULL);
				WideCharToMultiByte(CP_UTF8, 0, property_name.pwszVal, -1, (char*)this_device.name, ArrayLength(this_device.name), NULL, NULL);
				
				PropVariantClear(&property_interface_name);
				PropVariantClear(&property_description);
				PropVariantClear(&property_name);
				IPropertyStore_Release(property_store);
				
				this_device.immdevice = immdevice;
				this_device.id = ++g_audio.device_id_counter;
				this_device.dev_id = dev_id;
				g_audio.devices[g_audio.device_count++] = this_device;
				successfully_added = true;
			}
			
			if (!successfully_added)
			{
				CoTaskMemFree(dev_id);
				IMMDevice_Release(immdevice);
			}
		}
		
		IMMDeviceCollection_Release(collection);
		collection = NULL;
		
		//- Update infos
		g_os.audio.device_count = g_audio.device_count;
		g_os.audio.devices = g_audio.devices_info;
		
		for (int32 i = 0; i < g_audio.device_count; ++i)
		{
			Win32_AudioDevice* device = &g_audio.devices[i];
			OS_AudioDeviceInfo* info = &g_audio.devices_info[i];
			
			info->interface_name = StrMake(MemoryStrnlen((char*)device->interface_name, ArrayLength(device->interface_name)), device->interface_name);
			info->description = StrMake(MemoryStrnlen((char*)device->description, ArrayLength(device->description)), device->description);
			info->name = StrMake(MemoryStrnlen((char*)device->name, ArrayLength(device->name)), device->name);
			info->id = device->id;
		}
	}
	
	return result;
}

static uint32
Win32_FindDefaultAudioEndpoint_(void)
{
	Trace();
	uint32 id = 0;
	
	IMMDevice* immdevice;
	HRESULT hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(g_audio.device_enumerator, eRender, eConsole, &immdevice);
	if (SUCCEEDED(hr))
	{
		LPWSTR dev_id;
		hr = IMMDevice_GetId(immdevice, &dev_id);
		
		if (SUCCEEDED(hr))
		{
			for (int32 i = 0; i < g_audio.device_count; ++i)
			{
				bool is_same_device = Win32_AreSameImmDevices_(dev_id, g_audio.devices[i].dev_id);
				
				if (is_same_device)
				{
					id = g_audio.devices[i].id;
					break;
				}
			}
			
			CoTaskMemFree(dev_id);
		}
		
		IMMDevice_Release(immdevice);
	}
	
	return id;
}

static bool
Win32_ChangeAudioEndpoint_(uint32 id)
{
	Trace();
	bool result = true;
	
	IAudioClient* audio_client = NULL;
	IAudioRenderClient* audio_render_client = NULL;
	int32 device_index = -1;
	int32 new_frame_pull_rate = 0;
	int32 new_sample_rate = 0;
	int32 new_channels = 0;
	HANDLE event = NULL;
	
	//- Try to find the device by the ID
	for (int32 i = 0; i < g_audio.device_count; ++i)
	{
		if (g_audio.devices[i].id == id)
		{
			device_index = i;
			break;
		}
	}
	
	//- Try to create the client and render client interfaces
	if (device_index == -1)
		result = false;
	else
	{
		IMMDevice* immdevice = g_audio.devices[device_index].immdevice;
		WAVEFORMATEX* wave_format = NULL;
		
		HRESULT hr = IMMDevice_Activate(immdevice, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audio_client);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		hr = IAudioClient_GetMixFormat(audio_client, &wave_format);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		uint16 channels = wave_format->nChannels;
		uint32 sample_rate = wave_format->nSamplesPerSec;
		
		const REFERENCE_TIME requested_sound_duration = 10000000;
		uint16 bytes_per_sample = sizeof(int16);
		uint16 bits_per_sample = bytes_per_sample * 8;
		uint16 block_align = channels * bytes_per_sample;
		uint32 average_bytes_per_second = block_align * sample_rate;
		
		WAVEFORMATEX new_wave_format = {
			.wFormatTag = WAVE_FORMAT_PCM,
			.nChannels = channels,
			.nSamplesPerSec = sample_rate,
			.nAvgBytesPerSec = average_bytes_per_second,
			.nBlockAlign = block_align,
			.wBitsPerSample = bits_per_sample,
			.cbSize = 0,
		};
		
		hr = IAudioClient_Initialize(
			audio_client, AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST |
			AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
			requested_sound_duration, 0, &new_wave_format, NULL);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		hr = IAudioClient_GetService(audio_client, &IID_IAudioRenderClient, (void**)&audio_render_client);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		REFERENCE_TIME reftime;
		hr = IAudioClient_GetDevicePeriod(audio_client, &reftime, NULL);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		new_frame_pull_rate = 2 * (int32)(reftime * (int64)(sample_rate / channels) / 10000000);
		new_sample_rate = (int32)sample_rate;
		new_channels = (int32)channels;
		
		event = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!event)
			goto lbl_error;
		
		hr = IAudioClient_SetEventHandle(audio_client, event);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		hr = IAudioClient_Start(audio_client);
		if (!SUCCEEDED(hr))
			goto lbl_error;
		
		CoTaskMemFree(wave_format);
		
		//- Error
		if (0) lbl_error:
		{
			result = false;
			
			if (event)
				CloseHandle(event);
			if (wave_format)
				CoTaskMemFree(wave_format);
			if (audio_render_client)
				IAudioRenderClient_Release(audio_render_client);
			if (audio_client)
				IAudioClient_Release(audio_client);
		}
	}
	
	//- Apply changes if needed.
	if (result)
	{
		OS_LockExclusive(&g_audio.client_lock);
		
		if (g_audio.render_client)
			IAudioClient_Release(g_audio.render_client);
		
		if (g_audio.client)
		{
			IAudioClient_Stop(g_audio.client);
			IAudioClient_Release(g_audio.client);
		}
		
		g_audio.active_device_index = device_index;
		g_audio.client = audio_client;
		g_audio.render_client = audio_render_client;
		g_audio.frame_pull_rate = new_frame_pull_rate;
		g_audio.sample_rate = new_sample_rate;
		g_audio.channels = new_channels;
		
		// NOTE(ljre): Making sure there's always a valid event assigned to 'g_audio.event'...
		HANDLE old_event = g_audio.event;
		g_audio.event = event;
		
		if (old_event)
			CloseHandle(old_event);
		
		OS_UnlockExclusive(&g_audio.client_lock);
		
		g_os.audio.current_device_id = id;
	}
	
	return result;
}

//~ Internal API
static void Win32_DeinitAudio(OS_State* os_state);

static bool
Win32_InitAudio(OS_State* os_state)
{
	Trace();
	
	OS_InitRWLock(&g_audio.client_lock);
	
	if (!Win32_LoadComLibrary_())
		return false;
	
	HRESULT hr = 0;
	
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&g_audio.device_enumerator);
	if (!SUCCEEDED(hr))
		return Win32_DeinitAudio(os_state), false;
	
	if (!Win32_EnumerateAudioEndpoints_())
		return Win32_DeinitAudio(os_state), false;
	
	if (!Win32_ChangeAudioEndpoint_(Win32_FindDefaultAudioEndpoint_()))
		return Win32_DeinitAudio(os_state), false;
	
	g_audio.thread_proc = g_win32.app_api->pull_audio_samples;
	g_audio.thread_userdata = g_win32.audiothread_user_data;
	g_audio.thread = CreateThread(NULL, 0, Win32_AudioThreadProc_, NULL, 0, NULL);
	if (!g_audio.thread)
		return Win32_DeinitAudio(os_state), false;
	
	SetThreadPriority(g_audio.thread, THREAD_PRIORITY_TIME_CRITICAL);
	
	os_state->audio.mix_sample_rate = g_audio.sample_rate;
	os_state->audio.mix_channels = g_audio.channels;
	os_state->audio.mix_frame_pull_rate = g_audio.frame_pull_rate;
	os_state->audio.device_count = g_audio.device_count;
	os_state->audio.devices = g_audio.devices_info;
	os_state->audio.current_device_id = g_audio.devices[g_audio.active_device_index].id;
	
	return true;
}

static void
Win32_DeinitAudio(OS_State* os_state)
{
	Trace();
	
	if (g_audio.thread)
	{
		TerminateThread(g_audio.thread, 0);
		CloseHandle(g_audio.thread);
	}
	
	if (g_audio.event)
		CloseHandle(g_audio.event);
	if (g_audio.render_client)
		IAudioRenderClient_Release(g_audio.render_client);
	if (g_audio.client)
		IAudioClient_Release(g_audio.client);
	if (g_audio.device_enumerator)
		IMMDeviceEnumerator_Release(g_audio.device_enumerator);
	for (int32 i = 0; i < g_audio.device_count; ++i)
	{
		CoTaskMemFree(g_audio.devices[i].dev_id);
		IMMDevice_Release(g_audio.devices[i].immdevice);
	}
	
	MemoryZero(&g_audio, sizeof(g_audio));
	MemoryZero(&os_state->audio, sizeof(os_state->audio));
	
	os_state->has_audio = false;
}

static void
Win32_UpdateAudioEndpointIfNeeded(void)
{
	Trace();
	
	if (g_os.has_audio)
	{
		Win32_EnumerateAudioEndpoints_();
		uint32 default_endpoint_id = Win32_FindDefaultAudioEndpoint_();
		
		if (g_audio.active_device_index == UINT32_MAX ||
			g_audio.should_try_to_keep_on_default_endpoint && g_audio.devices[g_audio.active_device_index].id != default_endpoint_id)
		{
			Win32_ChangeAudioEndpoint_(default_endpoint_id);
		}
	}
}
