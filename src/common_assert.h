#ifndef COMMON_ASSERT_H
#define COMMON_ASSERT_H

#ifndef Assert_IsDebuggerPresent_
#   ifdef _WIN32
externC_ __declspec(dllimport) int __stdcall IsDebuggerPresent(void);
#       define Assert_IsDebuggerPresent_() IsDebuggerPresent()
#   else
#       define Assert_IsDebuggerPresent_() true
#   endif
#endif

//- NOTE(ljre): SafeAssert -- always present, side-effects allowed, memory safety assert
#ifndef SafeAssert_OnFailure
#   define SafeAssert_OnFailure(expr, file, line, func) Unreachable()
#endif

#define SafeAssert(...) do { \
bool not_ok = !(__VA_ARGS__); \
if (Unlikely(not_ok)) { \
if (Assert_IsDebuggerPresent_()) \
Debugbreak(); \
SafeAssert_OnFailure(#__VA_ARGS__, __FILE__, __LINE__, __func__); \
} \
} while (0)

//- NOTE(ljre): Assert -- set to Assume() on release, logic safety assert
#ifndef COMMON_DEBUG
#   define Assert(...) Assume(__VA_ARGS__)
#else //COMMON_DEBUG
#   ifndef Assert_OnFailure
#       define Assert_OnFailure(expr, file, line, func) Unreachable()
#   endif

#   define Assert(...) do { \
if (!(__VA_ARGS__)) { \
if (Assert_IsDebuggerPresent_()) \
Debugbreak(); \
Assert_OnFailure(#__VA_ARGS__, __FILE__, __LINE__, __func__); \
} \
} while (0)
#endif //COMMON_RELEASE

#endif //COMMON_ASSERT_H
