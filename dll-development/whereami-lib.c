// cl.exe /LD whereami-lib.c
#include <windows.h>
#include <stdio.h>

#pragma intrinsic(_ReturnAddress)
__declspec(noinline) void* GetCurrentAddress() {
    return _ReturnAddress();
}

__declspec(dllexport)
void WhereAmI(void)
{
    BYTE* cursor = (BYTE*)GetCurrentAddress();

    // Walk backward until we find 'MZ'
    while (cursor > (BYTE*)0x10000)  // avoid low memory
    {
        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)cursor;

        if (dos->e_magic == IMAGE_DOS_SIGNATURE)  // 'MZ'
        {
            // Validate NT header
            IMAGE_NT_HEADERS* nt =
                (IMAGE_NT_HEADERS*)(cursor + dos->e_lfanew);

            if (nt->Signature == IMAGE_NT_SIGNATURE)  // 'PE\0\0'
            {
                printf("[whereami] Found my base address: %p\n", cursor);
                return;
            }
        }

        cursor--;
    }

    printf("[whereami] Failed to locate image base.\n");
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinst);
    return TRUE;
}