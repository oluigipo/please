#ifdef CONFIG_ENABLE_STEAM
#include "config.h"
#include "api_os.h"
#include "api_steam.h"
#include <steam/steam_api.h>

struct S_Globals_
{
	bool initialized;
	
	String nickname;
	ISteamClient* client;
	ISteamFriends* friends;
	ISteamMatchmaking* matchmaking;
	ISteamNetworkingMessages* netmessages;
}
static g_steam;

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
		g_steam.friends = SteamFriends();
		g_steam.matchmaking = SteamMatchmaking();
		g_steam.netmessages = SteamNetworkingMessages();
		
		const char* name = g_steam.friends->GetPersonaName();
		uintsize name_size = Mem_Strlen(name);
		
		g_steam.nickname = StrMake(name_size, name);
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

API String
S_GetUserNickname(void)
{
	Trace();
	
	if (g_steam.initialized)
		return g_steam.nickname;
	
	return StrNull;
}

#endif //CONFIG_ENABLE_STEAM
