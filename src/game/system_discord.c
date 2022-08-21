// NOTE(ljre): If you want to use the 3.1.0 DLL for x86, you may want to change the calling convention
//             of all the callbacks, managers, and PlsDiscord_CreateProc to __stdcall.
#pragma pack(push, 8)
#   include <discord_game_sdk.h>
#pragma pack(pop)

enum EDiscordResult typedef PlsDiscord_CreateProc_(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

enum PlsDiscord_EventKind
{
	PlsDiscord_EventKind_Null = 0,
	
	// NOTE(ljre): From callbacks
	PlsDiscord_EventKind_UpdateActivity,
	PlsDiscord_EventKind_CreateLobby,
	PlsDiscord_EventKind_DeleteLobby,
	
	// NOTE(ljre): From events
	PlsDiscord_EventKind_OnLobbyUpdate,
	PlsDiscord_EventKind_OnLobbyDelete,
	PlsDiscord_EventKind_OnLobbyMessage,
	PlsDiscord_EventKind_OnMemberConnect,
	PlsDiscord_EventKind_OnMemberUpdate,
	PlsDiscord_EventKind_OnMemberDisconnect,
	PlsDiscord_EventKind_OnSpeaking,
	PlsDiscord_EventKind_OnNetworkMessage,
	PlsDiscord_EventKind_OnActivityJoin,
	PlsDiscord_EventKind_OnActivitySpectate,
	PlsDiscord_EventKind_OnActivityJoinRequest,
	PlsDiscord_EventKind_OnActivityInvite,
	PlsDiscord_EventKind_OnCurrentUserUpdate,
	PlsDiscord_EventKind_OnRelationshipUpdate,
	PlsDiscord_EventKind_OnRefresh,
	PlsDiscord_EventKind_OnToggle,
	PlsDiscord_EventKind_OnMessage,
	PlsDiscord_EventKind_OnRouteUpdate,
}
typedef PlsDiscord_EventKind;

struct PlsDiscord_Event typedef PlsDiscord_Event;
struct PlsDiscord_Event
{
	PlsDiscord_EventKind kind;
	PlsDiscord_Event* next;
	
	union
	{
		struct { enum EDiscordResult result; struct DiscordLobby lobby; } create_lobby;
		struct { enum EDiscordResult result; } delete_lobby;
		struct { enum EDiscordResult result; } update_activity;
		
		struct { int64 lobby_id; } on_lobby_update;
		struct { int64 lobby_id; uint32 reason; } on_lobby_delete;
		struct { int64 lobby_id; int64 user_id; uint8* data; uint32 data_length; } on_lobby_message;
		
		struct { int64 lobby_id; int64 user_id; struct DiscordUser user; } on_member_connect;
		struct { int64 lobby_id; int64 user_id; struct DiscordUser user;} on_member_update;
		struct { int64 lobby_id; int64 user_id; } on_member_disconnect;
		struct { int64 lobby_id; int64 user_id; bool state; } on_speaking;
		
		struct { int64 lobby_id; int64 user_id; uint8 channel; uint8* data; uint32 data_length; } on_network_message;
		struct { const char* route_data; } on_route_update;
		
		struct { const char* secret; } on_activity_join;
		struct { const char* secret; } on_activity_spectate;
		struct { struct DiscordUser user; } on_activity_join_request;
		struct { enum EDiscordActivityActionType type; struct DiscordUser user; struct DiscordActivity activity; } on_activity_invite;
		
		struct { uint64 peer_id; uint8 channel; uint8* data; uint32 data_length; } on_message;
		struct { bool locked; } on_toggle;
		struct { enum EDiscordResult result; struct DiscordUser state; } on_current_user_update;
		struct { struct DiscordRelationship state; } on_relationship_update;
		
	};
};

struct PlsDiscord_Client
{
	int64 appid;
	bool8 connected;
	
	PlsDiscord_CreateProc_* create;
	
	struct DiscordActivity activity;
	struct DiscordUser user;
	struct DiscordLobby lobby;
	
	Arena* events_output_arena;
	PlsDiscord_Event** event_list;
	
	struct IDiscordCore* core;
	struct IDiscordUserManager* users;
	struct IDiscordAchievementManager* achievements;
	struct IDiscordActivityManager* activities;
	struct IDiscordRelationshipManager* relationships;
	struct IDiscordApplicationManager* application;
	struct IDiscordLobbyManager* lobbies;
	struct IDiscordNetworkManager* network;
	struct IDiscordOverlayManager* overlays;
	struct IDiscordImageManager* images;
}
typedef PlsDiscord_Client;

//~ NOTE(ljre): Helpers
internal void
PlsDiscord_PushEvent_(PlsDiscord_Client* discord, PlsDiscord_Event* event)
{
	PlsDiscord_Event* p = Arena_PushStructData(discord->events_output_arena, PlsDiscord_Event, event);
	*discord->event_list = p;
	discord->event_list = &p->next;
}

//~ NOTE(ljre): Callbacks
internal void
PlsDiscord_CallbackUpdateActivity_(void* event_data, enum EDiscordResult result)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_UpdateActivity,
		.update_activity = { result },
	};
	
	PlsDiscord_PushEvent_(discord, &event);
}

internal void
PlsDiscord_CallbackCreateLobby_(void* event_data, enum EDiscordResult result, struct DiscordLobby* lobby)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_CreateLobby,
		.create_lobby = {
			.result = result,
			.lobby = *lobby,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	
	if (result != DiscordResult_Ok)
		return;
	
	discord->lobby = *lobby;
	Platform_DebugLog("Created Lobby!\n");
	
	MemCopy(discord->activity.secrets.join, discord->lobby.secret, sizeof(discord->lobby.secret));
	SPrintf(discord->activity.party.id, sizeof(discord->activity.party.id), "%lli", discord->lobby.id);
	discord->activity.party.size.current_size = 1;
	discord->activity.party.size.max_size = discord->lobby.capacity;
	
	discord->activities->update_activity(discord->activities, &discord->activity, discord, PlsDiscord_CallbackUpdateActivity_);
}

internal void
PlsDiscord_CallbackDeleteLobby_(void* event_data, enum EDiscordResult result)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_DeleteLobby,
		.delete_lobby = {
			.result = result,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	
	if (result != DiscordResult_Ok)
		MemSet(&discord->lobby, 0, sizeof(discord->lobby));
}

//~ NOTE(ljre): Events
internal void
PlsDiscord_EventOnCurrentUserUpdate_(void* data)
{
	struct DiscordUser user = { 0 };
	enum EDiscordResult result;
	PlsDiscord_Client* discord = data;
	
	result = discord->users->get_current_user(discord->users, &user);
	
	if (result == DiscordResult_Ok)
		discord->user = user;
	
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnCurrentUserUpdate,
		.on_current_user_update = {
			.result = result,
			.state = user,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("User ID: %lli\nUsername: %s\nDiscriminator: %s\n", discord->user.id, discord->user.username, discord->user.discriminator);
}

internal void
PlsDiscord_EventOnLobbyUpdate_(void* event_data, int64 lobby_id)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnLobbyUpdate,
		.on_lobby_update = {
			.lobby_id = lobby_id,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnLobbyUpdate -- %lli\n", lobby_id);
}

internal void
PlsDiscord_EventOnLobbyDelete_(void* event_data, int64 lobby_id, uint32 reason)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnLobbyDelete,
		.on_lobby_delete = {
			.lobby_id = lobby_id,
			.reason = reason,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnLobbyDelete -- %lli\t%u\n", lobby_id, reason);
}

internal void
PlsDiscord_EventOnMemberConnect_(void* event_data, int64 lobby_id, int64 user_id)
{
	PlsDiscord_Client* discord = event_data;
	struct DiscordUser user;
	enum EDiscordResult result = discord->lobbies->get_member_user(discord->lobbies, lobby_id, user_id, &user);
	
	if (result == DiscordResult_Ok)
	{
		PlsDiscord_Event event = {
			.kind = PlsDiscord_EventKind_OnMemberConnect,
			.on_member_connect = {
				.lobby_id = lobby_id,
				.user_id = user_id,
				.user = user,
			},
		};
		
		PlsDiscord_PushEvent_(discord, &event);
		//Platform_DebugLog("Discord: OnMemberConnect -- %lli\t%lli\n", lobby_id, user_id);
	}
	else
		Platform_DebugLog("Discord: OnMemberConnect failed to fetch user data\n");
}

internal void
PlsDiscord_EventOnMemberUpdate_(void* event_data, int64 lobby_id, int64 user_id)
{
	PlsDiscord_Client* discord = event_data;
	struct DiscordUser user;
	enum EDiscordResult result = discord->lobbies->get_member_user(discord->lobbies, lobby_id, user_id, &user);
	
	if (result == DiscordResult_Ok)
	{
		PlsDiscord_Event event = {
			.kind = PlsDiscord_EventKind_OnMemberUpdate,
			.on_member_update = {
				.lobby_id = lobby_id,
				.user_id = user_id,
				.user = user,
			},
		};
		
		PlsDiscord_PushEvent_(discord, &event);
		//Platform_DebugLog("Discord: OnMemberUpdate -- %lli\t%lli\n", lobby_id, user_id);
	}
	else
		Platform_DebugLog("Discord: OnMemberUpdate failed to fetch user data\n");
}

internal void
PlsDiscord_EventOnMemberDisconnect_(void* event_data, int64 lobby_id, int64 user_id)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnMemberDisconnect,
		.on_member_disconnect = {
			.lobby_id = lobby_id,
			.user_id = user_id,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnMemberDisconnect -- %lli\t%lli\n", lobby_id, user_id);
}

internal void
PlsDiscord_EventOnLobbyMessage_(void* event_data, int64 lobby_id, int64 user_id, uint8* data, uint32 data_length)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnLobbyMessage,
		.on_lobby_message = {
			.lobby_id = lobby_id,
			.user_id = user_id,
			.data = Arena_PushMemoryAligned(discord->events_output_arena, data, data_length, 8),
			.data_length = data_length,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnLobbyMessage -- %lli\t%lli\t%p\t%u\n", lobby_id, user_id, data, data_length);
}

internal void
PlsDiscord_EventOnSpeaking_(void* event_data, int64 lobby_id, int64 user_id, bool speaking)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnSpeaking,
		.on_speaking = {
			.lobby_id = lobby_id,
			.user_id = user_id,
			.state = speaking,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnSpeaking -- %lli\t%lli\t%i\n", lobby_id, user_id, speaking);
}

internal void
PlsDiscord_EventOnNetworkMessage_(void* event_data, int64 lobby_id, int64 user_id, uint8 channel_id, uint8* data, uint32 data_length)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnNetworkMessage,
		.on_network_message = {
			.lobby_id = lobby_id,
			.user_id = user_id,
			.channel = channel_id,
			.data = Arena_PushMemoryAligned(discord->events_output_arena, data, data_length, 8),
			.data_length = data_length,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnNetworkMessage -- %lli\t%lli\t%i\t%p\t%u\n", lobby_id, user_id, channel_id, data, data_length);
}

internal void
PlsDiscord_EventOnActivityJoin_(void* event_data, const char* secret)
{
	PlsDiscord_Client* discord = event_data;
	
	uintsize length = 0;
	{
		const char* p = secret;
		while (*p++);
		
		length = p - secret;
		Assert(length <= UINT32_MAX);
	}
	
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnActivityJoin,
		.on_activity_join = {
			.secret = Arena_PushMemoryAligned(discord->events_output_arena, secret, length, 1),
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnActivityJoin -- %s\n", secret);
}

internal void
PlsDiscord_EventOnActivitySpectate_(void* event_data, const char* secret)
{
	PlsDiscord_Client* discord = event_data;
	
	uintsize length = 0;
	{
		const char* p = secret;
		while (*p++);
		
		length = p - secret;
		Assert(length <= UINT32_MAX);
	}
	
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnActivitySpectate,
		.on_activity_spectate = {
			.secret = Arena_PushMemoryAligned(discord->events_output_arena, secret, length, 1),
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnActivitySpectate -- %s\n", secret);
}

internal void
PlsDiscord_EventOnActivityJoinRequest_(void* event_data, struct DiscordUser* user)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnActivityJoinRequest,
		.on_activity_join_request = {
			.user = *user
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnActivityJoinRequest -- %lli\n", user->id);
}

internal void
PlsDiscord_EventOnActivityInvite_(void* event_data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnActivityInvite,
		.on_activity_invite = {
			.type = type,
			.user = *user,
			.activity = *activity,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnActivityJoinRequest -- %i\t%lli\t%i\n", type, user->id, activity->type);
}

internal void
PlsDiscord_EventOnRefresh_(void* event_data)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnRefresh,
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnRefresh -- \n");
}

internal void
PlsDiscord_EventOnRelationshipUpdate_(void* event_data, struct DiscordRelationship* relationship)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnRelationshipUpdate,
		.on_relationship_update = {
			.state = *relationship,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnRelationshipUpdate -- %p\n", relationship);
}

internal void
PlsDiscord_EventOnMessage_(void* event_data, DiscordNetworkPeerId peer_id, uint8 channel_id, uint8* data, uint32 data_length)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnMessage,
		.on_message = {
			.peer_id = peer_id,
			.channel = channel_id,
			.data = Arena_PushMemoryAligned(discord->events_output_arena, data, data_length, 8),
			.data_length = data_length,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnMessage -- %llu\t%hhu\t%p\t%u\n", peer_id, channel_id, data, data_length);
}

internal void
PlsDiscord_EventOnRouteUpdate_(void* event_data, const char* route_data)
{
	PlsDiscord_Client* discord = event_data;
	
	uintsize length = 0;
	{
		const char* p = route_data;
		while (*p++);
		
		length = p - route_data;
		Assert(length <= UINT32_MAX);
	}
	
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnRouteUpdate,
		.on_route_update = {
			.route_data = Arena_PushMemoryAligned(discord->events_output_arena, route_data, length, 1),
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnRouteUpdate -- %s\n", route_data);
}

internal void
PlsDiscord_EventOnToggle_(void* event_data, bool locked)
{
	PlsDiscord_Client* discord = event_data;
	PlsDiscord_Event event = {
		.kind = PlsDiscord_EventKind_OnToggle,
		.on_toggle = {
			.locked = locked,
		},
	};
	
	PlsDiscord_PushEvent_(discord, &event);
	//Platform_DebugLog("Discord: OnToggle -- %s\n", locked ? "true" : "false");
}

//~ API
internal bool
PlsDiscord_Init(int64 appid, PlsDiscord_Client* discord)
{
	Trace();
	
	// Events
	static struct IDiscordUserEvents user_events = {
		.on_current_user_update = PlsDiscord_EventOnCurrentUserUpdate_,
	};
	
	static struct IDiscordActivityEvents activity_events = {
		.on_activity_join = PlsDiscord_EventOnActivityJoin_,
		.on_activity_spectate = PlsDiscord_EventOnActivitySpectate_,
		.on_activity_join_request = PlsDiscord_EventOnActivityJoinRequest_,
		.on_activity_invite = PlsDiscord_EventOnActivityInvite_,
	};
	
	static struct IDiscordRelationshipEvents relationship_events = {
		.on_refresh = PlsDiscord_EventOnRefresh_,
		.on_relationship_update = PlsDiscord_EventOnRelationshipUpdate_,
	};
	
	static struct IDiscordLobbyEvents lobby_events = {
		.on_lobby_update = PlsDiscord_EventOnLobbyUpdate_,
        .on_lobby_delete = PlsDiscord_EventOnLobbyDelete_,
        .on_member_connect = PlsDiscord_EventOnMemberConnect_,
        .on_member_update = PlsDiscord_EventOnMemberUpdate_,
        .on_member_disconnect = PlsDiscord_EventOnMemberDisconnect_,
        .on_lobby_message = PlsDiscord_EventOnLobbyMessage_,
        .on_speaking = PlsDiscord_EventOnSpeaking_,
        .on_network_message = PlsDiscord_EventOnNetworkMessage_,
	};
	
	static struct IDiscordNetworkEvents network_events = {
		.on_message = PlsDiscord_EventOnMessage_,
		.on_route_update = PlsDiscord_EventOnRouteUpdate_,
	};
	
	static struct IDiscordOverlayEvents overlay_events = {
		.on_toggle = PlsDiscord_EventOnToggle_,
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
	discord->network = discord->core->get_network_manager(discord->core);
	discord->overlays = discord->core->get_overlay_manager(discord->core);
	discord->images = discord->core->get_image_manager(discord->core);
	
	discord->appid = appid;
	discord->connected = true;
	MemSet(&discord->lobby, 0, sizeof(discord->lobby));
	
	discord->activities->update_activity(discord->activities, &discord->activity, discord, PlsDiscord_CallbackUpdateActivity_);
	
	return true;
}

internal void
PlsDiscord_UpdateActivity(PlsDiscord_Client* discord)
{
	if (discord->core)
		discord->activities->update_activity(discord->activities, &discord->activity, discord, PlsDiscord_CallbackUpdateActivity_);
}

internal void
PlsDiscord_EarlyUpdate(PlsDiscord_Client* discord, Arena* events_output_arena, PlsDiscord_Event** out_list)
{
	Trace();
	*out_list = NULL;
	
	if (discord->core)
	{
		discord->events_output_arena = events_output_arena;
		discord->event_list = out_list;
		
		discord->core->run_callbacks(discord->core);
		
		discord->events_output_arena = NULL;
		discord->event_list = NULL;
	}
}

internal void
PlsDiscord_LateUpdate(PlsDiscord_Client* discord)
{
	Trace();
	
	if (discord->core)
	{
		if (discord->lobbies)
			discord->lobbies->flush_network(discord->lobbies);
		if (discord->network)
			discord->network->flush(discord->network);
	}
}

internal bool
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
	
	result = transaction->set_capacity(transaction, 5);
	if (result != DiscordResult_Ok)
		return false;
	
	// NOTE(ljre): Actually Create Lobby
	discord->lobbies->create_lobby(discord->lobbies, transaction, discord, PlsDiscord_CallbackCreateLobby_);
	return true;
}

internal void
PlsDiscord_DeleteLobby(PlsDiscord_Client* discord)
{
	Assert(discord->core);
	
	if (discord->lobby.id)
		discord->lobbies->delete_lobby(discord->lobbies, discord->lobby.id, discord, PlsDiscord_CallbackDeleteLobby_);
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
