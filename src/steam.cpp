#ifdef CONFIG_ENABLE_STEAM
#include "config.h"
#include "api_os.h"
#include "api_steam.h"
#include <steam/steam_api.h>

struct S_ScratchScope_
{
	ArenaSavepoint savepoint;
	
	S_ScratchScope_(Arena* arena)
	{ savepoint = ArenaSave(arena); }
	~S_ScratchScope_()
	{ ArenaRestore(savepoint); }
};

extern void* operator new(size_t size, void* ptr);

DisableWarnings(); // NOTE(ljre): -Winvalid-offsetof
struct S_Callbacks_
{
	STEAM_CALLBACK(S_Callbacks_, LobbyGameCreated, LobbyGameCreated_t);
	STEAM_CALLBACK(S_Callbacks_, GameLobbyJoinRequested, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(S_Callbacks_, GameRichPresenceJoinRequested, GameRichPresenceJoinRequested_t);
	STEAM_CALLBACK(S_Callbacks_, AvatarImageLoaded, AvatarImageLoaded_t);
	STEAM_CALLBACK(S_Callbacks_, UserStatsReceived, UserStatsReceived_t);
	
	CCallResult<S_Callbacks_, LobbyCreated_t> result_LobbyCreated;
	CCallResult<S_Callbacks_, GlobalAchievementPercentagesReady_t> result_GlobalAchievementPercentagesReady;
	
	inline void LobbyCreated(LobbyCreated_t* param, bool io_failure);
	inline void GlobalAchievementPercentagesReady(GlobalAchievementPercentagesReady_t* param, bool io_failure);
};
ReenableWarnings();

struct S_Globals_
{
	bool initialized;
	
	ISteamClient* client;
	ISteamUser* user;
	ISteamFriends* friends;
	ISteamMatchmaking* matchmaking;
	ISteamNetworkingSockets* netsockets;
	ISteamUtils* utils;
	ISteamUserStats* stats;
	S_Callbacks_* callbacks;
	
	String nickname;
	CSteamID userid;
}
static g_steam;

void
S_Callbacks_::LobbyGameCreated(LobbyGameCreated_t* param)
{
	OS_DebugLog("[api_steam] Callback: LobbyGameCreated\n");
}

void
S_Callbacks_::GameLobbyJoinRequested(GameLobbyJoinRequested_t* param)
{
	OS_DebugLog("[api_steam] Callback: GameLobbyJoinRequested\n");
}

void
S_Callbacks_::GameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* param)
{
	OS_DebugLog("[api_steam] Callback: GameRichPresenceJoinRequested\n");
}

void
S_Callbacks_::AvatarImageLoaded(AvatarImageLoaded_t* param)
{
	OS_DebugLog("[api_steam] Callback: AvatarImageLoaded\n");
}

void
S_Callbacks_::UserStatsReceived(UserStatsReceived_t* param)
{
	OS_DebugLog("[api_steam] Callback: UserStatsReceived\n");
	
	SteamAPICall_t call = g_steam.stats->RequestGlobalAchievementPercentages();
	g_steam.callbacks->result_GlobalAchievementPercentagesReady.Set(call, g_steam.callbacks, &S_Callbacks_::GlobalAchievementPercentagesReady);
}

inline void
S_Callbacks_::LobbyCreated(LobbyCreated_t* param, bool io_failure)
{
	OS_DebugLog("[api_steam] Result Callback: LobbyCreated\n");
}

inline void
S_Callbacks_::GlobalAchievementPercentagesReady(GlobalAchievementPercentagesReady_t* param, bool io_failure)
{
	OS_DebugLog("[api_steam] Result Callback: GlobalAchievementPercentagesReady\n");
}

//~ API
API bool
S_Init(uint64 desired_app_id)
{
	Trace();
	Assert(desired_app_id <= UINT32_MAX);
	
	bool ok = true;
	ok = ok && (!desired_app_id || SteamAPI_RestartAppIfNecessary((uint32)desired_app_id));
	ok = ok && SteamAPI_Init();
	
	if (ok)
	{
		g_steam.initialized = true;
		g_steam.client = SteamClient();
		g_steam.user = SteamUser();
		g_steam.friends = SteamFriends();
		g_steam.matchmaking = SteamMatchmaking();
		g_steam.netsockets = SteamNetworkingSockets();
		g_steam.utils = SteamUtils();
		g_steam.stats = SteamUserStats();
		
		alignas(S_Callbacks_) static uint8 local_callbacks[sizeof(S_Callbacks_)];
		g_steam.callbacks = new(local_callbacks) S_Callbacks_();
		
		const char* name = g_steam.friends->GetPersonaName();
		uintsize name_size = MemoryStrlen(name);
		g_steam.nickname = StrMake(name_size, name);
		g_steam.userid = g_steam.user->GetSteamID();
		
		g_steam.friends->GetLargeFriendAvatar(g_steam.userid);
		g_steam.stats->RequestCurrentStats();
	}
	
	return ok;
}

API void
S_Update(void)
{
	Trace();
	
	if (g_steam.initialized)
	{
		SteamAPI_RunCallbacks();
	}
}

API void
S_Deinit(void)
{
	Trace();
	
	if (g_steam.initialized)
	{
		g_steam.initialized = false;
		SteamAPI_Shutdown();
	}
}

API bool
S_IsInitialized(void)
{
	return g_steam.initialized;
}

API String
S_GetUserNickname(void)
{
	Trace();
	
	if (g_steam.initialized)
		return g_steam.nickname;
	
	return StrNull;
}

API bool
S_GetUserLargeAvatar(S_ImageHandle* out_handle)
{
	Trace();
	
	if (g_steam.initialized)
	{
		int32 result = g_steam.friends->GetLargeFriendAvatar(g_steam.userid);
		
		if (result >= 0)
		{
			out_handle->id = result;
			return true;
		}
	}
	
	return false;
}

API bool
S_ImageGetSize(S_ImageHandle handle, int32* out_width, int32* out_height)
{
	Trace();
	uint32 width, height;
	
	if (g_steam.initialized && g_steam.utils->GetImageSize(handle.id, &width, &height))
	{
		*out_width = (int32)width;
		*out_height = (int32)height;
		
		return true;
	}
	
	return false;
}

API bool
S_ImageGetRgba(S_ImageHandle handle, void* out_buffer, uintsize buffer_size)
{
	Trace();
	SafeAssert(buffer_size <= INT32_MAX);
	
	if (g_steam.initialized)
		return g_steam.utils->GetImageRGBA(handle.id, (uint8*)out_buffer, (int32)buffer_size);
	
	return false;
}

API bool
S_AchievementIsUnlocked(Arena* scratch_arena, String name, bool* out_is_unlocked, uint64* out_unlock_date)
{
	Trace();
	bool result = false;
	bool is_unlocked = false;
	uint32 unlock_date = 0; // UNIX timestamp, in seconds
	
	// TODO(ljre): Steam API uses uint32. Is there a way to use uint64 instead?
	
	if (g_steam.initialized)
	{
		S_ScratchScope_ scratch(scratch_arena);
		const char* cstr_name = ArenaPushCString(scratch_arena, name);
		
		if (cstr_name)
			result = g_steam.stats->GetAchievementAndUnlockTime(cstr_name, &is_unlocked, &unlock_date);
	}
	
	if (out_is_unlocked)
		*out_is_unlocked = is_unlocked;
	if (out_unlock_date)
		*out_unlock_date = (uint64)unlock_date;
	
	return result;
}

API bool
S_AchievementGetUnlockPercentage(Arena* scratch_arena, String name, float32* out_percentage)
{
	Trace();
	bool result = false;
	float32 percentage = 0.0f;
	
	if (g_steam.initialized)
	{
		S_ScratchScope_ scratch(scratch_arena);
		const char* cstr_name = ArenaPushCString(scratch_arena, name);
		
		if (cstr_name)
			result = g_steam.stats->GetAchievementAchievedPercent(cstr_name, &percentage);
	}
	
	if (out_percentage)
		*out_percentage = percentage;
	
	return result;
}

API bool
S_AchievementGetInfo(Arena* scratch_arena, String name, String* out_local_name, String* out_local_desc, bool* out_is_hidden)
{
	Trace();
	bool result = false;
	String local_name = {};
	String local_desc = {};
	bool is_hidden = false;
	
	if (g_steam.initialized)
	{
		S_ScratchScope_ scratch(scratch_arena);
		const char* cstr_name = ArenaPushCString(scratch_arena, name);
		
		if (cstr_name)
		{
			const char* cstr_local_name = g_steam.stats->GetAchievementDisplayAttribute(cstr_name, "name");
			const char* cstr_local_desc = g_steam.stats->GetAchievementDisplayAttribute(cstr_name, "desc");
			const char* cstr_is_hidden = g_steam.stats->GetAchievementDisplayAttribute(cstr_name, "hidden");
			
			if (cstr_local_name && cstr_local_desc && cstr_is_hidden &&
				cstr_local_name[0] && cstr_local_desc[0] && cstr_is_hidden[0])
			{
				local_name = StrMake(cstr_local_name, MemoryStrlen(cstr_local_name));
				local_desc = StrMake(cstr_local_desc, MemoryStrlen(cstr_local_desc));
				is_hidden = (cstr_is_hidden[0] != '0');
				
				result = true;
			}
		}
	}
	
	if (out_local_name)
		*out_local_name = local_name;
	if (out_local_desc)
		*out_local_desc = local_desc;
	if (out_is_hidden)
		*out_is_hidden = is_hidden;
	
	return result;
}

API bool
S_AchievementGetIcon(Arena* scratch_arena, String name, S_ImageHandle* out_handle)
{
	Trace();
	bool result = false;
	S_ImageHandle handle = {};
	
	if (g_steam.initialized)
	{
		S_ScratchScope_ scratch(scratch_arena);
		const char* cstr_name = ArenaPushCString(scratch_arena, name);
		
		if (cstr_name)
		{
			int32 handle_num = g_steam.stats->GetAchievementIcon(cstr_name);
			
			if (handle_num != 0)
			{
				result = true;
				handle.id = handle_num;
			}
		}
	}
	
	if (out_handle)
		*out_handle = handle;
	
	return result;
}

API bool
S_AchievementUnlock(Arena* scratch_arena, String name)
{
	Trace();
	bool result = false;
	
	if (g_steam.initialized)
	{
		S_ScratchScope_ scratch(scratch_arena);
		const char* cstr_name = ArenaPushCString(scratch_arena, name);
		
		if (cstr_name)
			result = g_steam.stats->SetAchievement(cstr_name);
	}
	
	return result;
}

API bool
S_ShowAchievementProgress(Arena* scratch_arena, String name, uint32 progress, uint32 max_progress)
{
	Trace();
	bool result = false;
	
	if (g_steam.initialized)
	{
		S_ScratchScope_ scratch(scratch_arena);
		const char* cstr_name = ArenaPushCString(scratch_arena, name);
		
		if (cstr_name)
			result = g_steam.stats->IndicateAchievementProgress(cstr_name, progress, max_progress);
	}
	
	return result;
}

API bool
S_UploadNewStats(void)
{
	Trace();
	bool result = false;
	
	if (g_steam.initialized)
		result = g_steam.stats->StoreStats();
	
	return result;
}

#endif //CONFIG_ENABLE_STEAM
