# Detailed Code Breakdown: `notabeacon.c`

This document provides a comprehensive analysis of `notabeacon.c`, detailing its structural components, Windows API dependencies, and simulated behaviors.

---

## 1. Directives and Global State

### Headers
* `#include <windows.h>`: The core header file containing declarations for all the Windows API functions, data types (like `DWORD`, `HANDLE`, `BOOL`), and macros used throughout the program.

### Global Variables
```c
HANDLE g_hWorkerThread = NULL;
```
* **Purpose:** Acts as a globally accessible pointer reference to track the spawned background execution thread. 
* **Importance:** Initializing it to `NULL` provides a predictable default state, allowing the cleanup routine in `DLL_PROCESS_DETACH` to verify if a thread was successfully initialized before attempting to close its handle.

---

## 2. Execution Routine: `AgentLoop`

```c
DWORD WINAPI AgentLoop(LPVOID lpParam)
```
* **Significance:** This function serves as the entry point for the secondary thread spawned by the DLL. It matches the required `ThreadProc` signature (`DWORD` return type, `WINAPI` calling convention, and a single `LPVOID` parameter).

### Configuration & State Variables
* `int checkinCounter = 0;`: Tracks how many times the simulated beacon has completed an iteration.
* `DWORD sleepIntervalMs = 30000;`: Configures a static 30,000 millisecond (30 seconds) timer interval, establishing the foundational cadence of the agent.

### The Beaconing Simulation (`while (TRUE)`)
The infinite loop simulates a command-and-control (C2) agent's operational cycle:

1. **Activity Metric Increment:** `checkinCounter++` tracks execution longevity.
2. **Workload Simulation:** ```c
   SYSTEMTIME st;
   GetLocalTime(&st);
   ```
   Instead of carrying out malicious operations, the payload executes standard API calls to populate a `SYSTEMTIME` structure with the current local time coordinates (hours, minutes, seconds) and outputs them to stdout. This replicates benign host interaction telemetry.
3. **The Jitter/Sleep Cycle:**
   ```c
   Sleep(sleepIntervalMs);
   ```
   The thread pauses execution entirely for 30 seconds. In offensive security architecture, this replicates a basic beacon interval, which limits host processing overhead and makes network traffic or process activity predictable.

---

## 3. Exported Functions

```c
__declspec(dllexport) void voidRunTest(void)
```
* **Purpose:** The `__declspec(dllexport)` storage-class attribute explicitly directs the compiler to add `voidRunTest` to the DLL’s Export Address Table (EAT).
* **Utility:** This is really only useful to validate that the DLL loaded successfully and an export could be exported and run.

---

## 4. Entry Point: `DllMain`

```c
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
```
* **Purpose:** This is the standard entry-point function for a dynamic-link library. In normal execution, the operating system calls this function when loading or unloading the module into a process's memory space.

### Case: `DLL_PROCESS_ATTACH`
Triggered immediately when the DLL is first mapped into the virtual address space of a process.

* **Optimization Controls:**
  ```c
  DisableThreadLibraryCalls(hinst);
  ```
  This call disables `DLL_THREAD_ATTACH` and `DLL_THREAD_DETACH` notifications for this DLL. It prevents the OS from triggering `DllMain` every time the host process creates or destroys threads, reducing performance overhead and preventing common deadlocks during manual mapping or injection operations.

* **Thread Spawning:**
  ```c
  g_hWorkerThread = CreateThread(NULL, 0, AgentLoop, NULL, 0, NULL);
  ```
  Because lengthy execution within `DllMain` can lock the loader lock and crash the host application, the DLL offloads its continuous loop immediately. 
  * `AgentLoop`: Points to the target function to execute.
  * `0` (Parameter 5): Explicitly tells the OS to start the thread immediately rather than keeping it suspended.

* **Error Handling:** If `CreateThread` returns `NULL`, the function logs a failure message and returns `FALSE`, causing the operating system to fail the library loading process cleanly.

### Case: `DLL_PROCESS_DETACH`
Triggered when the DLL is being unmapped from the host process's address space.

* **Resource Cleanup:**
  ```c
  if (g_hWorkerThread) { CloseHandle(g_hWorkerThread); }
  ```
  Verifies if a thread handle exists. If it does, `CloseHandle` decreases the thread object's usage count, allowing the operating system to free kernel resources associated with that thread once it terminates.

Compilation
```
cl.exe /O1 /GS- /LD .\notabeacon.c /link /NODEFAULTLIB /ENTRY:DllMain /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF kernel32.lib /nologo
```

Example compilation output
```
PS C:\dev\dll-research\dll-development> cl.exe /O1 /GS- /LD .\notabeacon.c /link /NODEFAULTLIB /ENTRY:DllMain /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF kernel32.lib /nologo
notabeacon.c
   Creating library notabeacon.lib and object notabeacon.exp
PS C:\dev\dll-research\dll-development>
```

Difference between compiling with /LD vs additional flags
```
PS C:\dev\dll-research\dll-development> cl.exe /LD notabeacon.c
Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35228 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

notabeacon.c
Microsoft (R) Incremental Linker Version 14.44.35228.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:notabeacon.dll
/dll
/implib:notabeacon.lib
notabeacon.obj
   Creating library notabeacon.lib and object notabeacon.exp
PS C:\dev\dll-research\dll-development> (Get-Item notabeacon.dll).Length
104448
PS C:\dev\dll-research\dll-development>
```
Building with optimization flags
```
PS C:\dev\dll-research\dll-development> cl.exe /O1 /GS- /LD .\notabeacon.c /link /NODEFAULTLIB /ENTRY:DllMain /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF kernel32.lib
Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35228 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

notabeacon.c
Microsoft (R) Incremental Linker Version 14.44.35228.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:notabeacon.dll
/dll
/implib:notabeacon.lib
/NODEFAULTLIB
/ENTRY:DllMain
/SUBSYSTEM:WINDOWS
/OPT:REF
/OPT:ICF
kernel32.lib
notabeacon.obj
   Creating library notabeacon.lib and object notabeacon.exp
PS C:\dev\dll-research\dll-development> (Get-Item notabeacon.dll).Length
3072
PS C:\dev\dll-research\dll-development>
```