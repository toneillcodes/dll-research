#include <windows.h>

typedef BOOL (WINAPI *DllMainFn)(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

// Raw console output using Kernel32
static void ConsolePrint(const char* text) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut && hOut != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        DWORD len = 0;
        while (text[len]) len++;
        WriteFile(hOut, text, len, &written, NULL);
    }
}

// Minimal string appender: writes src to *dst and updates the pointer
static void AppendStr(char** dst, const char* src) {
    while (*src) {
        *(*dst)++ = *src++;
    }
}

// Converts a 32-bit unsigned integer to decimal string
static void AppendDec(char** dst, DWORD val) {
    if (val == 0) {
        *(*dst)++ = '0';
        return;
    }
    char temp[16];
    int t = 0;
    while (val > 0) {
        temp[t++] = (char)('0' + (val % 10));
        val /= 10;
    }
    while (t > 0) {
        *(*dst)++ = temp[--t];
    }
}

// Converts a pointer/address to hex (e.g. 0x00007FFC12340000)
static void AppendHexPtr(char** dst, const void* ptr) {
    AppendStr(dst, "0x");
    DWORD_PTR val = (DWORD_PTR)ptr;
    const char hexChars[] = "0123456789ABCDEF";

    // Format sized to architecture pointer width (8 chars for x86, 16 for x64)
    int nibbles = (int)(sizeof(void*) * 2);
    for (int i = nibbles - 1; i >= 0; i--) {
        *(*dst)++ = hexChars[(val >> (i * 4)) & 0xF];
    }
}

__declspec(dllexport) DllMainFn GetDllMainPtr(void) {
    return &DllMain;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    char buf[256];
    char* p = buf;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        AppendStr(&p, "[DLL] -> DLL_PROCESS_ATTACH (1) | Module: ");
        AppendHexPtr(&p, (void*)hinstDLL);
        AppendStr(&p, " | Reserved: ");
        AppendHexPtr(&p, lpReserved);
        AppendStr(&p, "\n");
        break;

    case DLL_THREAD_ATTACH:
        AppendStr(&p, "[DLL] -> DLL_THREAD_ATTACH  (2) | Module: ");
        AppendHexPtr(&p, (void*)hinstDLL);
        AppendStr(&p, "\n");
        break;

    case DLL_THREAD_DETACH:
        AppendStr(&p, "[DLL] -> DLL_THREAD_DETACH  (3) | Module: ");
        AppendHexPtr(&p, (void*)hinstDLL);
        AppendStr(&p, "\n");
        break;

    case DLL_PROCESS_DETACH:
        AppendStr(&p, "[DLL] -> DLL_PROCESS_DETACH (0) | Module: ");
        AppendHexPtr(&p, (void*)hinstDLL);
        AppendStr(&p, " | Reserved: ");
        AppendHexPtr(&p, lpReserved);
        AppendStr(&p, "\n");
        break;

    default:
        AppendStr(&p, "[DLL] -> Custom Reason Code (");
        AppendDec(&p, fdwReason);
        AppendStr(&p, ") invoked!\n");
        break;
    }

    *p = '\0';
    ConsolePrint(buf);
    return TRUE;
}