// cl.exe /LD hellow-dll.c /Fe:hellow.dll
#include <windows.h>
#include <stdio.h>

__declspec(dllexport) void voidRunTest(void) {
    printf("Hello from the test export.\n");
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    voidRunTest();
    return TRUE; 
}