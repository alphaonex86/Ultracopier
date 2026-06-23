/*
 * fs_hook.cpp -- Windows IOCP-lane filesystem-condition hook (SKELETON).
 *
 * This is the Windows counterpart of ../linux/fs_preload.c. On Linux we use
 * LD_PRELOAD to interpose libc symbols; Windows has no LD_PRELOAD, so we inject
 * THIS DLL into the ultracopier process and hook the Win32 API entry points that
 * the IOCP data-plane + scan/metadata path use. The scenario grammar is IDENTICAL
 * to the Linux shim and is read from the SAME env var: UC_FS_SCENARIO.
 *
 * Mapping of the Linux libc shim onto Win32 (what each rule hooks here):
 *
 *   Linux libc          Win32 equivalent we hook              rule(s) that fire
 *   ----------------    ----------------------------------    -----------------------
 *   open/open64/openat  CreateFileW (+ CreateFileA fallback)  openfail
 *   read/pread          ReadFile / ReadFileEx                 slow
 *   write/pwrite        WriteFile / WriteFileEx               efail, slow, shortwrite
 *   utimensat           SetFileTime                           efail
 *   (fs control/ioctl)  DeviceIoControl                       efail (e.g. sparse/trim)
 *   stat/lstat/statx    GetFileAttributesExW / GetFileInfo... statfail
 *
 * ===================== SCOPE / LIMITATION (same as Linux) ======================
 * This hook targets the IOCP backend (the Windows data plane: overlapped
 * ReadFile/WriteFile against handles opened with FILE_FLAG_OVERLAPPED, drained on
 * a completion port) AND the shared scan/metadata path (CreateFileW for handles,
 * GetFileAttributesEx... for metadata).
 *
 * It does NOT model io_uring (Linux-only). On Windows there is no io_uring lane,
 * so that limitation is moot here -- but note that if ultracopier ever issues I/O
 * through an alternative path that bypasses kernel32!ReadFile/WriteFile (e.g. a
 * direct ntdll!NtReadFile/NtWriteFile call, or RIO), you must hook THOSE syscalls
 * instead/as well. The TODOs below call this out.
 * ===============================================================================
 *
 * BUILD / INJECTION (TODO -- not wired up yet):
 *   - Recommended hooking engine: MinHook (https://github.com/TsudaKageyu/minhook),
 *     an MIT-licensed inline-hook (trampoline) library. Link it statically.
 *   - Injection options (pick one; the harness's Windows lane will choose):
 *       (a) AppInit_DLLs / IFEO -- fragile, global; avoid.
 *       (b) CreateRemoteThread + LoadLibraryW into a freshly-spawned, SUSPENDED
 *           ultracopier process (RECOMMENDED -- scoped to the one test process).
 *       (c) Detours' DetourCreateProcessWithDllEx (clean but adds a dependency).
 *   - This file intentionally does NOT compile yet: the MinHook include + the
 *     real GetProcAddress wiring are left as TODOs so the design is reviewable
 *     before committing to the dependency. See every "TODO" marker below.
 */

#ifdef _WIN32

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cwchar>

// TODO(build): #include "MinHook.h"  // and link minhook.lib

// =====================================================================
// Scenario parsing -- mirror of the Linux shim's uc_parse(), wide-char aware.
// =====================================================================

enum UcVerb {
    UC_NONE = 0,
    UC_EFAIL,        // WriteFile/SetFileTime/DeviceIoControl fail on matching path
    UC_OPENFAIL,     // CreateFileW fails on matching path
    UC_SLOW,         // sleep ms in ReadFile/WriteFile
    UC_SHORTWRITE,   // WriteFile reports fewer bytes written on matching path
    UC_STATFAIL      // GetFileAttributesEx fails on matching path
};

struct UcRule {
    UcVerb verb = UC_NONE;
    wchar_t arg[512] = {0};   // path substring to match (case-insensitive), or empty
    long    num = 0;          // numeric arg (ms for slow)
};

static const int UC_MAX_RULES = 32;
static UcRule    g_rules[UC_MAX_RULES];
static int       g_ruleCount = 0;
static bool      g_parsed = false;

// Map a handle back to the path it was opened with, so Write/SetFileTime can
// match by path. TODO: replace this fixed array with a CRITICAL_SECTION-guarded
// std::unordered_map<HANDLE,std::wstring> -- handles are not small integers on
// Windows, so we cannot index by them like the Linux fd table does.
struct HandlePath { HANDLE h = nullptr; wchar_t path[512] = {0}; };
static const int UC_MAX_HANDLES = 4096;
static HandlePath g_handles[UC_MAX_HANDLES];
static CRITICAL_SECTION g_handlesLock;
static bool g_handlesLockInit = false;

static void ucParse()
{
    if (g_parsed)
        return;
    g_parsed = true;

    wchar_t env[8192];
    DWORD n = GetEnvironmentVariableW(L"UC_FS_SCENARIO", env, (DWORD)(sizeof(env)/sizeof(env[0])));
    if (n == 0 || env[0] == L'\0')
        return;   // pure pass-through

    wchar_t *ctx = nullptr;
    for (wchar_t *tok = wcstok_s(env, L",", &ctx);
         tok != nullptr && g_ruleCount < UC_MAX_RULES;
         tok = wcstok_s(nullptr, L",", &ctx))
    {
        while (*tok == L' ')
            tok++;
        wchar_t *colon = wcschr(tok, L':');
        const wchar_t *arg = L"";
        if (colon) { *colon = L'\0'; arg = colon + 1; }

        UcRule &r = g_rules[g_ruleCount];
        if      (!wcscmp(tok, L"efail"))      { r.verb = UC_EFAIL;      wcsncpy_s(r.arg, arg, _TRUNCATE); }
        else if (!wcscmp(tok, L"openfail"))   { r.verb = UC_OPENFAIL;   wcsncpy_s(r.arg, arg, _TRUNCATE); }
        else if (!wcscmp(tok, L"slow"))       { r.verb = UC_SLOW;       r.num = wcstol(arg, nullptr, 10); }
        else if (!wcscmp(tok, L"shortwrite")) { r.verb = UC_SHORTWRITE; wcsncpy_s(r.arg, arg, _TRUNCATE); }
        else if (!wcscmp(tok, L"statfail"))   { r.verb = UC_STATFAIL;   wcsncpy_s(r.arg, arg, _TRUNCATE); }
        else continue;   // unknown verb: ignore
        g_ruleCount++;
    }
}

// Case-insensitive substring match (paths on Windows are case-insensitive).
static bool ucPathContains(const wchar_t *hay, const wchar_t *needle)
{
    if (needle[0] == L'\0')
        return true;
    if (!hay)
        return false;
    size_t nl = wcslen(needle);
    for (const wchar_t *p = hay; *p; ++p) {
        if (_wcsnicmp(p, needle, nl) == 0)
            return true;
    }
    return false;
}

static const UcRule *ucMatch(UcVerb verb, const wchar_t *path)
{
    ucParse();
    for (int i = 0; i < g_ruleCount; ++i)
        if (g_rules[i].verb == verb && ucPathContains(path, g_rules[i].arg))
            return &g_rules[i];
    return nullptr;
}

static const UcRule *ucSlow()
{
    ucParse();
    for (int i = 0; i < g_ruleCount; ++i)
        if (g_rules[i].verb == UC_SLOW)
            return &g_rules[i];
    return nullptr;
}

static void ucRememberHandle(HANDLE h, const wchar_t *path)
{
    if (h == INVALID_HANDLE_VALUE || !path)
        return;
    EnterCriticalSection(&g_handlesLock);
    for (int i = 0; i < UC_MAX_HANDLES; ++i) {
        if (g_handles[i].h == nullptr) {
            g_handles[i].h = h;
            wcsncpy_s(g_handles[i].path, path, _TRUNCATE);
            break;
        }
    }
    LeaveCriticalSection(&g_handlesLock);
}

static const wchar_t *ucHandlePath(HANDLE h)
{
    // Caller must hold no lock; returns a pointer valid until the handle is freed.
    for (int i = 0; i < UC_MAX_HANDLES; ++i)
        if (g_handles[i].h == h)
            return g_handles[i].path;
    return nullptr;
}

static void ucForgetHandle(HANDLE h)
{
    EnterCriticalSection(&g_handlesLock);
    for (int i = 0; i < UC_MAX_HANDLES; ++i) {
        if (g_handles[i].h == h) {
            g_handles[i].h = nullptr;
            g_handles[i].path[0] = L'\0';
            break;
        }
    }
    LeaveCriticalSection(&g_handlesLock);
}

// =====================================================================
// Real function pointers (filled in by MinHook when the hooks are created).
// =====================================================================

typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
                                       DWORD, DWORD, HANDLE);
typedef BOOL   (WINAPI *ReadFile_t)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL   (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL   (WINAPI *SetFileTime_t)(HANDLE, const FILETIME*, const FILETIME*, const FILETIME*);
typedef BOOL   (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD,
                                           LPDWORD, LPOVERLAPPED);
typedef BOOL   (WINAPI *GetFileAttributesExW_t)(LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
typedef BOOL   (WINAPI *CloseHandle_t)(HANDLE);

static CreateFileW_t            real_CreateFileW            = nullptr;
static ReadFile_t              real_ReadFile               = nullptr;
static WriteFile_t             real_WriteFile              = nullptr;
static SetFileTime_t           real_SetFileTime            = nullptr;
static DeviceIoControl_t       real_DeviceIoControl        = nullptr;
static GetFileAttributesExW_t  real_GetFileAttributesExW   = nullptr;
static CloseHandle_t           real_CloseHandle            = nullptr;

// =====================================================================
// Hook implementations.
// =====================================================================

static HANDLE WINAPI hook_CreateFileW(LPCWSTR name, DWORD access, DWORD share,
                                      LPSECURITY_ATTRIBUTES sa, DWORD disp,
                                      DWORD flags, HANDLE tmpl)
{
    if (ucMatch(UC_OPENFAIL, name)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }
    HANDLE h = real_CreateFileW(name, access, share, sa, disp, flags, tmpl);
    if (h != INVALID_HANDLE_VALUE)
        ucRememberHandle(h, name);
    return h;
}

static BOOL WINAPI hook_ReadFile(HANDLE h, LPVOID buf, DWORD len, LPDWORD got, LPOVERLAPPED ov)
{
    const UcRule *slow = ucSlow();
    if (slow)
        Sleep((DWORD)slow->num);
    // NOTE: for OVERLAPPED (IOCP) reads the byte count is reported via the
    // completion packet (GetQueuedCompletionStatus), not *got. Slowing the
    // submit call is still observable as back-pressure; failing a read mid-IOCP
    // would require also faulting the completion -- see TODO below.
    return real_ReadFile(h, buf, len, got, ov);
}

static BOOL WINAPI hook_WriteFile(HANDLE h, LPCVOID buf, DWORD len, LPDWORD put, LPOVERLAPPED ov)
{
    const wchar_t *path = ucHandlePath(h);
    const UcRule *slow = ucSlow();
    if (slow)
        Sleep((DWORD)slow->num);
    if (path) {
        if (ucMatch(UC_EFAIL, path)) {
            SetLastError(ERROR_IO_DEVICE);   // ~ EIO
            if (put) *put = 0;
            return FALSE;
        }
        const UcRule *sw = ucMatch(UC_SHORTWRITE, path);
        if (sw && len > 1) {
            DWORD half = len / 2; if (half < 1) half = 1;
            // For synchronous writes this reports a short count. For OVERLAPPED
            // writes the real short count surfaces in the completion packet;
            // TODO: to short-write the IOCP lane, intercept the completion in
            // GetQueuedCompletionStatus (hook that too) and rewrite dwNumberOfBytes.
            return real_WriteFile(h, buf, half, put, ov);
        }
    }
    return real_WriteFile(h, buf, len, put, ov);
}

static BOOL WINAPI hook_SetFileTime(HANDLE h, const FILETIME *c, const FILETIME *a, const FILETIME *m)
{
    const wchar_t *path = ucHandlePath(h);
    if (path && ucMatch(UC_EFAIL, path)) {
        SetLastError(ERROR_IO_DEVICE);
        return FALSE;
    }
    return real_SetFileTime(h, c, a, m);
}

static BOOL WINAPI hook_DeviceIoControl(HANDLE h, DWORD code, LPVOID inb, DWORD inl,
                                        LPVOID outb, DWORD outl, LPDWORD ret, LPOVERLAPPED ov)
{
    const wchar_t *path = ucHandlePath(h);
    if (path && ucMatch(UC_EFAIL, path)) {
        SetLastError(ERROR_IO_DEVICE);
        return FALSE;
    }
    return real_DeviceIoControl(h, code, inb, inl, outb, outl, ret, ov);
}

static BOOL WINAPI hook_GetFileAttributesExW(LPCWSTR name, GET_FILEEX_INFO_LEVELS lvl, LPVOID info)
{
    if (ucMatch(UC_STATFAIL, name)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return real_GetFileAttributesExW(name, lvl, info);
}

static BOOL WINAPI hook_CloseHandle(HANDLE h)
{
    ucForgetHandle(h);
    return real_CloseHandle(h);
}

// =====================================================================
// Hook table + install/remove.
// =====================================================================

struct HookEntry {
    const wchar_t *module;   // L"kernel32.dll" etc.
    const char    *symbol;   // exported name
    void          *detour;   // our hook
    void         **trampoline; // receives the real function pointer
};

static HookEntry g_hookTable[] = {
    { L"kernel32.dll", "CreateFileW",           (void*)&hook_CreateFileW,           (void**)&real_CreateFileW },
    { L"kernel32.dll", "ReadFile",              (void*)&hook_ReadFile,              (void**)&real_ReadFile },
    { L"kernel32.dll", "WriteFile",             (void*)&hook_WriteFile,             (void**)&real_WriteFile },
    { L"kernel32.dll", "SetFileTime",           (void*)&hook_SetFileTime,           (void**)&real_SetFileTime },
    { L"kernel32.dll", "DeviceIoControl",       (void*)&hook_DeviceIoControl,       (void**)&real_DeviceIoControl },
    { L"kernel32.dll", "GetFileAttributesExW",  (void*)&hook_GetFileAttributesExW,  (void**)&real_GetFileAttributesExW },
    { L"kernel32.dll", "CloseHandle",           (void*)&hook_CloseHandle,           (void**)&real_CloseHandle },
    // TODO: on some toolchains these live in kernelbase.dll, not kernel32.dll.
    //       MinHook's MH_CreateHookApi resolves the real forwarded target, so
    //       prefer MH_CreateHookApi(L"kernel32", "CreateFileW", ...) which follows
    //       the API-set/forwarder chain to kernelbase automatically.
    // TODO: if ultracopier bypasses kernel32 and calls ntdll!NtCreateFile /
    //       NtReadFile / NtWriteFile directly, add ntdll entries here too.
};

static bool installHooks()
{
    // TODO(MinHook):
    //   if (MH_Initialize() != MH_OK) return false;
    //   for (auto &e : g_hookTable) {
    //       if (MH_CreateHookApi(e.module, e.symbol, e.detour, e.trampoline) != MH_OK)
    //           return false;
    //   }
    //   return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
    (void)g_hookTable;
    return false;   // not yet wired -- skeleton only
}

static void removeHooks()
{
    // TODO(MinHook):
    //   MH_DisableHook(MH_ALL_HOOKS);
    //   MH_Uninitialize();
}

// =====================================================================
// DLL entry point.
// =====================================================================

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        if (!g_handlesLockInit) {
            InitializeCriticalSection(&g_handlesLock);
            g_handlesLockInit = true;
        }
        ucParse();
        installHooks();   // returns false until MinHook is wired (TODO)
        break;
    case DLL_PROCESS_DETACH:
        removeHooks();
        if (g_handlesLockInit) {
            DeleteCriticalSection(&g_handlesLock);
            g_handlesLockInit = false;
        }
        break;
    default:
        break;
    }
    return TRUE;
}

#endif // _WIN32
