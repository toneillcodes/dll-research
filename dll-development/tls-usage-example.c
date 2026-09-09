#include <windows.h>
#include <stdio.h>

// 1. Implicit TLS variable (forces creation of the .tls section & TLS directory)
__declspec(thread) int g_tlsCounter = 0x1337;

// 2. TLS Callback declaration
void NTAPI TlsCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    if (Reason == DLL_PROCESS_ATTACH) {
        // Runs before DllMain during process initialization
        //g_tlsCounter = 0x42;
        int x = 0;
    }
}

// 3. Register the TLS callback in the PE TLS directory
// Note: CRT/MSVC places callbacks in the .CRT$XL* section array
#pragma section(".CRT$XLB", read)
__declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK pTlsCallback = TlsCallback;

// Exported function to prevent the compiler/linker from discarding symbols
__declspec(dllexport) int GetTlsValue(void) {
    return g_tlsCounter;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    int value = GetTlsValue();
    printf("Value = %d", value);
    return TRUE;
}