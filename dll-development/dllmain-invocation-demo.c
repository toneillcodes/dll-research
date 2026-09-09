#include <windows.h>
#include <stdio.h>

// DllMain signature
typedef BOOL(WINAPI* DllMain_t)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
typedef DllMain_t(*GetDllMainPtr_t)();

DWORD WINAPI DummyThread(LPVOID param) {
    printf("[Harness] Worker thread started (triggers THREAD_ATTACH).\n");
    Sleep(100);
    printf("[Harness] Worker thread exiting (triggers THREAD_DETACH).\n");
    return 0;
}

int main() {
    printf("=== Part 1: Natural OS Invocation ===\n");

    // 1. Triggers DLL_PROCESS_ATTACH
    HMODULE hMod = LoadLibraryA("example.dll");
    if (!hMod) {
        printf("Failed to load example.dll\n");
        return 1;
    }

    // 2. Triggers DLL_THREAD_ATTACH and DLL_THREAD_DETACH automatically
    HANDLE hThread = CreateThread(NULL, 0, DummyThread, NULL, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    // 3. Triggers DLL_PROCESS_DETACH
    FreeLibrary(hMod);

    printf("\n=== Part 2: Manual Direct Invocations ===\n");

    // Reload without unloading to keep it in memory
    hMod = LoadLibraryA("example.dll");
    if (!hMod) return 1;

    GetDllMainPtr_t pfnGetDllMain = (GetDllMainPtr_t)GetProcAddress(hMod, "GetDllMainPtr");
    if (!pfnGetDllMain) {
        printf("Could not find GetDllMainPtr export.\n");
        FreeLibrary(hMod);
        return 1;
    }

    DllMain_t pDllMain = pfnGetDllMain();
    printf("[Harness] DllMain located at: %p\n\n", (void*)pDllMain);

    // Call each standard reason code directly
    printf("Invoking DLL_PROCESS_ATTACH manually:\n");
    pDllMain(hMod, DLL_PROCESS_ATTACH, (LPVOID)1);      /* 1 indicates static/non-FreeLibrary context */

    printf("\nInvoking DLL_THREAD_ATTACH manually:\n");
    pDllMain(hMod, DLL_THREAD_ATTACH, NULL);

    printf("\nInvoking DLL_THREAD_DETACH manually:\n");
    pDllMain(hMod, DLL_THREAD_DETACH, NULL);

    printf("\nInvoking DLL_PROCESS_DETACH manually:\n");
    pDllMain(hMod, DLL_PROCESS_DETACH, NULL);

    printf("\nInvoking a custom/arbitrary reason value (e.g., 999):\n");
    pDllMain(hMod, 999, NULL);

    FreeLibrary(hMod);
    return 0;
}