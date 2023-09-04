#ifndef API_STEAM_H
#define API_STEAM_H

struct STM_ImageHandle { int32 id; } typedef STM_ImageHandle;

API bool STM_Init(uint64 desired_app_id);
API void STM_Deinit(void);
API void STM_Update(void);
API bool STM_IsInitialized(void);

API String STM_GetUserNickname(void);
API bool STM_GetUserLargeAvatar(STM_ImageHandle* out_handle);

API bool STM_ImageGetSize(STM_ImageHandle handle, int32* out_width, int32* out_height);
API bool STM_ImageGetRgba(STM_ImageHandle handle, void* out_buffer, uintsize buffer_size);

API bool STM_AchievementIsUnlocked(Arena* scratch_arena, String name, bool* out_is_unlocked, uint64* out_unlock_date);
API bool STM_AchievementGetUnlockPercentage(Arena* scratch_arena, String name, float32* out_percentage);
API bool STM_AchievementGetInfo(Arena* scratch_arena, String name, String* out_local_name, String* out_local_desc, bool* out_is_hidden);
API bool STM_AchievementGetIcon(Arena* scratch_arena, String name, STM_ImageHandle* out_handle);
API bool STM_AchievementUnlock(Arena* scratch_arena, String name);

API bool STM_ShowAchievementProgress(Arena* scratch_arena, String name, uint32 progress, uint32 max_progress);
API bool STM_UploadNewStats(void);

#endif //API_STEAM_H
