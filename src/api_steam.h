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

#endif //API_STEAM_H
