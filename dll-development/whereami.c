// cl.exe whereami.c
#include <windows.h>
#include <stdio.h>

typedef void (*FnWhereAmI)(void);

int main(void)
{
    printf("[test] Loading whereami-lib.dll...\n");

    HMODULE hMod = LoadLibraryA("whereami-lib.dll");
    if (!hMod) {
        printf("[test] Failed to load DLL.\n");
        return -1;
    }

    printf("[test] DLL loaded at: %p\n", hMod);
    printf("[test] Resolving target function\n");
    FnWhereAmI WhereAmI = (FnWhereAmI)GetProcAddress(hMod, "WhereAmI");
    if (!WhereAmI) {
        printf("[test] Failed to resolve WhereAmI export.\n");
        return -1;
    }
    printf("[test] Target function found at: %p\n", WhereAmI);

    printf("[test] Calling WhereAmI...\n\n");

    WhereAmI();

    printf("\n[test] Done.\n");

    return 0;
}
