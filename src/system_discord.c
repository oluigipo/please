// NOTE(ljre): If you want to use the 3.1.0 DLL for x86, you may want to change the calling convention
//             of all the callbacks, managers, and ProcDiscordCreate to __stdcall.
#pragma pack(push, 8)
#   include <discord_game_sdk.h>
#pragma pack(pop)

enum EDiscordResult typedef PlsDiscord_CreateProc(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

struct PlsDiscord_Client
{
	int64 appid;
	PlsDiscord_CreateProc* create;
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
typedef PlsDiscord_Client;

#define AssertResult(x, ...) if ((x) != DiscordResult_Ok) { Assert(!(void*) #x ); return __VA_ARGS__; }

//~ Functions
internal void
DiscordCallbackOnCurrentUserUpdate(void* data)
{
	struct DiscordUser user;
	enum EDiscordResult result;
	PlsDiscord_Client* discord = data;
	
	result = discord->users->get_current_user(discord->users, &user);
	AssertResult(result);
	
	discord->connected = true;
	discord->user = user;
	
	Platform_DebugLog("User ID: %lli\nUsername: %s\nDiscriminator: %u\n",
					  discord->user.id, discord->user.username, discord->user.discriminator);
}

internal void
DiscordCallbackUpdateActivity(void* data, enum EDiscordResult result)
{
	// nothing
}

//~ API
internal bool32
PlsDiscord_Init(int64 appid, PlsDiscord_Client* discord)
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
	if (!discord->create)
	{
		discord->create = Platform_LoadDiscordLibrary();
		
		if (!discord->create)
			return false;
	}
	
	if (discord->core)
		return true;
	
	struct DiscordCreateParams params;
	enum EDiscordResult result;
	
	// DiscordCreate
	DiscordCreateParamsSetDefault(&params);
	
	params.client_id = appid;
	params.flags = DiscordCreateFlags_NoRequireDiscord;
	params.event_data = discord;
	params.activity_events = &activity_events;
	params.relationship_events = &relationship_events;
	params.user_events = &user_events;
	params.lobby_events = &lobby_events;
	
	result = discord->create(DISCORD_VERSION, &params, &discord->core);
	if (result != DiscordResult_Ok || !discord->core)
		return false;
	
	// get managers
	discord->users = discord->core->get_user_manager(discord->core);
	discord->achievements = discord->core->get_achievement_manager(discord->core);
	discord->activities = discord->core->get_activity_manager(discord->core);
	discord->application = discord->core->get_application_manager(discord->core);
	discord->lobbies = discord->core->get_lobby_manager(discord->core);
	discord->relationships = discord->core->get_relationship_manager(discord->core);
	
	discord->appid = appid;
	discord->connected = false;
	
	PlsDiscord_UpdateActivity(DiscordActivityType_Playing, StrNull, StrNull, StrNull);
	
	return true;
}

internal void
PlsDiscord_UpdateActivityAssets(PlsDiscord_Client* discord, String large_image, String large_text, String small_image, String small_text)
{
	if (large_image.len)
		String_Copy(Str(discord->activity.assets.large_image), large_image);
	if (large_text.len)
		String_Copy(Str(discord->activity.assets.large_text), large_text);
	if (small_image.len)
		String_Copy(Str(discord->activity.assets.small_image), small_image);
	if (small_text.len)
		String_Copy(Str(discord->activity.assets.small_text), small_text);
}

internal void
PlsDiscord_UpdateActivity(PlsDiscord_Client* discord, int32 type, String name, String state, String details)
{
	discord->activity.type = type;
	discord->activity.application_id = discord->appid;
	
	String_Copy(Str(discord->activity.name), name);
	String_Copy(Str(discord->activity.state), state);
	String_Copy(Str(discord->activity.details), details);
	
	discord->activities->update_activity(discord->activities, &discord->activity, NULL, DiscordCallbackUpdateActivity);
}

internal void
PlsDiscord_Update(PlsDiscord_Client* discord)
{
	if (discord->core)
	{
		discord->core->run_callbacks(discord->core);
		
		if (discord->lobbies)
			discord->lobbies->flush_network(discord->lobbies);
	}
}

internal void
PlsDiscord_Deinit(PlsDiscord_Client* discord)
{
	if (discord->core)
	{
		discord->core->destroy(discord->core);
		discord->core = NULL;
	}
}

#undef AssertResult
