// Compile command: cl.exe /LD notabeacon.c /Fe:notabeacon.dll
#include <windows.h>
#include <stdio.h>

// A global handle to keep track of our worker thread footprint
HANDLE g_hWorkerThread = NULL;

// The asynchronous worker loop that replicates agent heartbeat execution
DWORD WINAPI AgentLoop(LPVOID lpParam) {
    int checkinCounter = 0;
    DWORD sleepIntervalMs = 30000; // 30 second default interval

    printf("[PAYLOAD] Asynchronous agent validation thread started successfully.\n");

    // Replicate an infinite execution cycle
    while (TRUE) {
        checkinCounter++;
        printf("[PAYLOAD] Heartbeat Event #%d - Dispatching local diagnostics check...\n", checkinCounter);

        // Standard, non-malicious workload simulation (e.g., system time check)
        SYSTEMTIME st;
        GetLocalTime(&st);
        printf("[PAYLOAD] Local System Time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);

        // Mimic beacon 'Jitter' or sleep intervals
        printf("[PAYLOAD] Entering interval sleep for %lu ms.\n\n", sleepIntervalMs);
        Sleep(sleepIntervalMs);
    }

    return 0;
}

__declspec(dllexport) void voidRunTest(void) {
    printf("[PAYLOAD] Manual verification export triggered.\n");
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            // Crucial for manual mapping: prevents thread attach/detach locks
            DisableThreadLibraryCalls(hinst);

            printf("[PAYLOAD] DllMain triggered with DLL_PROCESS_ATTACH.\n");

            // Hand execution off to a background thread immediately
            g_hWorkerThread = CreateThread(
                NULL,               // Default security descriptors
                0,                  // Default stack size
                AgentLoop,          // Target function pointer
                NULL,               // No arguments passed
                0,                  // Run immediately upon instantiation
                NULL                // Ignore thread identifier tracking
            );

            if (g_hWorkerThread == NULL) {
                printf("[PAYLOAD] Critical Error: Failed to spawn background worker loop.\n");
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            // Cleanup the background handle if the process closes down cleanly
            if (g_hWorkerThread) {
                CloseHandle(g_hWorkerThread);
            }
            break;
    }
    return TRUE; 
}