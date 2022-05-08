// NOTE(ljre): If you want to use the 3.1.0 DLL for x86, you may want to change the calling convention
//             of all the callbacks, managers, and PlsDiscord_CreateProc to __stdcall.
#pragma pack(push, 8)
#   include <discord_game_sdk.h>
#pragma pack(pop)

enum EDiscordResult typedef PlsDiscord_CreateProc(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

struct PlsDiscord_Client
{
	int64 appid;
	
	bool8 connected;
	bool8 connecting_to_lobby;
	
	PlsDiscord_CreateProc* create;
	
	struct DiscordActivity activity;
	struct DiscordUser user;
	struct DiscordLobby lobby;
	
	struct IDiscordCore* core;
	struct IDiscordUserManager* users;
	struct IDiscordAchievementManager* achievements;
	struct IDiscordActivityManager* activities;
	struct IDiscordRelationshipManager* relationships;
	struct IDiscordApplicationManager* application;
	struct IDiscordLobbyManager* lobbies;
}
typedef PlsDiscord_Client;

#include "common_printf.h"

//~ NOTE(ljre): Callbacks
internal void
DiscordCallbackUpdateActivity(void* data, enum EDiscordResult result)
{
	// nothing
}

internal void
DiscordCallbackCreateLobby(void* event_data, enum EDiscordResult result, struct DiscordLobby* lobby)
{
	PlsDiscord_Client* discord = event_data;
	discord->connecting_to_lobby = false;
	
	if (result != DiscordResult_Ok)
		return;
	
	discord->lobby = *lobby;
	Platform_DebugLog("Created Lobby!\n");
	
	MemCopy(discord->activity.secrets.join, discord->lobby.secret, sizeof(discord->lobby.secret));
	SPrintf(discord->activity.party.id, sizeof(discord->activity.party.id), "%lli", discord->lobby.id);
	discord->activity.party.size.current_size = 1;
	discord->activity.party.size.max_size = discord->lobby.capacity;
	
	discord->activities->update_activity(discord->activities, &discord->activity, NULL, DiscordCallbackUpdateActivity);
}

//~ NOTE(ljre): Events
internal void
DiscordEventOnCurrentUserUpdate(void* data)
{
	struct DiscordUser user;
	enum EDiscordResult result;
	PlsDiscord_Client* discord = data;
	
	result = discord->users->get_current_user(discord->users, &user);
	if (result != DiscordResult_Ok)
		return;
	
	discord->connected = true;
	discord->user = user;
	
	Platform_DebugLog("User ID: %lli\nUsername: %s\nDiscriminator: %s\n",
					  discord->user.id, discord->user.username, discord->user.discriminator);
}

internal void
DiscordEventOnLobbyUpdate(void* event_data, int64 lobby_id)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnLobbyUpdate -- %lli\n", lobby_id);
}

internal void
DiscordEventOnLobbyDelete(void* event_data, int64 lobby_id, uint32 reason)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnLobbyDelete -- %lli\t%u\n", lobby_id, reason);
}

internal void
DiscordEventOnMemberConnect(void* event_data, int64 lobby_id, int64 user_id)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnMemberConnect -- %lli\t%lli\n", lobby_id, user_id);
}

internal void
DiscordEventOnMemberUpdate(void* event_data, int64 lobby_id, int64 user_id)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnMemberUpdate -- %lli\t%lli\n", lobby_id, user_id);
}

internal void
DiscordEventOnMemberDisconnect(void* event_data, int64 lobby_id, int64 user_id)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnMemberDisconnect -- %lli\t%lli\n", lobby_id, user_id);
}

internal void
DiscordEventOnLobbyMessage(void* event_data, int64 lobby_id, int64 user_id, uint8* data, uint32 data_length)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnLobbyMessage -- %lli\t%lli\t%p\t%u\n", lobby_id, user_id, data, data_length);
}

internal void
DiscordEventOnSpeaking(void* event_data, int64 lobby_id, int64 user_id, bool speaking)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnSpeaking -- %lli\t%lli\t%i\n", lobby_id, user_id, speaking);
}

internal void
DiscordEventOnNetworkMessage(void* event_data, int64 lobby_id, int64 user_id, uint8 channel_id, uint8* data, uint32 data_length)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnNetworkMessage -- %lli\t%lli\t%i\t%p\t%u\n", lobby_id, user_id, channel_id, data, data_length);
}

internal void
DiscordEventOnActivityJoin(void* event_data, const char* secret)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnActivityJoin -- %s\n", secret);
}

internal void
DiscordEventOnActivitySpectate(void* event_data, const char* secret)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnActivitySpectate -- %s\n", secret);
}

internal void
DiscordEventOnActivityJoinRequest(void* event_data, struct DiscordUser* user)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnActivityJoinRequest -- %lli\n", user->id);
}

internal void
DiscordEventOnActivityInvite(void* event_data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnActivityJoinRequest -- %i\t%lli\t%i\n", type, user->id, activity->type);
}

internal void
DiscordEventOnRefresh(void* event_data)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnRefresh -- \n");
}

internal void
DiscordEventOnRelationshipUpdate(void* event_data, struct DiscordRelationship* relationship)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnRelationshipUpdate -- %p\n", relationship);
}

internal void
DiscordEventOnMessage(void* event_data, DiscordNetworkPeerId peer_id, DiscordNetworkChannelId channel_id, uint8* data, uint32 data_length)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnMessage -- %llu\t%hhu\t%p\t%u\n", peer_id, channel_id, data, data_length);
}

internal void
DiscordEventOnRouteUpdate(void* event_data, const char* route_data)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnRouteUpdate -- %s\n", route_data);
}

internal void
DiscordEventOnToggle(void* event_data, bool locked)
{
	PlsDiscord_Client* discord = event_data;
	(void)discord;
	
	Platform_DebugLog("Discord: OnToggle -- %s\n", locked ? "true" : "false");
}

//~ API
internal bool32
PlsDiscord_Init(int64 appid, PlsDiscord_Client* discord)
{
	// Events
	static struct IDiscordUserEvents user_events = {
		.on_current_user_update = DiscordEventOnCurrentUserUpdate,
	};
	
	static struct IDiscordActivityEvents activity_events = {
		.on_activity_join = DiscordEventOnActivityJoin,
		.on_activity_spectate = DiscordEventOnActivitySpectate,
		.on_activity_join_request = DiscordEventOnActivityJoinRequest,
		.on_activity_invite = DiscordEventOnActivityInvite,
	};
	
	static struct IDiscordRelationshipEvents relationship_events = {
		.on_refresh = DiscordEventOnRefresh,
		.on_relationship_update = DiscordEventOnRelationshipUpdate,
	};
	
	static struct IDiscordLobbyEvents lobby_events = {
		.on_lobby_update = DiscordEventOnLobbyUpdate,
        .on_lobby_delete = DiscordEventOnLobbyDelete,
        .on_member_connect = DiscordEventOnMemberConnect,
        .on_member_update = DiscordEventOnMemberUpdate,
        .on_member_disconnect = DiscordEventOnMemberDisconnect,
        .on_lobby_message = DiscordEventOnLobbyMessage,
        .on_speaking = DiscordEventOnSpeaking,
        .on_network_message = DiscordEventOnNetworkMessage,
	};
	
	static struct IDiscordNetworkEvents network_events = {
		.on_message = DiscordEventOnMessage,
		.on_route_update = DiscordEventOnRouteUpdate,
	};
	
	static struct IDiscordOverlayEvents overlay_events = {
		.on_toggle = DiscordEventOnToggle,
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
	params.overlay_events = &overlay_events;
	params.network_events = &network_events;
	
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
	discord->connecting_to_lobby = false;
	MemSet(&discord->lobby, 0, sizeof(discord->lobby));
	
	discord->activities->update_activity(discord->activities, &discord->activity, NULL, DiscordCallbackUpdateActivity);
	
	return true;
}

internal void
PlsDiscord_UpdateActivity(PlsDiscord_Client* discord)
{
	Assert(discord->core);
	
	discord->activities->update_activity(discord->activities, &discord->activity, NULL, DiscordCallbackUpdateActivity);
}

internal void
PlsDiscord_Update(PlsDiscord_Client* discord)
{
	Trace("PlsDiscord_Update");
	Assert(discord->core);
	
	if (discord->core)
	{
		discord->core->run_callbacks(discord->core);
		
		if (discord->lobbies)
			discord->lobbies->flush_network(discord->lobbies);
	}
}

internal bool32
PlsDiscord_CreateLobby(PlsDiscord_Client* discord)
{
	Assert(discord->core);
	
	struct IDiscordLobbyTransaction* transaction = NULL;
	enum EDiscordResult result;
	
	// NOTE(ljre): Transaction
	result = discord->lobbies->get_lobby_create_transaction(discord->lobbies, &transaction);
	if (result != DiscordResult_Ok)
		return false;
	
	result = transaction->set_type(transaction, DiscordLobbyType_Public);
	if (result != DiscordResult_Ok)
		return false;
	
	result = transaction->set_owner(transaction, discord->user.id);
	if (result != DiscordResult_Ok)
		return false;
	
	result = transaction->set_capacity(transaction, 5);
	if (result != DiscordResult_Ok)
		return false;
	
	// NOTE(ljre): Actually Create Lobby
	discord->lobbies->create_lobby(discord->lobbies, transaction, discord, DiscordCallbackCreateLobby);
	return true;
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
