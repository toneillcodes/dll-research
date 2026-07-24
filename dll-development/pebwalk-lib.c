// cl.exe /LD pebwalk-lib.c
#include <windows.h>
#include <winternl.h>
#include <stdio.h>

#include "pebwalk-lib.h"

#define LOCAL_PROCESS_HANDLE ((HANDLE)(LONG_PTR)-1)

typedef struct _FULL_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks; 
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;    
    ULONG Flags;
    WORD ObsoleteLoadCount;
    WORD TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} FULL_LDR_DATA_TABLE_ENTRY, *PFULL_LDR_DATA_TABLE_ENTRY;

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

#pragma intrinsic(__readgsqword)

__declspec(dllexport)
void WalkPEB(void)
{
#ifdef _WIN64
    PPEB peb = (PPEB)__readgsqword(0x60);
#else
    PPEB peb = (PPEB)__readfsdword(0x30);
#endif

    if (!peb || !peb->Ldr) {
        printf("[walkpeb] Failed to read PEB or Ldr.\n");
        return;
    }

    LIST_ENTRY* head = &peb->Ldr->InMemoryOrderModuleList;
    LIST_ENTRY* curr = head->Flink;

    printf("[walkpeb] Walking PEB module list:\n");

    while (curr != head)
    {
        PFULL_LDR_DATA_TABLE_ENTRY entry =
            CONTAINING_RECORD(curr, FULL_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        printf("  Base: %p | Name: %ws\n",
               entry->DllBase,
               entry->BaseDllName.Buffer);

        curr = curr->Flink;
    }
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

// PE validation function to parse and verify DOS and NT headers safely
BOOL GetPEHeaders(HANDLE hProcess, PVOID moduleBase, IMAGE_DOS_HEADER* outDos, IMAGE_NT_HEADERS* outNt) {
    if (!ReadMemoryInternal(hProcess, moduleBase, outDos, sizeof(IMAGE_DOS_HEADER))) return FALSE;
    if (outDos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;

    PVOID ntHeadersAddr = (BYTE*)moduleBase + outDos->e_lfanew;
    if (!ReadMemoryInternal(hProcess, ntHeadersAddr, outNt, sizeof(IMAGE_NT_HEADERS))) return FALSE;
    if (outNt->Signature != IMAGE_NT_SIGNATURE) return FALSE;

    return TRUE;
}

PVOID GetModuleBaseManual(PPEB pebObject, const char* targetModuleName) {
    return GetModuleBaseManualGeneric(LOCAL_PROCESS_HANDLE, (PVOID)pebObject, targetModuleName);
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
            
            if (ReadMemoryInternal(hProcess, funcRVAAddr, &funcRVA, sizeof(DWORD))) {
                if (funcRVA >= exportDataDir.VirtualAddress && funcRVA < (exportDataDir.VirtualAddress + exportDataDir.Size)) {
                    printf("[!] Warning: Forwarded export detected.\n");
                    break;
                }
                functionAddress = (BYTE*)moduleBase + funcRVA;
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

__declspec(dllexport)
void DemoKernel32Exports(void)
{
    PPEB peb = (PPEB)GetPebAddressGeneric(GetCurrentProcess());
    // todo: stack-based string construction
    char k32libraryName[] = { 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', '\0' };
    void* kernelBase = GetModuleBaseManual(peb, k32libraryName);

    // todo: hash values
    // "LoadLibraryA" on the stack (13 bytes total)
    char loadLibraryAName[] = { 'L', 'o', 'a', 'd', 'L', 'i', 'b', 'r', 'a', 'r', 'y', 'A', '\0' };
    void* loadLib = GetProcAddressManual(
        LOCAL_PROCESS_HANDLE,
        kernelBase,
        loadLibraryAName
    );

    // todo: hash values
    // "GetProcAddress" on the stack (15 bytes total)
    char getProcAddressName[] = { 'G', 'e', 't', 'P', 'r', 'o', 'c', 'A', 'd', 'd', 'r', 'e', 's', 's', '\0' };
    void* getProc = GetProcAddressManual(
        LOCAL_PROCESS_HANDLE,
        kernelBase,
        getProcAddressName
    );

    printf("LoadLibraryA: %p\n", loadLib);
    printf("GetProcAddress: %p\n", getProc);
}