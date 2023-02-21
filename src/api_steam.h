#ifndef API_STEAM_H
#define API_STEAM_H

struct S_ImageHandle { int32 id; } typedef S_ImageHandle;

API bool S_Init(uint64 desired_app_id);
API void S_Deinit(void);
API void S_Update(void);
API bool S_IsInitialized(void);

API String S_GetUserNickname(void);
API bool S_GetUserLargeAvatar(S_ImageHandle* out_handle);

API bool S_ImageGetSize(S_ImageHandle handle, int32* out_width, int32* out_height);
API bool S_ImageGetRgba(S_ImageHandle handle, void* out_buffer, uintsize buffer_size);

API bool S_AchievementIsUnlocked(Arena* scratch_arena, String name, bool* out_is_unlocked, uint64* out_unlock_date);
API bool S_AchievementGetUnlockPercentage(Arena* scratch_arena, String name, float32* out_percentage);
API bool S_AchievementGetInfo(Arena* scratch_arena, String name, String* out_local_name, String* out_local_desc, bool* out_is_hidden);
API bool S_AchievementGetIcon(Arena* scratch_arena, String name, S_ImageHandle* out_handle);
API bool S_AchievementUnlock(Arena* scratch_arena, String name);

API bool S_ShowAchievementProgress(Arena* scratch_arena, String name, uint32 progress, uint32 max_progress);
API bool S_UploadNewStats(void);

#endif //API_STEAM_H
