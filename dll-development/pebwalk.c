// cl.exe pebwalk.c
#include <windows.h>
#include <stdio.h>

typedef void (*FnWalkPEB)(void);

int main(void)
{
    printf("[test] Loading pebwalk-lib.dll...\n");

    HMODULE hPebWalkMod = LoadLibraryA("pebwalk-lib.dll");
    if (!hPebWalkMod) {
        printf("[test] Failed to load pebwalk DLL.\n");
        return -1;
    }

    printf("[test] pebwalk-lib.dll DLL loaded at: %p\n", hPebWalkMod);

    FnWalkPEB WalkPEB = (FnWalkPEB)GetProcAddress(hPebWalkMod, "WalkPEB");
    if (!WalkPEB) {
        printf("[test] Failed to resolve WalkPEB export.\n");
        return -1;
    }

    printf("[test] Calling WalkPEB...\n\n");
    WalkPEB();

    printf("\n[test] Done.\n");

    return 0;
}
