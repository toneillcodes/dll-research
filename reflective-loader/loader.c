#include <windows.h>
#include <winternl.h>

PVOID GetPebAddressGeneric(HANDLE hProcess);

PVOID GetModuleBaseManual(PPEB pebObject, const char* targetModuleName);

PVOID GetProcAddressManualGeneric(
    HANDLE hProcess,
    PVOID moduleBase,
    const char* functionName,
    WORD ordinal
);

PVOID ResolveForwarder(
    HANDLE hProcess,
    const char* forwarderString
);

#define LOCAL_PROCESS_HANDLE ((HANDLE)(LONG_PTR)-1)

typedef struct _LOADER_CONTEXT {
    BYTE*  ImageBase;      // original DLL base (where we found MZ)
    PIMAGE_DOS_HEADER Dos;
    PIMAGE_NT_HEADERS Nt;

    BYTE*  NewBase;        // newly allocated mapped image
    SIZE_T ImageSize;
    ULONG_PTR Delta;       // NewBase - OriginalImageBase

    // Core APIs resolved manually
    LPVOID pVirtualAlloc;
    LPVOID pVirtualProtect;
    LPVOID pLoadLibraryA;
    LPVOID pGetProcAddress;
} LOADER_CONTEXT;

// Custom layout ensuring BaseDllName is present regardless of SDK versions
typedef struct _LDR_DATA_TABLE_ENTRY_COMPAT {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} LDR_DATA_TABLE_ENTRY_COMPAT, *PLDR_DATA_TABLE_ENTRY_COMPAT;

#pragma intrinsic(_ReturnAddress)
__declspec(noinline) void* GetCurrentAddress() {
    return _ReturnAddress();
}

BYTE* FindImageBase(void)
{
    BYTE* cursor = (BYTE*)GetCurrentAddress();

    while (cursor > (BYTE*)0x10000) {
        if (ValidatePEHeaders(cursor))
            return cursor;
        cursor--;
    }
    return NULL;
}

BOOL ValidatePEHeaders(BYTE* base)
{
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    if (dos->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        dos->e_lfanew > 4096)
        return FALSE;

    IMAGE_NT_HEADERS* nt =
        (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    if (nt->FileHeader.NumberOfSections == 0)
        return FALSE;

    if (nt->OptionalHeader.SizeOfImage == 0 ||
        nt->OptionalHeader.SizeOfHeaders == 0)
        return FALSE;

    return TRUE;
}

typedef BOOLEAN (WINAPI *FnRtlAddFunctionTable)(
    PRUNTIME_FUNCTION FunctionTable,
    DWORD             EntryCount,
    DWORD64           BaseAddress
);

FnRtlAddFunctionTable ResolveRtlAddFunctionTable(void)
{
    // 1. Get local PEB
    PPEB peb = (PPEB)GetPebAddressGeneric(LOCAL_PROCESS_HANDLE);
    if (!peb) return NULL;

    // 2. Get ntdll base via your manual module resolver
    PVOID ntdllBase = GetModuleBaseManual(peb, "ntdll.dll");
    if (!ntdllBase) return NULL;

    // 3. Resolve RtlAddFunctionTable via your manual export resolver
    FnRtlAddFunctionTable pRtlAddFunctionTable = (FnRtlAddFunctionTable)
        GetProcAddressManualGeneric(
            LOCAL_PROCESS_HANDLE,
            ntdllBase,
            "RtlAddFunctionTable",
            0
        );

    return pRtlAddFunctionTable;
}

void RegisterSEHForImage(LOADER_CONTEXT* ctx)
{
    IMAGE_DATA_DIRECTORY excDir =
        ctx->Nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

    if (excDir.VirtualAddress == 0 || excDir.Size == 0)
        return; // no unwind info

    // 1. Locate runtime function table in mapped image
    PRUNTIME_FUNCTION funcTable = (PRUNTIME_FUNCTION)(
        ctx->NewBase + excDir.VirtualAddress
    );

    DWORD entryCount = excDir.Size / sizeof(RUNTIME_FUNCTION);

    // 2. Resolve RtlAddFunctionTable PIC-safe
    FnRtlAddFunctionTable pRtlAddFunctionTable = ResolveRtlAddFunctionTable();
    if (!pRtlAddFunctionTable)
        return;

    // 3. Register unwind metadata for this image
    pRtlAddFunctionTable(
        funcTable,
        entryCount,
        (DWORD64)ctx->NewBase
    );
}

// Fully PIC TLS initialization using only PEB + manual exports
void InitializeTLS(LOADER_CONTEXT* ctx)
{
    IMAGE_DATA_DIRECTORY tlsDir =
        ctx->Nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

    if (tlsDir.VirtualAddress == 0 || tlsDir.Size == 0)
        return; // no TLS

    // TLS directory inside mapped image
    PIMAGE_TLS_DIRECTORY tls =
        (PIMAGE_TLS_DIRECTORY)(ctx->NewBase + tlsDir.VirtualAddress);

    //
    // 1. Resolve kernel32.dll via PEB + manual module walk
    //
    PPEB peb = (PPEB)GetPebAddressGeneric(LOCAL_PROCESS_HANDLE);
    PVOID kernelBase = GetModuleBaseManual(peb, "kernel32.dll");
    if (!kernelBase) return;

    //
    // 2. Resolve TLS APIs via your manual export resolver
    //
    typedef DWORD  (WINAPI *FnTlsAlloc)(void);
    typedef LPVOID (WINAPI *FnTlsGetValue)(DWORD);
    typedef BOOL   (WINAPI *FnTlsSetValue)(DWORD, LPVOID);

    FnTlsAlloc pTlsAlloc = (FnTlsAlloc)
        GetProcAddressManualGeneric(LOCAL_PROCESS_HANDLE, kernelBase, "TlsAlloc", 0);

    FnTlsGetValue pTlsGetValue = (FnTlsGetValue)
        GetProcAddressManualGeneric(LOCAL_PROCESS_HANDLE, kernelBase, "TlsGetValue", 0);

    FnTlsSetValue pTlsSetValue = (FnTlsSetValue)
        GetProcAddressManualGeneric(LOCAL_PROCESS_HANDLE, kernelBase, "TlsSetValue", 0);

    if (!pTlsAlloc || !pTlsGetValue || !pTlsSetValue)
        return;

    //
    // 3. Allocate TLS index
    //
    DWORD tlsIndex = pTlsAlloc();
    if (tlsIndex == TLS_OUT_OF_INDEXES)
        return;

    //
    // 4. Compute TLS template location inside mapped image
    //
    SIZE_T dataSize = (SIZE_T)(tls->EndAddressOfRawData - tls->StartAddressOfRawData);

    BYTE* tlsDataSrc = ctx->NewBase +
        (SIZE_T)(tls->StartAddressOfRawData - ctx->Nt->OptionalHeader.ImageBase);

    //
    // 5. Allocate TLS block via resolved VirtualAlloc
    //
    typedef LPVOID (WINAPI *FnVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
    FnVirtualAlloc pVirtualAlloc = (FnVirtualAlloc)ctx->pVirtualAlloc;

    BYTE* tlsBlock = (BYTE*)pVirtualAlloc(
        NULL,
        dataSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!tlsBlock)
        return;

    memcpy(tlsBlock, tlsDataSrc, dataSize);

    //
    // 6. Store pointer in TLS slot
    //
    pTlsSetValue(tlsIndex, tlsBlock);

    // TLS callbacks themselves you already invoke later from the TLS directory
}


BOOL InitPeHeaders(LOADER_CONTEXT* ctx)
{
    ctx->Dos = (PIMAGE_DOS_HEADER)ctx->ImageBase;
    if (ctx->Dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    ctx->Nt = (PIMAGE_NT_HEADERS)(ctx->ImageBase + ctx->Dos->e_lfanew);
    if (ctx->Nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    ctx->ImageSize = ctx->Nt->OptionalHeader.SizeOfImage;
    return TRUE;
}

// Seamlessly handles memory read operations for both local and remote execution
BOOL ReadMemoryInternal(HANDLE hProcess, PVOID baseAddress, PVOID localBuffer, SIZE_T size) {
    if (hProcess == NULL || hProcess == LOCAL_PROCESS_HANDLE) {
        __try {
            memcpy(localBuffer, baseAddress, size);
            return TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return FALSE;
        }
    } else {
        SIZE_T bytesRead = 0;
        return ReadProcessMemory(hProcess, baseAddress, localBuffer, size, &bytesRead) && (bytesRead == size);
    }
}

// Resolves forwarded exports of the form "MODULE.FUNCTION" or "MODULE.#ORDINAL"
PVOID ResolveForwarder(
    HANDLE hProcess,
    const char* forwarderString
)
{
    // 1. Split into module part and function part
    char modPart[128]  = {0};
    char funcPart[128] = {0};

    const char* dot = strchr(forwarderString, '.');
    if (!dot) return NULL;

    size_t modLen = (size_t)(dot - forwarderString);
    if (modLen == 0 || modLen >= sizeof(modPart)) return NULL;

    memcpy(modPart, forwarderString, modLen);
    strcpy(funcPart, dot + 1);

    // 2. Normalize module name → ensure ".dll"
    char normalizedModule[160] = {0};
    strcpy(normalizedModule, modPart);

    if (!strchr(modPart, '.')) {
        strcat(normalizedModule, ".dll");
    }

    // 3. Resolve module base
    PVOID targetModuleBase = NULL;

    if (hProcess == LOCAL_PROCESS_HANDLE) {
        PPEB peb = (PPEB)GetPebAddressGeneric(hProcess);
        targetModuleBase = GetModuleBaseManual(peb, normalizedModule);
    } else {
        // Remote case: use normal LoadLibraryA from imports
        HMODULE hMod = LoadLibraryA(normalizedModule);
        targetModuleBase = (PVOID)hMod;
    }

    if (!targetModuleBase) return NULL;

    // 4. Resolve function by ordinal or name
    if (funcPart[0] == '#') {
        WORD ord = (WORD)atoi(funcPart + 1);
        return GetProcAddressManualGeneric(
            hProcess,
            targetModuleBase,
            NULL,
            ord
        );
    }

    return GetProcAddressManualGeneric(
        hProcess,
        targetModuleBase,
        funcPart,
        0
    );
}

// PE validation function to parse and verify DOS and NT headers safely
BOOL GetPEHeaders(HANDLE hProcess, PVOID moduleBase, IMAGE_DOS_HEADER* outDos, IMAGE_NT_HEADERS* outNt) {
    if (!ReadMemoryInternal(hProcess, moduleBase, outDos, sizeof(IMAGE_DOS_HEADER))) return FALSE;
    if (outDos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;

    PVOID ntHeadersAddr = (BYTE*)moduleBase + outDos->e_lfanew;
    if (!ReadMemoryInternal(hProcess, ntHeadersAddr, outNt, sizeof(IMAGE_NT_HEADERS))) return FALSE;
    if (outNt->Signature != IMAGE_NT_SIGNATURE) return FALSE;

    return TRUE;
}

PVOID GetProcAddressManualGeneric(HANDLE hProcess, PVOID moduleBase, const char* functionName, WORD ordinal) {
    IMAGE_DOS_HEADER dosHeader = { 0 };
    IMAGE_NT_HEADERS ntHeaders = { 0 };
    if (!GetPEHeaders(hProcess, moduleBase, &dosHeader, &ntHeaders)) return NULL;

    IMAGE_DATA_DIRECTORY exportDataDir = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDataDir.VirtualAddress == 0) return NULL;

    IMAGE_EXPORT_DIRECTORY exportDir = { 0 };
    PVOID exportDirAddr = (BYTE*)moduleBase + exportDataDir.VirtualAddress;
    if (!ReadMemoryInternal(hProcess, exportDirAddr, &exportDir, sizeof(IMAGE_EXPORT_DIRECTORY))) return NULL;

    if (functionName == NULL) {
        // Resolve via Ordinal
        DWORD functionIndex = ordinal - exportDir.Base;
        if (functionIndex >= exportDir.NumberOfFunctions) return NULL;

        DWORD funcRVA = 0;
        PVOID funcRVAAddr = (BYTE*)moduleBase + exportDir.AddressOfFunctions + (functionIndex * sizeof(DWORD));
        if (!ReadMemoryInternal(hProcess, funcRVAAddr, &funcRVA, sizeof(DWORD))) return NULL;

        return (BYTE*)moduleBase + funcRVA;
    }

    // Resolve via Name (Binary Search)
    DWORD* nameTable = (DWORD*)malloc(exportDir.NumberOfNames * sizeof(DWORD));
    WORD* ordinalTable = (WORD*)malloc(exportDir.NumberOfNames * sizeof(WORD));
    if (!nameTable || !ordinalTable) {
        free(nameTable); free(ordinalTable);
        return NULL;
    }

    ReadMemoryInternal(hProcess, (BYTE*)moduleBase + exportDir.AddressOfNames, nameTable, exportDir.NumberOfNames * sizeof(DWORD));
    ReadMemoryInternal(hProcess, (BYTE*)moduleBase + exportDir.AddressOfNameOrdinals, ordinalTable, exportDir.NumberOfNames * sizeof(WORD));

    int low = 0;
    int high = exportDir.NumberOfNames - 1;
    PVOID functionAddress = NULL;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        char currentName[256] = { 0 };
        
        PVOID nameAddress = (BYTE*)moduleBase + nameTable[mid];
        ReadMemoryInternal(hProcess, nameAddress, currentName, sizeof(currentName) - 1);

        int cmp = strcmp(functionName, currentName);
        if (cmp == 0) {
            WORD ordinalValue = ordinalTable[mid];
            DWORD funcRVA = 0;
            PVOID funcRVAAddr = (BYTE*)moduleBase + exportDir.AddressOfFunctions + (ordinalValue * sizeof(DWORD));
            
            /*if (ReadMemoryInternal(hProcess, funcRVAAddr, &funcRVA, sizeof(DWORD))) {
                if (funcRVA >= exportDataDir.VirtualAddress && funcRVA < (exportDataDir.VirtualAddress + exportDataDir.Size)) {
                    printf("[!] Warning: Forwarded export detected.\n");
                    break;
                }
                functionAddress = (BYTE*)moduleBase + funcRVA;
            }*/
            if (ReadMemoryInternal(hProcess, funcRVAAddr, &funcRVA, sizeof(DWORD))) {

                // Check if this is a forwarded export (RVA points into export directory)
                if (funcRVA >= exportDataDir.VirtualAddress &&
                    funcRVA <  (exportDataDir.VirtualAddress + exportDataDir.Size)) {

                    char forwarder[256] = {0};
                    PVOID fwdAddr = (BYTE*)moduleBase + funcRVA;

                    // Read the forwarder string from the module
                    ReadMemoryInternal(hProcess, fwdAddr, forwarder, sizeof(forwarder) - 1);

                    // Resolve the forwarded target using your helper
                    functionAddress = ResolveForwarder(hProcess, forwarder);
                }
                else {
                    // Normal export: RVA points to code/data inside the module
                    functionAddress = (BYTE*)moduleBase + funcRVA;
                }
            }
            break;
        }
        if (cmp < 0) high = mid - 1;
        else low = mid + 1;
    }

    free(nameTable);
    free(ordinalTable);
    return functionAddress;
}

void* GetProcAddressManual(HANDLE hProcess, void* moduleBase, const char* functionName)
{
    return GetProcAddressManualGeneric(
        hProcess,
        moduleBase,
        functionName,
        0
    );
}

PVOID GetModuleBaseManualGeneric(HANDLE hProcess, PVOID pebAddr, const char* targetModuleName) {
    PEB localPeb = { 0 };
    if (!ReadMemoryInternal(hProcess, pebAddr, &localPeb, sizeof(PEB))) return NULL;
    if (!localPeb.Ldr) return NULL;

    PEB_LDR_DATA localLdr = { 0 };
    if (!ReadMemoryInternal(hProcess, localPeb.Ldr, &localLdr, sizeof(PEB_LDR_DATA))) return NULL;

    PVOID remoteListHead = (BYTE*)localPeb.Ldr + offsetof(PEB_LDR_DATA, InMemoryOrderModuleList);
    LIST_ENTRY currentEntry = localLdr.InMemoryOrderModuleList;

    WCHAR targetNameWide[MAX_PATH] = { 0 };
    MultiByteToWideChar(CP_ACP, 0, targetModuleName, -1, targetNameWide, MAX_PATH);

    while (currentEntry.Flink != remoteListHead) {
        PVOID tableEntryAddr = CONTAINING_RECORD(currentEntry.Flink, LDR_DATA_TABLE_ENTRY_COMPAT, InMemoryOrderLinks);
        LDR_DATA_TABLE_ENTRY_COMPAT moduleEntry = { 0 };

        if (!ReadMemoryInternal(hProcess, tableEntryAddr, &moduleEntry, sizeof(LDR_DATA_TABLE_ENTRY_COMPAT))) break;

        if (moduleEntry.BaseDllName.Buffer && moduleEntry.BaseDllName.Length < (MAX_PATH * sizeof(WCHAR))) {
            WCHAR localNameBuffer[MAX_PATH] = { 0 };
            
            if (ReadMemoryInternal(hProcess, moduleEntry.BaseDllName.Buffer, localNameBuffer, moduleEntry.BaseDllName.Length)) {
                localNameBuffer[moduleEntry.BaseDllName.Length / sizeof(WCHAR)] = L'\0';

                if (_wcsicmp(localNameBuffer, targetNameWide) == 0) {
                    return moduleEntry.DllBase;
                }
            }
        }
        currentEntry = moduleEntry.InMemoryOrderLinks;
    }
    return NULL;
}

PVOID GetModuleBaseManual(PPEB pebObject, const char* targetModuleName) {
    return GetModuleBaseManualGeneric(LOCAL_PROCESS_HANDLE, (PVOID)pebObject, targetModuleName);
}

PVOID GetPebAddressGeneric(HANDLE hProcess) {
    // --- Bootstrap our own API resolution via the local PEB ---
    // Get the local PEB base address silently
    if (hProcess == NULL || hProcess == LOCAL_PROCESS_HANDLE || hProcess == GetCurrentProcess()) {
#ifdef _WIN64
        return (PVOID)__readgsqword(0x60);
#else
        return (PVOID)__readfsdword(0x30);
#endif
    } else {
        return NULL;
    }
}

BOOL ResolveCoreApis(LOADER_CONTEXT* ctx)
{
    PPEB peb = (PPEB)GetPebAddressGeneric(LOCAL_PROCESS_HANDLE);
    void* kernelBase = GetModuleBaseManual(peb, "kernel32.dll");
    if (!kernelBase) return FALSE;

    ctx->pVirtualAlloc   = GetProcAddressManual(LOCAL_PROCESS_HANDLE, kernelBase, "VirtualAlloc");
    ctx->pVirtualProtect = GetProcAddressManual(LOCAL_PROCESS_HANDLE, kernelBase, "VirtualProtect");
    ctx->pLoadLibraryA   = GetProcAddressManual(LOCAL_PROCESS_HANDLE, kernelBase, "LoadLibraryA");
    ctx->pGetProcAddress = GetProcAddressManual(LOCAL_PROCESS_HANDLE, kernelBase, "GetProcAddress");

    return ctx->pVirtualAlloc && ctx->pVirtualProtect &&
           ctx->pLoadLibraryA && ctx->pGetProcAddress;
}

BOOL MapImage(LOADER_CONTEXT* ctx)
{
    typedef LPVOID (WINAPI *FnVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
    FnVirtualAlloc pVirtualAlloc = (FnVirtualAlloc)ctx->pVirtualAlloc;

    ctx->NewBase = (BYTE*)pVirtualAlloc(NULL,
                                        ctx->ImageSize,
                                        MEM_COMMIT | MEM_RESERVE,
                                        PAGE_READWRITE);
    if (!ctx->NewBase) return FALSE;

    memcpy(ctx->NewBase,
           ctx->ImageBase,
           ctx->Nt->OptionalHeader.SizeOfHeaders);

    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(ctx->Nt);
    for (WORD i = 0; i < ctx->Nt->FileHeader.NumberOfSections; i++) {
        if (sec[i].SizeOfRawData == 0) continue;

        BYTE* dest = ctx->NewBase + sec[i].VirtualAddress;
        BYTE* src  = ctx->ImageBase + sec[i].PointerToRawData;

        memcpy(dest, src, sec[i].SizeOfRawData);
    }

    ctx->Delta = (ULONG_PTR)ctx->NewBase - ctx->Nt->OptionalHeader.ImageBase;
    return TRUE;
}

BOOL ApplyRelocations(LOADER_CONTEXT* ctx)
{
    if (ctx->Delta == 0)
        return TRUE;

    IMAGE_DATA_DIRECTORY relocDir =
        ctx->Nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (relocDir.VirtualAddress == 0 || relocDir.Size == 0)
        return TRUE;

    PIMAGE_BASE_RELOCATION block =
        (PIMAGE_BASE_RELOCATION)(ctx->NewBase + relocDir.VirtualAddress);

    while (block->VirtualAddress && block->SizeOfBlock) {
        DWORD count = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        PWORD entries = (PWORD)((BYTE*)block + sizeof(IMAGE_BASE_RELOCATION));

        for (DWORD i = 0; i < count; i++) {
            WORD type   = entries[i] >> 12;
            WORD offset = entries[i] & 0x0FFF;

#ifdef _WIN64
            if (type == IMAGE_REL_BASED_DIR64) {
                ULONG_PTR* target = (ULONG_PTR*)(ctx->NewBase +
                                                 block->VirtualAddress + offset);
                *target += ctx->Delta;
            }
#else
            if (type == IMAGE_REL_BASED_HIGHLOW) {
                DWORD* target = (DWORD*)(ctx->NewBase +
                                         block->VirtualAddress + offset);
                *target += (DWORD)ctx->Delta;
            }
#endif
        }

        block = (PIMAGE_BASE_RELOCATION)((BYTE*)block + block->SizeOfBlock);
    }

    return TRUE;
}

BOOL ResolveImports(LOADER_CONTEXT* ctx)
{
    typedef HMODULE (WINAPI *FnLoadLibraryA)(LPCSTR);
    typedef FARPROC (WINAPI *FnGetProcAddress)(HMODULE, LPCSTR);

    FnLoadLibraryA   pLoadLibraryA   = (FnLoadLibraryA)ctx->pLoadLibraryA;
    FnGetProcAddress pGetProcAddress = (FnGetProcAddress)ctx->pGetProcAddress;

    IMAGE_DATA_DIRECTORY importDir =
        ctx->Nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    if (importDir.VirtualAddress == 0 || importDir.Size == 0)
        return TRUE;

    PIMAGE_IMPORT_DESCRIPTOR imp =
        (PIMAGE_IMPORT_DESCRIPTOR)(ctx->NewBase + importDir.VirtualAddress);

    while (imp->Name) {
        char* dllName = (char*)(ctx->NewBase + imp->Name);
        HMODULE hMod = pLoadLibraryA(dllName);
        if (!hMod) return FALSE;

        PIMAGE_THUNK_DATA iat =
            (PIMAGE_THUNK_DATA)(ctx->NewBase + imp->FirstThunk);

        DWORD lookupRva = imp->OriginalFirstThunk ? imp->OriginalFirstThunk
                                                  : imp->FirstThunk;

        PIMAGE_THUNK_DATA intThunk =
            (PIMAGE_THUNK_DATA)(ctx->NewBase + lookupRva);

        while (iat->u1.Function) {
            PVOID addr = NULL;

            if (IMAGE_SNAP_BY_ORDINAL(intThunk->u1.Ordinal)) {
                WORD ord = IMAGE_ORDINAL(intThunk->u1.Ordinal);
                addr = pGetProcAddress(hMod, (LPCSTR)ord);
            } else {
                PIMAGE_IMPORT_BY_NAME name =
                    (PIMAGE_IMPORT_BY_NAME)(ctx->NewBase + intThunk->u1.AddressOfData);
                addr = pGetProcAddress(hMod, (LPCSTR)name->Name);
            }

            if (!addr) return FALSE;

            iat->u1.Function = (ULONG_PTR)addr;
            iat++;
            intThunk++;
        }

        imp++;
    }

    return TRUE;
}

BOOL ProtectSections(LOADER_CONTEXT* ctx)
{
    typedef BOOL (WINAPI *FnVirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
    FnVirtualProtect pVirtualProtect = (FnVirtualProtect)ctx->pVirtualProtect;

    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(ctx->Nt);

    for (WORD i = 0; i < ctx->Nt->FileHeader.NumberOfSections; i++) {
        if (sec[i].SizeOfRawData == 0) continue;

        BYTE* addr = ctx->NewBase + sec[i].VirtualAddress;
        DWORD size = sec[i].SizeOfRawData;
        DWORD newProt = PAGE_NOACCESS;
        DWORD oldProt = 0;

        if (sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            if (sec[i].Characteristics & IMAGE_SCN_MEM_WRITE)
                newProt = PAGE_EXECUTE_READWRITE;
            else if (sec[i].Characteristics & IMAGE_SCN_MEM_READ)
                newProt = PAGE_EXECUTE_READ;
            else
                newProt = PAGE_EXECUTE;
        } else {
            if (sec[i].Characteristics & IMAGE_SCN_MEM_WRITE)
                newProt = PAGE_READWRITE;
            else if (sec[i].Characteristics & IMAGE_SCN_MEM_READ)
                newProt = PAGE_READONLY;
        }

        pVirtualProtect(addr, size, newProt, &oldProt);
    }

    return TRUE;
}

void CallEntry(LOADER_CONTEXT* ctx)
{
    DWORD epRva = ctx->Nt->OptionalHeader.AddressOfEntryPoint;
    BYTE* ep    = ctx->NewBase + epRva;

    typedef BOOL (WINAPI *FnDllMain)(HINSTANCE, DWORD, LPVOID);
    FnDllMain DllEntry = (FnDllMain)ep;

    DllEntry((HINSTANCE)ctx->NewBase, DLL_PROCESS_ATTACH, NULL);
}

__declspec(dllexport)
void ReflectiveLoader(void)
{
    LOADER_CONTEXT ctx = {0};

    ctx.ImageBase = FindImageBase();
    if (!ctx.ImageBase) return;

    if (!InitPeHeaders(&ctx))        return;
    if (!ResolveCoreApis(&ctx))      return;
    if (!MapImage(&ctx))             return;
    if (!ApplyRelocations(&ctx))     return;
    if (!ResolveImports(&ctx))       return;
    InitializeTLS(&ctx);
    RegisterSEHForImage(&ctx);      // SEH / unwind
    if (!ProtectSections(&ctx))      return;

    CallEntry(&ctx);
}
