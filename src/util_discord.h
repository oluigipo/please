#ifndef UTIL_DISCORD_H
#define UTIL_DISCORD_H

// NOTE(ljre): If you want to use the 3.1.0 DLL for x86, you may want to change the calling convention
//             of all the callbacks, managers, and UDiscord_CreateProc to __stdcall.
//
//             Newer versions still do use __stdcall, but the header is updated to use the correct callconv.
#pragma pack(push, 8)
#	include <discord_game_sdk.h>
#pragma pack(pop)

#ifndef DISCORD_CALLBACK
#	define DISCORD_CALLBACK
#endif

#ifndef DISCORD_API
#	define DISCORD_API
#endif

enum EDiscordResult typedef DISCORD_API UDiscord_CreateProc_(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

enum UDiscord_EventKind
{
	UDiscord_EventKind_Null = 0,
	
	// NOTE(ljre): From callbacks
	UDiscord_EventKind_UpdateActivity,
	UDiscord_EventKind_CreateLobby,
	UDiscord_EventKind_DeleteLobby,
	UDiscord_EventKind_ConnectLobbyWithActivitySecret,
	
	// NOTE(ljre): From events
	UDiscord_EventKind_OnLobbyUpdate,
	UDiscord_EventKind_OnLobbyDelete,
	UDiscord_EventKind_OnLobbyMessage,
	UDiscord_EventKind_OnMemberConnect,
	UDiscord_EventKind_OnMemberUpdate,
	UDiscord_EventKind_OnMemberDisconnect,
	UDiscord_EventKind_OnSpeaking,
	UDiscord_EventKind_OnNetworkMessage,
	UDiscord_EventKind_OnActivityJoin,
	UDiscord_EventKind_OnActivitySpectate,
	UDiscord_EventKind_OnActivityJoinRequest,
	UDiscord_EventKind_OnActivityInvite,
	UDiscord_EventKind_OnCurrentUserUpdate,
	UDiscord_EventKind_OnRelationshipUpdate,
	UDiscord_EventKind_OnRefresh,
	UDiscord_EventKind_OnToggle,
	UDiscord_EventKind_OnMessage,
	UDiscord_EventKind_OnRouteUpdate,
}
typedef UDiscord_EventKind;

struct UDiscord_Event typedef UDiscord_Event;
struct UDiscord_Event
{
	UDiscord_EventKind kind;
	UDiscord_Event* next;
	
	union
	{
		struct { enum EDiscordResult result; struct DiscordLobby lobby; } create_lobby;
		struct { enum EDiscordResult result; } delete_lobby;
		struct { enum EDiscordResult result; } update_activity;
		struct { enum EDiscordResult result; struct DiscordLobby lobby; } connect_lobby_with_activity_secret;
		
		struct { int64 lobby_id; struct DiscordLobby lobby; } on_lobby_update;
		struct { int64 lobby_id; uint32 reason; } on_lobby_delete;
		struct { int64 lobby_id; int64 user_id; uint8* data; uint32 data_length; } on_lobby_message;
		
		struct { int64 lobby_id; int64 user_id; struct DiscordUser user; } on_member_connect;
		struct { int64 lobby_id; int64 user_id; struct DiscordUser user;} on_member_update;
		struct { int64 lobby_id; int64 user_id; } on_member_disconnect;
		struct { int64 lobby_id; int64 user_id; bool state; } on_speaking;
		
		struct { int64 lobby_id; int64 user_id; uint8 channel; uint8* data; uint32 data_length; } on_network_message;
		struct { const char* route_data; } on_route_update;
		
		struct { char* secret; } on_activity_join;
		struct { char* secret; } on_activity_spectate;
		struct { struct DiscordUser user; } on_activity_join_request;
		struct { enum EDiscordActivityActionType type; struct DiscordUser user; struct DiscordActivity activity; } on_activity_invite;
		
		struct { uint64 peer_id; uint8 channel; uint8* data; uint32 data_length; } on_message;
		struct { bool locked; } on_toggle;
		struct { enum EDiscordResult result; struct DiscordUser state; } on_current_user_update;
		struct { struct DiscordRelationship state; } on_relationship_update;
	};
};

struct UDiscord_Client
{
	int64 appid;
	bool8 connected;
	
	UDiscord_CreateProc_* create;
	OS_LibraryHandle library;
	
	struct DiscordActivity activity;
	struct DiscordUser user;
	struct DiscordLobby lobby;
	
	Arena* events_output_arena;
	UDiscord_Event** event_list;
	
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
typedef UDiscord_Client;

enum EDiscordResult static UDiscord_UpdateLobbyActivity(UDiscord_Client* discord);

//~ NOTE(ljre): Helpers
static void
UDiscord_PushEvent_(UDiscord_Client* discord, UDiscord_Event* event)
{
	UDiscord_Event* p = ArenaPushStructData(discord->events_output_arena, UDiscord_Event, event);
	*discord->event_list = p;
	discord->event_list = &p->next;
}

//~ NOTE(ljre): Callbacks
static DISCORD_CALLBACK void
UDiscord_CallbackUpdateActivity_(void* event_data, enum EDiscordResult result)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_UpdateActivity,
		.update_activity = { result },
	};
	
	UDiscord_PushEvent_(discord, &event);
}

static DISCORD_CALLBACK void
UDiscord_CallbackCreateLobby_(void* event_data, enum EDiscordResult result, struct DiscordLobby* lobby)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_CreateLobby,
		.create_lobby = {
			.result = result,
			.lobby = *lobby,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	
	if (result != DiscordResult_Ok)
		return;
	
	discord->lobby = *lobby;
	UDiscord_UpdateLobbyActivity(discord);
	OS_DebugLog("Created Lobby!\n");
}

static DISCORD_CALLBACK void
UDiscord_CallbackDeleteLobby_(void* event_data, enum EDiscordResult result)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_DeleteLobby,
		.delete_lobby = {
			.result = result,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	
	if (result != DiscordResult_Ok)
		MemoryZero(&discord->lobby, sizeof(discord->lobby));
}

static DISCORD_CALLBACK void
UDiscord_CallbackConnectLobbyWithActivitySecret_(void* event_data, enum EDiscordResult result, struct DiscordLobby* lobby)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_ConnectLobbyWithActivitySecret,
		.connect_lobby_with_activity_secret = {
			.result = result,
			.lobby = *lobby,
		},
	};
	
	discord->lobby = *lobby;
	UDiscord_PushEvent_(discord, &event);
}

//~ NOTE(ljre): Events
static DISCORD_CALLBACK void
UDiscord_EventOnCurrentUserUpdate_(void* data)
{
	struct DiscordUser user = { 0 };
	enum EDiscordResult result;
	UDiscord_Client* discord = data;
	
	result = discord->users->get_current_user(discord->users, &user);
	
	if (result == DiscordResult_Ok)
		discord->user = user;
	
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnCurrentUserUpdate,
		.on_current_user_update = {
			.result = result,
			.state = user,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("User ID: %I\nUsername: %s\nDiscriminator: %s\n", discord->user.id, discord->user.username, discord->user.discriminator);
}

static DISCORD_CALLBACK void
UDiscord_EventOnLobbyUpdate_(void* event_data, int64 lobby_id)
{
	UDiscord_Client* discord = event_data;
	struct DiscordLobby lobby;
	enum EDiscordResult result = discord->lobbies->get_lobby(discord->lobbies, lobby_id, &lobby);
	
	if (result == DiscordResult_Ok)
	{
		UDiscord_Event event = {
			.kind = UDiscord_EventKind_OnLobbyUpdate,
			.on_lobby_update = {
				.lobby_id = lobby_id,
				.lobby = lobby,
			},
		};
		
		UDiscord_PushEvent_(discord, &event);
		discord->lobby = lobby;
	}
	else
		OS_DebugLog("Discord: OnMemberUpdate failed to fetch user data\n");
	
	//OS_DebugLog("Discord: OnLobbyUpdate -- %I\n", lobby_id);
}

static DISCORD_CALLBACK void
UDiscord_EventOnLobbyDelete_(void* event_data, int64 lobby_id, uint32 reason)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnLobbyDelete,
		.on_lobby_delete = {
			.lobby_id = lobby_id,
			.reason = reason,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	MemoryZero(&discord->lobby, sizeof(discord->lobby));
	//OS_DebugLog("Discord: OnLobbyDelete -- %I\t%u\n", lobby_id, reason);
}

static DISCORD_CALLBACK void
UDiscord_EventOnMemberConnect_(void* event_data, int64 lobby_id, int64 user_id)
{
	UDiscord_Client* discord = event_data;
	struct DiscordUser user;
	enum EDiscordResult result = discord->lobbies->get_member_user(discord->lobbies, lobby_id, user_id, &user);
	
	if (result == DiscordResult_Ok)
	{
		UDiscord_Event event = {
			.kind = UDiscord_EventKind_OnMemberConnect,
			.on_member_connect = {
				.lobby_id = lobby_id,
				.user_id = user_id,
				.user = user,
			},
		};
		
		UDiscord_PushEvent_(discord, &event);
		//OS_DebugLog("Discord: OnMemberConnect -- %I\t%I\n", lobby_id, user_id);
	}
	else
		OS_DebugLog("Discord: OnMemberConnect failed to fetch user data\n");
}

static DISCORD_CALLBACK void
UDiscord_EventOnMemberUpdate_(void* event_data, int64 lobby_id, int64 user_id)
{
	UDiscord_Client* discord = event_data;
	struct DiscordUser user;
	enum EDiscordResult result = discord->lobbies->get_member_user(discord->lobbies, lobby_id, user_id, &user);
	
	if (result == DiscordResult_Ok)
	{
		UDiscord_Event event = {
			.kind = UDiscord_EventKind_OnMemberUpdate,
			.on_member_update = {
				.lobby_id = lobby_id,
				.user_id = user_id,
				.user = user,
			},
		};
		
		UDiscord_PushEvent_(discord, &event);
		//OS_DebugLog("Discord: OnMemberUpdate -- %I\t%I\n", lobby_id, user_id);
	}
	else
		OS_DebugLog("Discord: OnMemberUpdate failed to fetch user data\n");
}

static DISCORD_CALLBACK void
UDiscord_EventOnMemberDisconnect_(void* event_data, int64 lobby_id, int64 user_id)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnMemberDisconnect,
		.on_member_disconnect = {
			.lobby_id = lobby_id,
			.user_id = user_id,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnMemberDisconnect -- %I\t%I\n", lobby_id, user_id);
}

static DISCORD_CALLBACK void
UDiscord_EventOnLobbyMessage_(void* event_data, int64 lobby_id, int64 user_id, uint8* data, uint32 data_length)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnLobbyMessage,
		.on_lobby_message = {
			.lobby_id = lobby_id,
			.user_id = user_id,
			.data = ArenaPushMemoryAligned(discord->events_output_arena, data, data_length, 8),
			.data_length = data_length,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnLobbyMessage -- %I\t%I\t%p\t%u\n", lobby_id, user_id, data, data_length);
}

static DISCORD_CALLBACK void
UDiscord_EventOnSpeaking_(void* event_data, int64 lobby_id, int64 user_id, bool speaking)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnSpeaking,
		.on_speaking = {
			.lobby_id = lobby_id,
			.user_id = user_id,
			.state = speaking,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnSpeaking -- %I\t%I\t%i\n", lobby_id, user_id, speaking);
}

static DISCORD_CALLBACK void
UDiscord_EventOnNetworkMessage_(void* event_data, int64 lobby_id, int64 user_id, uint8 channel_id, uint8* data, uint32 data_length)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnNetworkMessage,
		.on_network_message = {
			.lobby_id = lobby_id,
			.user_id = user_id,
			.channel = channel_id,
			.data = ArenaPushMemoryAligned(discord->events_output_arena, data, data_length, 8),
			.data_length = data_length,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnNetworkMessage -- %I\t%I\t%i\t%p\t%u\n", lobby_id, user_id, channel_id, data, data_length);
}

static DISCORD_CALLBACK void
UDiscord_EventOnActivityJoin_(void* event_data, const char* secret)
{
	UDiscord_Client* discord = event_data;
	
	uintsize length = 0;
	{
		const char* p = secret;
		while (*p++);
		
		length = p - secret;
		Assert(length <= UINT32_MAX);
	}
	
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnActivityJoin,
		.on_activity_join = {
			.secret = ArenaPushMemoryAligned(discord->events_output_arena, secret, length, 1),
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnActivityJoin -- %s\n", secret);
}

static DISCORD_CALLBACK void
UDiscord_EventOnActivitySpectate_(void* event_data, const char* secret)
{
	UDiscord_Client* discord = event_data;
	
	uintsize length = 0;
	{
		const char* p = secret;
		while (*p++);
		
		length = p - secret;
		Assert(length <= UINT32_MAX);
	}
	
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnActivitySpectate,
		.on_activity_spectate = {
			.secret = ArenaPushMemoryAligned(discord->events_output_arena, secret, length, 1),
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnActivitySpectate -- %s\n", secret);
}

static DISCORD_CALLBACK void
UDiscord_EventOnActivityJoinRequest_(void* event_data, struct DiscordUser* user)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnActivityJoinRequest,
		.on_activity_join_request = {
			.user = *user
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnActivityJoinRequest -- %I\n", user->id);
}

static DISCORD_CALLBACK void
UDiscord_EventOnActivityInvite_(void* event_data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnActivityInvite,
		.on_activity_invite = {
			.type = type,
			.user = *user,
			.activity = *activity,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnActivityJoinRequest -- %i\t%I\t%i\n", type, user->id, activity->type);
}

static DISCORD_CALLBACK void
UDiscord_EventOnRefresh_(void* event_data)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnRefresh,
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnRefresh -- \n");
}

static DISCORD_CALLBACK void
UDiscord_EventOnRelationshipUpdate_(void* event_data, struct DiscordRelationship* relationship)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnRelationshipUpdate,
		.on_relationship_update = {
			.state = *relationship,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnRelationshipUpdate -- %p\n", relationship);
}

static DISCORD_CALLBACK void
UDiscord_EventOnMessage_(void* event_data, DiscordNetworkPeerId peer_id, uint8 channel_id, uint8* data, uint32 data_length)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnMessage,
		.on_message = {
			.peer_id = peer_id,
			.channel = channel_id,
			.data = ArenaPushMemoryAligned(discord->events_output_arena, data, data_length, 8),
			.data_length = data_length,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnMessage -- %llu\t%hhu\t%p\t%u\n", peer_id, channel_id, data, data_length);
}

static DISCORD_CALLBACK void
UDiscord_EventOnRouteUpdate_(void* event_data, const char* route_data)
{
	UDiscord_Client* discord = event_data;
	
	uintsize length = 0;
	{
		const char* p = route_data;
		while (*p++);
		
		length = p - route_data;
		Assert(length <= UINT32_MAX);
	}
	
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnRouteUpdate,
		.on_route_update = {
			.route_data = ArenaPushMemoryAligned(discord->events_output_arena, route_data, length, 1),
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnRouteUpdate -- %s\n", route_data);
}

static DISCORD_CALLBACK void
UDiscord_EventOnToggle_(void* event_data, bool locked)
{
	UDiscord_Client* discord = event_data;
	UDiscord_Event event = {
		.kind = UDiscord_EventKind_OnToggle,
		.on_toggle = {
			.locked = locked,
		},
	};
	
	UDiscord_PushEvent_(discord, &event);
	//OS_DebugLog("Discord: OnToggle -- %s\n", locked ? "true" : "false");
}

//~ API
static bool
UDiscord_Init(int64 appid, UDiscord_Client* discord)
{
	Trace();
	
	// Events
	static struct IDiscordUserEvents user_events = {
		.on_current_user_update = UDiscord_EventOnCurrentUserUpdate_,
	};
	
	static struct IDiscordActivityEvents activity_events = {
		.on_activity_join = UDiscord_EventOnActivityJoin_,
		.on_activity_spectate = UDiscord_EventOnActivitySpectate_,
		.on_activity_join_request = UDiscord_EventOnActivityJoinRequest_,
		.on_activity_invite = UDiscord_EventOnActivityInvite_,
	};
	
	static struct IDiscordRelationshipEvents relationship_events = {
		.on_refresh = UDiscord_EventOnRefresh_,
		.on_relationship_update = UDiscord_EventOnRelationshipUpdate_,
	};
	
	static struct IDiscordLobbyEvents lobby_events = {
		.on_lobby_update = UDiscord_EventOnLobbyUpdate_,
		.on_lobby_delete = UDiscord_EventOnLobbyDelete_,
		.on_member_connect = UDiscord_EventOnMemberConnect_,
		.on_member_update = UDiscord_EventOnMemberUpdate_,
		.on_member_disconnect = UDiscord_EventOnMemberDisconnect_,
		.on_lobby_message = UDiscord_EventOnLobbyMessage_,
		.on_speaking = UDiscord_EventOnSpeaking_,
		.on_network_message = UDiscord_EventOnNetworkMessage_,
	};
	
	static struct IDiscordNetworkEvents network_events = {
		.on_message = UDiscord_EventOnMessage_,
		.on_route_update = UDiscord_EventOnRouteUpdate_,
	};
	
	static struct IDiscordOverlayEvents overlay_events = {
		.on_toggle = UDiscord_EventOnToggle_,
	};
	
	// Init
	if (!discord->library.ptr)
	{
		discord->library = OS_LoadLibrary(Str("discord_game_sdk"));
		
		if (!discord->library.ptr)
			return false;
	}
	
	if (!discord->create)
	{
		discord->create = OS_LoadSymbol(discord->library, "DiscordCreate");
		
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
	
	SafeAssert(discord->users && discord->achievements && discord->activities && discord->application &&
		discord->lobbies && discord->relationships && discord->network && discord->overlays &&
		discord->images);
	
	discord->appid = appid;
	discord->connected = true;
	MemoryZero(&discord->lobby, sizeof(discord->lobby));
	
	discord->activities->update_activity(discord->activities, &discord->activity, discord, UDiscord_CallbackUpdateActivity_);
	
	return true;
}

static void
UDiscord_UpdateActivity(UDiscord_Client* discord)
{
	if (discord->core)
		discord->activities->update_activity(discord->activities, &discord->activity, discord, UDiscord_CallbackUpdateActivity_);
}

static void
UDiscord_EarlyUpdate(UDiscord_Client* discord, Arena* events_output_arena, UDiscord_Event** out_list)
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

static void
UDiscord_LateUpdate(UDiscord_Client* discord)
{
	Trace();
	
	if (discord->core)
	{
		enum EDiscordResult result;
		
		result = discord->network->flush(discord->network);
		if (result != DiscordResult_Ok)
			OS_DebugLog("Discord: discord->network->flush failed.");
		
		result = discord->lobbies->flush_network(discord->lobbies);
		if (result != DiscordResult_Ok)
			OS_DebugLog("Discord: discord->lobbies->flush_network failed.");
	}
}

static void
UDiscord_Deinit(UDiscord_Client* discord)
{
	if (discord->core)
	{
		discord->core->destroy(discord->core);
		discord->core = NULL;
	}
}

//~ NOTE(ljre): Lobbies
static bool
UDiscord_CreateLobby(UDiscord_Client* discord)
{
	SafeAssert(discord->core);
	
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
	discord->lobbies->create_lobby(discord->lobbies, transaction, discord, UDiscord_CallbackCreateLobby_);
	return true;
}

static enum EDiscordResult
UDiscord_UpdateLobbyActivity(UDiscord_Client* discord)
{
	SafeAssert(discord->core);
	
	DiscordLobbySecret secret;
	int32 member_count;
	enum EDiscordResult result;
	
	result = discord->lobbies->get_lobby_activity_secret(discord->lobbies, discord->lobby.id, &secret);
	if (result != DiscordResult_Ok)
		return result;
	
	result = discord->lobbies->member_count(discord->lobbies, discord->lobby.id, &member_count);
	if (result != DiscordResult_Ok)
		return result;
	
	MemoryCopy(discord->activity.secrets.join, secret, sizeof(discord->lobby.secret));
	StringPrintfBuffer(discord->activity.party.id, sizeof(discord->activity.party.id), "%I", discord->lobby.id);
	discord->activity.party.size.current_size = member_count;
	discord->activity.party.size.max_size = discord->lobby.capacity;
	
	discord->activities->update_activity(discord->activities, &discord->activity, discord, UDiscord_CallbackUpdateActivity_);
	
	return DiscordResult_Ok;
}

static void
UDiscord_DeleteLobby(UDiscord_Client* discord)
{
	SafeAssert(discord->core);
	
	if (discord->lobby.id)
		discord->lobbies->delete_lobby(discord->lobbies, discord->lobby.id, discord, UDiscord_CallbackDeleteLobby_);
}

static void
UDiscord_JoinLobby(UDiscord_Client* discord, DiscordLobbySecret secret)
{
	SafeAssert(discord->core);
	
	discord->lobbies->connect_lobby_with_activity_secret(discord->lobbies, secret, discord, UDiscord_CallbackConnectLobbyWithActivitySecret_);
}

//~ NOTE(ljre): Lobby networking
enum EDiscordResult static
UDiscord_ConnectNetwork(UDiscord_Client* discord)
{
	SafeAssert(discord->core);
	SafeAssert(discord->lobby.id);
	enum EDiscordResult result;
	
	result = discord->lobbies->connect_network(discord->lobbies, discord->lobby.id);
	if (result != DiscordResult_Ok)
		return result;
	
	result = discord->lobbies->open_network_channel(discord->lobbies, discord->lobby.id, 0, false);
	if (result != DiscordResult_Ok)
	{
		discord->lobbies->disconnect_network(discord->lobbies, discord->lobby.id);
		return result;
	}
	
	result = discord->lobbies->open_network_channel(discord->lobbies, discord->lobby.id, 1, true);
	if (result != DiscordResult_Ok)
	{
		discord->lobbies->disconnect_network(discord->lobbies, discord->lobby.id);
		return result;
	}
	
	return DiscordResult_Ok;
}

enum EDiscordResult static
UDiscord_DisconnectNetwork(UDiscord_Client* discord)
{
	SafeAssert(discord->core);
	
	return discord->lobbies->disconnect_network(discord->lobbies, discord->lobby.id);
}

enum EDiscordResult static
UDiscord_SendNetworkMessage(UDiscord_Client* discord, int64 user_id, bool reliable, String memory)
{
	SafeAssert(discord->core);
	SafeAssert(memory.size <= UINT32_MAX);
	
	uint8 channel = (uint8)reliable;
	
	return discord->lobbies->send_network_message(discord->lobbies, discord->lobby.id, user_id, channel, (void*)memory.data, (uint32)memory.size);
}

#endif //UTIL_DISCORD_H
