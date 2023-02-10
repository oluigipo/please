#ifdef CONFIG_ENABLE_STEAM
#include "config.h"
#include "api_os.h"
#include "api_steam.h"
#include <steam/steam_api.h>

DisableWarnings(); // NOTE(ljre): -Winvalid-offsetof
struct S_Callbacks_
{
	STEAM_CALLBACK(S_Callbacks_, LobbyGameCreated, LobbyGameCreated_t);
	STEAM_CALLBACK(S_Callbacks_, GameLobbyJoinRequested, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(S_Callbacks_, GameRichPresenceJoinRequested, GameRichPresenceJoinRequested_t);
	STEAM_CALLBACK(S_Callbacks_, AvatarImageLoaded, AvatarImageLoaded_t);
	
	CCallResult<S_Callbacks_, LobbyCreated_t> result_LobbyCreated;
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
	S_Callbacks_* callbacks;
	
	String nickname;
	CSteamID userid;
}
static g_steam;

void
S_Callbacks_::LobbyGameCreated(LobbyGameCreated_t* param)
{
	OS_DebugLog("[api_steam] Created game lobby!\n");
}

void
S_Callbacks_::GameLobbyJoinRequested(GameLobbyJoinRequested_t* param)
{
	OS_DebugLog("[api_steam] Trying to join a lobby!\n");
}

void
S_Callbacks_::GameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* param)
{
	OS_DebugLog("[api_steam] Trying to join a game!\n");
}

void
S_Callbacks_::AvatarImageLoaded(AvatarImageLoaded_t* param)
{
	OS_DebugLog("[api_steam] Avatar image loaded!\n");
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
		
		static S_Callbacks_ local_callbacks;
		g_steam.callbacks = &local_callbacks;
		
		const char* name = g_steam.friends->GetPersonaName();
		uintsize name_size = Mem_Strlen(name);
		g_steam.nickname = StrMake(name_size, name);
		g_steam.userid = g_steam.user->GetSteamID();
		
		g_steam.friends->GetLargeFriendAvatar(g_steam.userid);
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

#endif //CONFIG_ENABLE_STEAM
