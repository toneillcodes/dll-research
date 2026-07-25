// cl.exe loader-test.c
#include <windows.h>
#include <stdio.h>

int main(void)
{
    // Load the DLL
    HMODULE hMod = LoadLibraryA("loader-notabeacon.dll");
    if (!hMod) {
        printf("Failed to load DLL.\n");
        return 1;
    }

    // Resolve the reflective loader export
    FARPROC pReflective = GetProcAddress(hMod, "StartReflective");
    if (!pReflective) {
        printf("Failed to resolve StartReflective. Error = %lu\n", GetLastError());
        return 1;
    }

    // Call the reflective loader export
    printf("[DEBUG] Calling ReflectiveLoader...\n");
    ((void(*)())pReflective)();
    printf("[DEBUG] Returned from ReflectiveLoader.\n");

    // Keep process alive to observe the agent threads
    printf("Reflective loader invoked. Press Enter to exit.\n");
    getchar();

    return 0;
}