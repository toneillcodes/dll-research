#include <windows.h>
#include <stdio.h>

// Signature of your exported function
typedef int (*GetTlsValue_t)(void);

// Global pointer to the exported function
static GetTlsValue_t pfnGetTlsValue = NULL;

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    int threadNum = (int)(INT_PTR)lpParam;

    if (pfnGetTlsValue) {
        // In a new thread, the TLS template initialized g_tlsCounter to 0x1337 (4919).
        // The TLS callback only overwrote it on DLL_PROCESS_ATTACH, not DLL_THREAD_ATTACH.
        printf("[Thread %d] GetTlsValue() = 0x%X (%d)\n", 
               threadNum, pfnGetTlsValue(), pfnGetTlsValue());
    }
    return 0;
}

int main(void) {
    printf("[Main] Starting harness...\n");

    // 1. Load the DLL dynamically
    // During this call, the OS processes the TLS directory, invokes TlsCallback 
    // (setting value to 0x42), and then invokes DllMain (printing Value = 66).
    printf("[Main] Calling LoadLibrary...\n");
    HMODULE hDll = LoadLibraryA("example-with-tls.dll");
    if (!hDll) {
        printf("[Main] Failed to load DLL. Error: %lu\n", GetLastError());
        return 1;
    }
    printf("[Main] LoadLibrary returned successfully.\n");

    // 2. Resolve the exported function
    pfnGetTlsValue = (GetTlsValue_t)GetProcAddress(hDll, "GetTlsValue");
    if (!pfnGetTlsValue) {
        printf("[Main] Failed to find GetTlsValue export.\n");
        FreeLibrary(hDll);
        return 1;
    }

    // 3. Check value in the main thread (should still be 0x42 / 66)
    printf("[Main] Main thread GetTlsValue() = 0x%X (%d)\n", 
           pfnGetTlsValue(), pfnGetTlsValue());

    // 4. Spawn a worker thread to verify thread isolation
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, (LPVOID)1, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    // 5. Unload the DLL
    FreeLibrary(hDll);
    printf("[Main] Finished.\n");

    return 0;
}