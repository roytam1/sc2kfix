// sc2kfix dllmain.cpp: all the magic happens here
// (c) 2025 github.com/araxestroy - released under the MIT license

#define GETPROC(i, name) fpWinMMHookList[i] = GetProcAddress(hRealWinMM, #name);
#define DEFPROC(i, name) extern "C" __declspec(naked) void __stdcall _##name() { __asm { jmp fpWinMMHookList[i*4] }};

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <time.h>

#include <sc2kfix.h>
#include <resource.h>
#include <winmm_exports.h>

#pragma intrinsic(_ReturnAddress)

// From registry_install.cpp
extern char szMayorName[64];
extern char szCompanyName[64];

// Global variables that we need to keep handy
HMODULE hRealWinMM = NULL;
HMODULE hSC2KAppModule = NULL;
HMODULE hSC2KFixModule = NULL;
FARPROC fpWinMMHookList[180] = { NULL };
DWORD dwDetectedVersion = SC2KVERSION_UNKNOWN;
DWORD dwSC2KAppTimestamp = 0;
const char* szSC2KFixVersion = SC2KFIX_VERSION;
const char* szSC2KFixBuildInfo = __DATE__ " " __TIME__;
FILE* fdLog = NULL;
BOOL bInSCURK = FALSE;

// Statics
static DWORD dwDummy;

// This code replaces the original stack cleanup and return after the engine
// cycles the animation palette.
// 
// 6881000000      push dword 0x81              ; flags = RDW_INVALIDATE | RDW_ALLCHILDREN
// 6A00            push 0                       ; hrgnUpdate = NULL
// 6A00            push 0                       ; lprcUpdate = NULL
// 8B0D2C704C00    mov ecx, [pCwndMainWindow]
// 8B511C          mov edx, [ecx+0x1C]
// 52              push edx                     ; hWnd
// FF155CFD4E00    call [RedrawWindow]
// 5D              pop ebp                      ; Clean up stack and return
// 5F              pop edi
// 5E              pop esi
// 5B              pop ebx
// C3              retn
BYTE bAnimationPatch1996[30] = {
    0x68, 0x81, 0x00, 0x00, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x8B, 0x0D, 0x2C,
    0x70, 0x4C, 0x00, 0x8B, 0x51, 0x1C, 0x52, 0xFF, 0x15, 0x5C, 0xFD, 0x4E,
    0x00, 0x5D, 0x5F, 0x5E, 0x5B, 0xC3
};

// Same as above, but with the offsets adjusted for the 1995 EXE
BYTE bAnimationPatch1995[30] = {
    0x68, 0x81, 0x00, 0x00, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x8B, 0x0D, 0x2C,
    0x60, 0x4C, 0x00, 0x8B, 0x51, 0x1C, 0x52, 0xFF, 0x15, 0xE8, 0xEC, 0x4E,
    0x00, 0x5D, 0x5F, 0x5E, 0x5B, 0xC3
};

const char* HexPls(UINT uNumber) {
    thread_local char szRet[16] = { 0 };
    sprintf_s(szRet, 16, "0x%08X", uNumber);
    return szRet;
}

void ConsoleLog(int iLogLevel, const char* fmt, ...) {
    va_list args;
    int len;
    char* buf;
    const char* prefix;

    switch (iLogLevel) {
    case LOG_EMERGENCY:
        prefix = "EMERG: ";
        break;
    case LOG_ALERT:
        prefix = "ALERT: ";
        break;
    case LOG_CRITICAL:
        prefix = "CRIT:  ";
        break;
    case LOG_ERROR:
        prefix = "ERROR: ";
        break;
    case LOG_WARNING:
        prefix = "WARN:  ";
        break;
    case LOG_NOTICE:
        prefix = "NOTE:  ";
        break;
    case LOG_INFO:
        prefix = "INFO:  ";
        break;
    case LOG_DEBUG:
        prefix = "DEBUG: ";
        break;
    case LOG_NONE:
    default:                            // XXX - can this be a constexpr error?
        prefix = "";
        break;
    }

    va_start(args, fmt);
    len = _vscprintf(fmt, args) + 1;
    buf = (char*)malloc(len);
    if (buf) {
        vsprintf_s(buf, len, fmt, args);

        if (fdLog) {
            fprintf(fdLog, "%s%s", prefix, buf);
            fflush(fdLog);
        }

        if (bConsoleEnabled)
            printf("%s%s", prefix, buf);
        
        free(buf);
    }

    va_end(args);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    int argc = 0;
    LPWSTR* argv = NULL;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        // Save our own module handle
        hSC2KFixModule = hModule;

        // Find the actual WinMM library
        char buf[200];
        GetSystemDirectory(buf, 200);
        strcat_s(buf, "\\winmm.dll");

        // Load the actual WinMM library
        if (!(hRealWinMM = LoadLibrary(buf))) {
            MessageBox(GetActiveWindow(), "Could not load winmm.dll (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        // Retrieve the list of functions we need to hook or pass through to WinMM
        ALLEXPORTS_HOOKED(GETPROC);
        ALLEXPORTS_PASSTHROUGH(GETPROC);

        if (!(hSC2KAppModule = GetModuleHandle(NULL))) {
            MessageBox(GetActiveWindow(), "Could not GetModuleHandle(NULL) (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        // Get our command line. WARNING: This uses WIDE STRINGS.
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv) {
            for (int i = 0; i < argc; i++) {
                if (!lstrcmpiW(argv[i], L"-console"))
                    bConsoleEnabled = TRUE;
                // TODO - put some debug options here
            }
        }

        // Open a log file. If it fails, we handle that safely elsewhere
        fopen_s(&fdLog, "sc2kfix.log", "w");

        // Allocate ourselves a console and redirect libc stdio to it
        if (bConsoleEnabled) {
            AllocConsole();
            SetConsoleTitle("sc2kfix console");
            FILE* fdDummy = NULL;
            freopen_s(&fdDummy, "CONIN$", "r", stdin);
            freopen_s(&fdDummy, "CONOUT$", "w", stdout);
            freopen_s(&fdDummy, "CONOUT$", "w", stderr);
        }

        ConsoleLog(LOG_NONE, "sc2kfix version %s started - https://github.com/araxestroy/sc2kfix\n", szSC2KFixVersion);
#ifdef DEBUGALL
        ConsoleLog(LOG_DEBUG, "sc2kfix built with DEBUGALL. Strap in.\n");
#endif

        ConsoleLog(LOG_INFO, "SC2K session started at %lld.\n", time(NULL));

        if (bConsoleEnabled) {
            ConsoleLog(LOG_INFO, "Spawned console session.\n");
            printf("INFO:  ");
            ConsoleCmdShowDebug(NULL, NULL);
            hConsoleThread = CreateThread(NULL, 0, ConsoleThread, 0, 0, NULL);
        }

        // If we're attached to SCURK, switch over to the SCURK fix code
        char szModuleBaseName[200];
        GetModuleBaseName(GetCurrentProcess(), NULL, szModuleBaseName, 200);
        if (!_stricmp(szModuleBaseName, "winscurk.exe")) {
            InjectSCURKFix();
            break;
        }

        // Determine what version of SC2K this is
        // HACK: there's probably a better way to do this
        dwSC2KAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSC2KAppModule)->e_lfanew + (UINT_PTR)hSC2KAppModule))->FileHeader.TimeDateStamp;
        switch (dwSC2KAppTimestamp) {
        case 0x302FEA8A:
            dwDetectedVersion = SC2KVERSION_1995;
            break;

        case 0x313E706E:
            dwDetectedVersion = SC2KVERSION_1996;
            break;

        default:
            dwDetectedVersion = SC2KVERSION_UNKNOWN;
            char msg[300];
            sprintf_s(msg, 300, "Could not detect SC2K version (got timestamp %08Xd). Your game will probably crash.\r\n\r\n"
                "Please let us know in a GitHub issue what version of the game you're running so we can look into this.", dwSC2KAppTimestamp);
            MessageBox(GetActiveWindow(), msg, "sc2kfix warning", MB_OK | MB_ICONWARNING);
            ConsoleLog(LOG_WARNING, "SC2K version could not be detected (got timestamp 0x%08X). Game will probably crash.\n", dwSC2KAppTimestamp);
        }

        // Registry check
        if (DoRegistryCheckAndInstall())
            ConsoleLog(LOG_INFO, "Registry entries created by faux-installer.");

        // Palette animation fix
        LPVOID lpAnimationFix;
        PBYTE lpAnimationFixSrc;
        UINT uAnimationFixLength;
        switch (dwDetectedVersion) {
        case SC2KVERSION_1995:
            lpAnimationFix = (LPVOID)0x00456B23;
            lpAnimationFixSrc = bAnimationPatch1995;
            uAnimationFixLength = 30;
            break;

        case SC2KVERSION_1996:
        default:
            lpAnimationFix = (LPVOID)0x004571D3;
            lpAnimationFixSrc = bAnimationPatch1996;
            uAnimationFixLength = 30;
        }
        
        VirtualProtect(lpAnimationFix, uAnimationFixLength, PAGE_EXECUTE_READWRITE, &dwDummy);
        memcpy(lpAnimationFix, lpAnimationFixSrc, uAnimationFixLength);
        ConsoleLog(LOG_INFO, "Patched palette animation fix.\n");

        // Dialog crash fix - hat tip to Aleksander Krimsky (@alekasm on GitHub)
        LPVOID lpDialogFix1;
        LPVOID lpDialogFix2;
        switch (dwDetectedVersion) {
        case SC2KVERSION_1995:
            lpDialogFix1 = (LPVOID)0x0049EE93;
            lpDialogFix2 = (LPVOID)0x0049EEF2;
            break;

        case SC2KVERSION_1996:
        default:
            lpDialogFix1 = (LPVOID)0x004A04FA;
            lpDialogFix2 = (LPVOID)0x004A0559;
        }

        VirtualProtect(lpDialogFix1, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
        *(LPBYTE)lpDialogFix1 = 0x20;
        VirtualProtect(lpDialogFix2, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
        *(LPBYTE)lpDialogFix2 = 0xEB;
        *(LPBYTE)((UINT_PTR)lpDialogFix2 + 1) = 0xEB;
        ConsoleLog(LOG_INFO, "Patched dialog crash fix.\n");

        // Remove palette warnings
        LPVOID lpWarningFix1;
        LPVOID lpWarningFix2;
        switch (dwDetectedVersion) {
        case SC2KVERSION_1995:
            lpWarningFix1 = (LPVOID)0x00408749;
            lpWarningFix2 = (LPVOID)0x0040878E;
            break;

        case SC2KVERSION_1996:
        default:
            lpWarningFix1 = (LPVOID)0x00408A79;
            lpWarningFix2 = (LPVOID)0x00408ABE;
        }
        VirtualProtect(lpWarningFix1, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
        VirtualProtect(lpWarningFix2, 18, PAGE_EXECUTE_READWRITE, &dwDummy);
        *(LPBYTE)lpWarningFix1 = 0x90;
        *(LPBYTE)((UINT_PTR)lpWarningFix1 + 1) = 0x90;
        memset((LPVOID)lpWarningFix2, 0x90, 18);   // nop nop nop nop nop
        ConsoleLog(LOG_INFO, "Patched 8-bit colour warnings.\n");

        // Hooks we only want to inject on the 1996 Special Edition version
        if (dwDetectedVersion == SC2KVERSION_1996) {
            // Intercept call to 0x480140 at 0x401F9B
            VirtualProtect((LPVOID)0x401F9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
            NEWJMP((LPVOID)0x401F9B, Hook_401F9B);

            // Install miscellaneous hooks
            InstallMiscHooks();
        }

        // Print the console prompt and let the console thread take over.
        printf("\n> ");
        break;

    // Nothing to do for these two cases
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    // Clean up after ourselves and get ready to exit
    case DLL_PROCESS_DETACH:
        // Send a closing message and close the log file
        ConsoleLog(LOG_INFO, "Closing down at %lld. Goodnight!\n", time(NULL));
        fflush(fdLog);
        fclose(fdLog);
        break;
    }
    return TRUE;
}

// Exports for WinMM hook
ALLEXPORTS_PASSTHROUGH(DEFPROC)
