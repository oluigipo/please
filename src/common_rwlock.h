#ifndef COMMON_RWLOCK_H
#define COMMON_RWLOCK_H

struct RWLock typedef RWLock;

static inline void RWLock_Init(RWLock* lock);
static inline void RWLock_Deinit(RWLock* lock);
static inline void RWLock_LockRead(RWLock* lock);
static inline void RWLock_LockWrite(RWLock* lock);
static inline bool RWLock_TryLockRead(RWLock* lock);
static inline bool RWLock_TryLockWrite(RWLock* lock);
static inline void RWLock_UnlockRead(RWLock* lock);
static inline void RWLock_UnlockWrite(RWLock* lock);

#if defined(_WIN32)

// NOTE(ljre): Definition ABI compatible with Win32's RTL_SRWLOCK in <winnt.h>.
struct _RTL_SRWLOCK;
struct RWLock
{
	void* ptr;
}
typedef RWLock;

// NOTE(ljre): Definition compatible with Win32's RTL_SRWLOCK_INIT in <winnt.h>
static inline void
RWLock_Init(RWLock* lock)
{ Mem_Set(lock, 0, sizeof(*lock)); }

static inline void
RWLock_Deinit(RWLock* lock)
{}

static inline void
RWLock_LockRead(RWLock* lock)
{
	__declspec(dllimport) void __stdcall AcquireSRWLockShared(struct _RTL_SRWLOCK* lock);
	AcquireSRWLockShared((struct _RTL_SRWLOCK*)lock);
}

static inline void
RWLock_LockWrite(RWLock* lock)
{
	__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(struct _RTL_SRWLOCK* lock);
	AcquireSRWLockExclusive((struct _RTL_SRWLOCK*)lock);
}

static inline bool
RWLock_TryLockRead(RWLock* lock)
{
	__declspec(dllimport) unsigned char __stdcall TryAcquireSRWLockShared(struct _RTL_SRWLOCK* lock);
	return TryAcquireSRWLockShared((struct _RTL_SRWLOCK*)lock);
}

static inline bool
RWLock_TryLockWrite(RWLock* lock)
{
	__declspec(dllimport) unsigned char __stdcall TryAcquireSRWLockExclusive(struct _RTL_SRWLOCK* lock);
	return TryAcquireSRWLockExclusive((struct _RTL_SRWLOCK*)lock);
}

static inline void
RWLock_UnlockRead(RWLock* lock)
{
	__declspec(dllimport) void __stdcall ReleaseSRWLockShared(struct _RTL_SRWLOCK* lock);
	ReleaseSRWLockShared((struct _RTL_SRWLOCK*)lock);
}

static inline void
RWLock_UnlockWrite(RWLock* lock)
{
	__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(struct _RTL_SRWLOCK* lock);
	ReleaseSRWLockExclusive((struct _RTL_SRWLOCK*)lock);
}

#elif defined(__linux__)
#   error TODO
#endif

#endif //COMMON_RWLOCK_H
