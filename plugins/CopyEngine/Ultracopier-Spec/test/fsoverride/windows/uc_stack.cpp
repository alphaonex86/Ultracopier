/*
 * uc_stack.exe <PID> -- a tiny sampling stack-walker (TEST TOOLING ONLY).
 *
 * The laptop has no cdb/procdump, so this captures the call stacks of every thread of a target process
 * via dbghelp's StackWalk64 (x64 unwind info, which mingw binaries carry in .pdata -- no PDB needed). For
 * each frame it prints "<module> + 0x<rva>" (rva = pc - module_base) so the harness can addr2line the
 * ultracopier frames against the -g build to see exactly where a spinning process loops.
 *
 * Run it 2-3 times a few seconds apart on a spinning process: the frame that keeps reappearing IS the loop.
 */
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: uc_stack <PID>\n"); return 2; }
    DWORD pid = (DWORD)strtoul(argv[1], NULL, 10);
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_OPERATION |
                              PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!proc) { printf("OpenProcess(%lu) failed: %lu\n", pid, GetLastError()); return 3; }

    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_NO_PROMPTS);
    SymInitialize(proc, NULL, TRUE);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) { printf("snapshot failed\n"); return 4; }

    THREADENTRY32 te; te.dwSize = sizeof(te);
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID != pid) continue;
            HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME,
                                   FALSE, te.th32ThreadID);
            if (!th) continue;
            SuspendThread(th);
            CONTEXT ctx; ZeroMemory(&ctx, sizeof(ctx)); ctx.ContextFlags = CONTEXT_FULL;
            if (GetThreadContext(th, &ctx)) {
                STACKFRAME64 sf; ZeroMemory(&sf, sizeof(sf));
                sf.AddrPC.Offset    = ctx.Rip; sf.AddrPC.Mode    = AddrModeFlat;
                sf.AddrFrame.Offset = ctx.Rbp; sf.AddrFrame.Mode = AddrModeFlat;
                sf.AddrStack.Offset = ctx.Rsp; sf.AddrStack.Mode = AddrModeFlat;
                printf("--- thread %lu ---\n", te.th32ThreadID);
                for (int i = 0; i < 28; ++i) {
                    if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, th, &sf, &ctx, NULL,
                                     SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                        break;
                    DWORD64 pc = sf.AddrPC.Offset;
                    if (!pc) break;
                    DWORD64 base = SymGetModuleBase64(proc, pc);
                    char mod[MAX_PATH] = "?";
                    IMAGEHLP_MODULE64 mi; ZeroMemory(&mi, sizeof(mi)); mi.SizeOfStruct = sizeof(mi);
                    if (base && SymGetModuleInfo64(proc, base, &mi))
                        strncpy(mod, mi.ModuleName, MAX_PATH - 1);
                    printf("  %-16s + 0x%-8llx pc=0x%llx\n", mod,
                           (unsigned long long)(base ? pc - base : pc), (unsigned long long)pc);
                }
            }
            ResumeThread(th);
            CloseHandle(th);
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
    SymCleanup(proc);
    CloseHandle(proc);
    return 0;
}
#endif
