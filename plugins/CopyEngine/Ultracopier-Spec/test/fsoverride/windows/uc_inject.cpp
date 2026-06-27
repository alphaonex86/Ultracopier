/*
 * uc_inject.exe -- Windows IOCP-test-lane DLL injector (TEST TOOLING ONLY).
 *
 * Spawns a target process and loads fs_hook.dll into it via the standard
 * CreateRemoteThread+LoadLibraryW technique, so UC_FS_SCENARIO fault/corruption
 * injection (the Windows analogue of the Linux LD_PRELOAD shim) applies to
 * ultracopier's IOCP data plane. The target inherits THIS process's environment,
 * so UC_FS_SCENARIO propagates from the harness.
 *
 * This is dual-use (process injection) used here strictly for fault-injection
 * testing of Ultracopier on the user's own test machine, with explicit user
 * authorization. It lives under test/ and is never part of the shipping product.
 *
 * Timing: fs_hook patches the import-address table, which only exists once the
 * loader has resolved imports -- so we DO NOT use CREATE_SUSPENDED (the IAT is
 * unresolved there and the loader would overwrite our patch). We launch normally
 * and inject during ultracopier's Qt-init/scan window, before the transfer's dest
 * writes. The test's checksum-OFF run validates the flip actually landed.
 *
 * Usage: uc_inject.exe <scenario> <dll-path> <target-exe> [args...]
 *   <scenario> is the UC_FS_SCENARIO string (or "-" for none); it is set in this
 *   process's environment so the freshly-spawned target inherits it and fs_hook reads it.
 * Prints "UC_INJECT_PID=<pid>" on success so the harness can monitor/kill it.
 */
#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#include <string>

int wmain(int argc, wchar_t **argv)
{
    if (argc < 4) { fwprintf(stderr, L"usage: uc_inject <scenario> <dll> <exe> [args...]\n"); return 2; }
    const wchar_t *scenario = argv[1];
    const wchar_t *dll      = argv[2];
    const wchar_t *exe      = argv[3];

    // Set UC_FS_SCENARIO so the spawned target (and the injected fs_hook in it) inherit it.
    if (scenario[0] != L'\0' && wcscmp(scenario, L"-") != 0)
        SetEnvironmentVariableW(L"UC_FS_SCENARIO", scenario);

    std::wstring cmd = L"\"";
    cmd += exe;
    cmd += L"\"";
    for (int i = 4; i < argc; ++i) { cmd += L" \""; cmd += argv[i]; cmd += L"\""; }
    std::wstring cmdMut = cmd;   // CreateProcessW may modify the buffer

    STARTUPINFOW si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    // Pin the interactive desktop so the GUI/Qt ultracopier inits like it does under Win32_Process.Create
    // (a child spawned from our console may otherwise inherit a non-interactive desktop and never run the copy).
    si.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    // DETACHED_PROCESS: do NOT let the GUI ultracopier inherit/share this console process's console --
    // a Qt app launched attached to a console can spin its event loop (which wedged _wait_idle: it never
    // goes CPU-idle). Win32_Process.Create (the working direct-launch path) gives it no console either.
    if (!CreateProcessW(exe, &cmdMut[0], NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
        fwprintf(stderr, L"CreateProcess(%ls) failed: %lu\n", exe, GetLastError());
        return 3;
    }

    // Let the loader resolve the target's imports (so its IAT is populated) but inject well before the
    // transfer begins (ultracopier still loads Qt + reads its conf + scans the source first).
    Sleep(120);

    if (wcscmp(scenario, L"NOINJECT") == 0) {
        // DIAGNOSTIC: spawn ultracopier but do NOT inject -> isolates "spawn breaks the copy" from
        // "the DLL load breaks the copy".
        wprintf(L"UC_INJECT_PID=%lu\n", pi.dwProcessId);
        fflush(stdout);
        CloseHandle(pi.hThread);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        return 0;
    }

    int rc = 0;
    SIZE_T bytes = (wcslen(dll) + 1) * sizeof(wchar_t);
    void *remote = VirtualAllocEx(pi.hProcess, NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(pi.hProcess, remote, dll, bytes, NULL)) {
        fwprintf(stderr, L"alloc/write failed: %lu\n", GetLastError());
        rc = 4;
    } else {
        HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
        LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");
        HANDLE th = CreateRemoteThread(pi.hProcess, NULL, 0, loadLib, remote, 0, NULL);
        if (!th) {
            fwprintf(stderr, L"CreateRemoteThread failed: %lu\n", GetLastError());
            rc = 5;
        } else {
            WaitForSingleObject(th, 15000);   // wait for LoadLibraryW -> DllMain -> installHooks()
            DWORD loaded = 0; GetExitCodeThread(th, &loaded);
            if (loaded == 0) { fwprintf(stderr, L"LoadLibraryW returned NULL (fs_hook.dll failed to load)\n"); rc = 6; }
            CloseHandle(th);
        }
    }
    if (remote) VirtualFreeEx(pi.hProcess, remote, 0, MEM_RELEASE);

    if (rc != 0) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return rc;
    }
    // ultracopier is a tray app and keeps running; the harness monitors + kills it by PID.
    wprintf(L"UC_INJECT_PID=%lu\n", pi.dwProcessId);
    fflush(stdout);
    CloseHandle(pi.hThread);
    // STAY ALIVE as long as ultracopier instead of orphaning it: some processes wedge when their parent
    // exits mid-startup. We were launched detached (Win32_Process.Create), so blocking here costs nothing;
    // ultracopier is a tray app that never exits on its own, so the harness kills it (and us) externally.
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    return 0;
}
#endif // _WIN32
