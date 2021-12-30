// NOTE(ljre): If you want to use the 3.1.0 DLL for x86, you may want to change the calling convention
//             of all the callbacks, managers, and ProcDiscordCreate to __stdcall.
#pragma pack(push, 8)
#   include <discord_game_sdk.h>
#pragma pack(pop)

typedef enum EDiscordResult (*ProcDiscordCreate)(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

struct Discord_Client
{
	int64 appid;
	ProcDiscordCreate create;
	bool32 connected;
	
	struct DiscordActivity activity;
	struct DiscordUser user;
	
	struct IDiscordCore* core;
	struct IDiscordUserManager* users;
	struct IDiscordAchievementManager* achievements;
	struct IDiscordActivityManager* activities;
	struct IDiscordRelationshipManager* relationships;
	struct IDiscordApplicationManager* application;
	struct IDiscordLobbyManager* lobbies;
}
typedef Discord_Client;

#define AssertResult(x, ...) if ((x) != DiscordResult_Ok) { Assert(!(void*) #x ); return __VA_ARGS__; }

//~ Globals
internal Discord_Client global_discord;

//~ Functions
internal void
DiscordCallbackOnCurrentUserUpdate(void* data)
{
	struct DiscordUser user;
	enum EDiscordResult result;
	
	result = global_discord.users->get_current_user(global_discord.users, &user);
	AssertResult(result);
	
	global_discord.connected = true;
	global_discord.user = user;
	
	Platform_DebugLog("User ID: %lli\nUsername: %s\nDiscriminator: %u\n",
					  global_discord.user.id, global_discord.user.username, global_discord.user.discriminator);
}

internal void
DiscordCallbackUpdateActivity(void* data, enum EDiscordResult result)
{
	// nothing
}

//~ API
API bool32
Discord_Init(int64 appid)
{
	// Events
	static struct IDiscordUserEvents user_events = {
		.on_current_user_update = DiscordCallbackOnCurrentUserUpdate,
	};
	
	static struct IDiscordActivityEvents activity_events = {
		0
	};
	
	static struct IDiscordRelationshipEvents relationship_events = {
		0
	};
	
	static struct IDiscordLobbyEvents lobby_events = {
		0
	};
	
	// Init
	if (!global_discord.create)
	{
		global_discord.create = Platform_LoadDiscordLibrary();
		
		if (!global_discord.create)
			return false;
	}
	
	if (global_discord.core)
		return true;
	
	struct DiscordCreateParams params;
	enum EDiscordResult result;
	
	// DiscordCreate
	DiscordCreateParamsSetDefault(&params);
	
	params.client_id = appid;
	params.flags = DiscordCreateFlags_NoRequireDiscord;
	params.event_data = &global_discord;
	params.activity_events = &activity_events;
	params.relationship_events = &relationship_events;
	params.user_events = &user_events;
	params.lobby_events = &lobby_events;
	
	result = global_discord.create(DISCORD_VERSION, &params, &global_discord.core);
	if (result != DiscordResult_Ok || !global_discord.core)
		return false;
	
	// get managers
	global_discord.users = global_discord.core->get_user_manager(global_discord.core);
	global_discord.achievements = global_discord.core->get_achievement_manager(global_discord.core);
	global_discord.activities = global_discord.core->get_activity_manager(global_discord.core);
	global_discord.application = global_discord.core->get_application_manager(global_discord.core);
	global_discord.lobbies = global_discord.core->get_lobby_manager(global_discord.core);
	global_discord.relationships = global_discord.core->get_relationship_manager(global_discord.core);
	
	global_discord.appid = appid;
	global_discord.connected = false;
	
	Discord_UpdateActivity(DiscordActivityType_Playing, StrNull, StrNull, StrNull);
	
	return true;
}

API void
Discord_UpdateActivityAssets(String large_image, String large_text, String small_image, String small_text)
{
	if (large_image.len)
		String_Copy(Str(global_discord.activity.assets.large_image), large_image);
	if (large_text.len)
		String_Copy(Str(global_discord.activity.assets.large_text), large_text);
	if (small_image.len)
		String_Copy(Str(global_discord.activity.assets.small_image), small_image);
	if (small_text.len)
		String_Copy(Str(global_discord.activity.assets.small_text), small_text);
}

API void
Discord_UpdateActivity(int32 type, String name, String state, String details)
{
	global_discord.activity.type = type;
	global_discord.activity.application_id = global_discord.appid;
	
	String_Copy(Str(global_discord.activity.name), name);
	String_Copy(Str(global_discord.activity.state), state);
	String_Copy(Str(global_discord.activity.details), details);
	
	global_discord.activities->update_activity(global_discord.activities, &global_discord.activity, NULL, DiscordCallbackUpdateActivity);
}

API void
Discord_Update(void)
{
	if (global_discord.core)
	{
		global_discord.core->run_callbacks(global_discord.core);
		
		if (global_discord.lobbies)
			global_discord.lobbies->flush_network(global_discord.lobbies);
	}
}

API void
Discord_Deinit(void)
{
	if (global_discord.core)
	{
		global_discord.core->destroy(global_discord.core);
		global_discord.core = NULL;
	}
}

#undef AssertResult
