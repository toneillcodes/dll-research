# DLL Notes
- [DLL Notes](#dll-notes)
  - [Structure](#structure)
    - [Section Table Entry: `IMAGE_SECTION_HEADER`](#section-table-entry-image_section_header)
  - [Internal Layout of .text Bytes](#internal-layout-of-text-bytes)
    - [A. Compiled Machine Code (Instructions)](#a-compiled-machine-code-instructions)
    - [B. Import Address Table (IAT) / Linker Thunks](#b-import-address-table-iat--linker-thunks)
    - [C. Alignment Padding (Slack Space / Code Caves)](#c-alignment-padding-slack-space--code-caves)
  - [Standard Entry Points & Exports](#standard-entry-points--exports)
    - [A. Core OS Lifecycle Entry Point (`DllMain`)](#a-core-os-lifecycle-entry-point-dllmain)
    - [B. COM (Component Object Model) Standard Exports](#b-com-component-object-model-standard-exports)
      - [1. `DllCanUnloadNow`](#1-dllcanunloadnow)
      - [2. `DllGetClassObject`](#2-dllgetclassobject)
      - [3. `DllRegisterServer`](#3-dllregisterserver)
      - [4. `DllUnregisterServer`](#4-dllunregisterserver)
      - [5. `DllInstall`](#5-dllinstall)
  - [Deprecated Stubs](#deprecated-stubs)

## Structure
### Section Table Entry: `IMAGE_SECTION_HEADER`
Before the operating system maps the `.text` section into memory, it parses this specific 40-byte structure to determine where the code lives and how it should be protected.
| Offset | Size (Bytes) | Field Name | Description / Example Value |
| :--- | :--- | :--- | :--- |
| `+0x00` | 8 | `Name` | An 8-byte UTF-8 string padded with null bytes. For this section, it is literally `2E 74 65 78 74 00 00 00` (`.text\0\0\0`). |
| `+0x08` | 4 | `VirtualSize` | The total size of the section when loaded into memory (in bytes). |
| `+0x0C` | 4 | `VirtualAddress` | The **Relative Virtual Address (RVA)**. This is the memory offset from the DLL's base address where the `.text` section will be mapped. |
| `+0x10` | 4 | `SizeOfRawData` | The size of the section on disk (rounded up to the file alignment factor). |
| `+0x14` | 4 | `PointerToRawData` | The file offset (pointer) pointing to the exact byte where the `.text` section's code begins on disk. |
| `+0x18` | 4 | `PointerToRelocations` | Set to `0x00000000` for executables/DLLs (handled by the `.reloc` section instead). |
| `+0x1C` | 4 | `PointerToLinenumbers` | Legacy COFF debugging field; almost always `0x00000000`. |
| `+0x20` | 2 | `NumberOfRelocations` | Legacy field; `0x0000`. |
| `+0x22` | 2 | `NumberOfLinenumbers` | Legacy field; `0x0000`. |
| `+0x24` | 4 | `Characteristics` | Flags defining memory permissions. For `.text`, this is typically a combination of `IMAGE_SCN_CNT_CODE`, `IMAGE_SCN_MEM_EXECUTE`, and `IMAGE_SCN_MEM_READ` (usually `0x60000020`). |

## Internal Layout of .text Bytes
Once mapped into memory or read from disk, the raw byte stream of the .text section is structurally organized into three functional components:

### A. Compiled Machine Code (Instructions)
The bulk of the section consists of continuous, variable-length CPU opcodes (x86/x64). There are no structural boundaries separating distinct functions. 

* **Example:** A standard 64-bit function prologue:
    push rbp
    mov rbp, rsp
    sub rsp, 0x20

* **Byte-for-Byte Representation:** `55 48 89 E5 48 83 EC 20`

### B. Import Address Table (IAT) / Linker Thunks
In many compiled binaries, the compiler places the **Import Address Table (IAT)** or small indirect jump stubs (thunks) directly within the .text section (often grouped in a subsection like .text$mn).

* **Mechanism:** When calling an external API like VirtualProtect, the code jumps to a stub within .text: `jmp qword ptr [__imp_VirtualProtect]`.
* **Byte-for-Byte Representation:** `FF 25 XX XX XX XX` (where XX represents the 4-byte relative offset to the real API pointer).

### C. Alignment Padding (Slack Space / Code Caves)
To optimize CPU cache utilization and conform to section alignment boundaries, compilers pad the gaps between functions and at the end of the section block.

* **Mechanism:** If a function block terminates before reaching a 16-byte alignment boundary, the compiler fills the remaining bytes.
* **Byte-for-Byte Representation:** Usually manifests as repetitive sequences of `0x90` (NOP - No Operation) or `0xCC` (INT 3 - Software Breakpoint Trap).

> **Analysis Note:** These 0xCC or 0x90 sequences serve as highly visible structural delimiters under binary analysis, explicitly marking the boundaries between distinct function segments and the end of the executable section.

## Standard Entry Points & Exports
Unlike a standalone executable, a DLL relies on specific system-defined entry points and standard exports to manage its lifecycle and interface with host processes or subsystems.

### A. Core OS Lifecycle Entry Point (`DllMain`)
This is the fundamental, optional entry point managed directly by the Windows Loader. The operating system calls it automatically whenever the DLL is loaded, unloaded, or when threads interact with the host process.

* **Signature:**
    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

* **Trigger Reasons (`fdwReason`):**
    * `DLL_PROCESS_ATTACH`: The DLL is being mapped into the process address space (e.g., via `LoadLibrary`).
    * `DLL_PROCESS_DETACH`: The DLL is being unmapped from the process address space (e.g., via `FreeLibrary`).
    * `DLL_THREAD_ATTACH`: The host process is creating a new thread.
    * `DLL_THREAD_DETACH`: A thread within the host process is terminating cleanly.

### B. COM (Component Object Model) Standard Exports
When a DLL acts as an In-Process COM Server, the COM runtime expects it to explicitly export a standard set of interface functions from its Export Address Table (EAT).

#### 1. `DllCanUnloadNow`
* **Purpose:** Called by the COM subsystem (via `CoFreeUnusedLibraries`) to determine whether the DLL is doing active work or if it can be safely unmapped from memory.
* **Signature:**
    HRESULT DllCanUnloadNow(void);
* **Return Values:**
    * `S_OK` (`0`): The DLL is idle and has no active object references or class factory locks. The OS will proceed to unload it.
    * `S_FALSE` (`1`): The DLL is still in use and must remain pinned in memory.

#### 2. `DllGetClassObject`
* **Purpose:** The factory gatekeeper. When an external application attempts to instantiate a component inside the DLL (e.g., via `CoCreateInstance`), Windows calls this function to retrieve the Class Factory object capable of creating that component.
* **Signature:**
    HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

#### 3. `DllRegisterServer`
* **Purpose:** Instructs the DLL to write its own configuration, Class IDs (CLSIDs), and file paths directly into the Windows Registry.
* **Trigger Mechanism:** Executed by running the registration tool: `regsvr32.exe mylibrary.dll`.
* **Signature:**
    HRESULT DllRegisterServer(void);

#### 4. `DllUnregisterServer`
* **Purpose:** The inverse of registration. Instructs the DLL to cleanly wipe all of its keys, programmatic identifiers, and GUIDs from the Windows Registry.
* **Trigger Mechanism:** Executed by running the unregistration flag: `regsvr32.exe /u mylibrary.dll`.
* **Signature:**
    HRESULT DllUnregisterServer(void);

#### 5. `DllInstall`
* **Purpose:** An optional, customizable setup function used to perform specific installation or configuration routines that go beyond standard registry entries.
* **Trigger Mechanism:** Executed via `regsvr32.exe /i mylibrary.dll`.
* **Signature:**
    HRESULT DllInstall(BOOL bInstall, LPCWSTR pszCmdLine);

## Deprecated Stubs
If you dump the EAT from wininet you will notice how an entire block of functions—mostly legacy Gopher protocol functions, along with things like LoadUrlCacheContent—all share the exact same RVA (0x168ac0) and the exact same size (29 bytes).  

For example, the Gopher protocol was completely deprecated and stripped out of wininet.dll years ago. However, if Microsoft completely deleted these names from the Export Address Table (EAT), legacy applications that link against them would fail to start with a catastrophic "Entry Point Not Found" loader error.  

To preserve backward compatibility without maintaining dead code, Microsoft points all of these deprecated functions to a single, generic stub function. This stub function usually contains just a few assembly instructions to set a failure code (like ERROR_NOT_SUPPORTED or FALSE) and return cleanly.