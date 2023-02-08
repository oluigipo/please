#ifndef API_STEAM_H
#define API_STEAM_H

API bool S_Init(uint64 desired_app_id);
API void S_Update(void);
API void S_Deinit(void);

API String S_GetUserNickname(void);

#endif //API_STEAM_H
