#ifdef CONFIG_ENABLE_STEAM
#include "config.h"
#include "common.h"
#include "api_steam.h"
#include <steam/steam_api.h>

struct S_Globals_
{
	bool initialized;
	
	ISteamClient* client;
	ISteamFriends* friends;
}
g_steam;

API bool
S_Init(uint64 desired_app_id)
{
	Assert(desired_app_id <= UINT32_MAX);
	
	bool ok = true;
	ok = ok && (!desired_app_id || SteamAPI_RestartAppIfNecessary((uint32)desired_app_id));
	ok = ok && SteamAPI_Init();
	
	if (ok)
	{
		g_steam.initialized = true;
		g_steam.client = SteamClient();
		g_steam.friends = SteamFriends();
		
		g_steam.friends->ActivateGameOverlay("friends");
	}
	
	return ok;
}

API void
S_Update(void)
{
	if (g_steam.initialized)
	{
		SteamAPI_RunCallbacks();
	}
}

API void
S_Deinit(void)
{
	if (g_steam.initialized)
	{
		g_steam.initialized = false;
		SteamAPI_Shutdown();
	}
}

#endif //CONFIG_ENABLE_STEAM
