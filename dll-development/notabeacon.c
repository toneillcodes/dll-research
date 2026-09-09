// notabeacon.c
// compilation: cl.exe /O1 /GS- /LD .\notabeacon.c /link /NODEFAULTLIB /ENTRY:DllMain /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF kernel32.lib
#include <windows.h>

HANDLE g_hWorkerThread = NULL;

static void ConsolePrint(const char* text) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut && hOut != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        DWORD len = 0;
        while (text[len]) len++;
        WriteFile(hOut, text, len, &written, NULL);
    }
}

static void PrintNumber(int val) {
    char buf[16];
    int i = 0;
    if (val == 0) {
        buf[i++] = '0';
    } else {
        char temp[16];
        int t = 0;
        while (val > 0) {
            temp[t++] = '0' + (val % 10);
            val /= 10;
        }
        while (t > 0) {
            buf[i++] = temp[--t];
        }
    }
    buf[i] = '\0';
    ConsolePrint(buf);
}

DWORD WINAPI AgentLoop(LPVOID lpParam) {
    int checkinCounter = 0;
    DWORD sleepIntervalMs = 5000; // Reduced to 5s for easier testing

    ConsolePrint("[PAYLOAD] Asynchronous agent validation thread started successfully.\n");

    while (TRUE) {
        checkinCounter++;
        ConsolePrint("[PAYLOAD] Heartbeat Event #");
        PrintNumber(checkinCounter);
        ConsolePrint(" - Dispatching local diagnostics check...\n");

        Sleep(sleepIntervalMs);
    }

    return 0;
}

__declspec(dllexport) void voidRunTest(void) {
    ConsolePrint("[PAYLOAD] Manual verification export triggered.\n");
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        ConsolePrint("[PAYLOAD] DllMain triggered with DLL_PROCESS_ATTACH.\n");

        g_hWorkerThread = CreateThread(NULL, 0, AgentLoop, NULL, 0, NULL);
        if (!g_hWorkerThread) {
            ConsolePrint("[PAYLOAD] Failed to spawn background worker.\n");
            return FALSE;
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_hWorkerThread) {
            CloseHandle(g_hWorkerThread);
        }
    }
    return TRUE;
}