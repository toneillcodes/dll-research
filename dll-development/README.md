# DLL Development
## Component: `hellow-dll`

`hellow-dll.c` is a minimalistic Dynamic Link Library (DLL) template designed to verify basic DLL loading dynamics and export triggering. Unlike more complex implementations that decouple execution via background threads, this component serves as a direct, synchronized test payload to confirm immediate entry-point execution and explicit export accessibility.
    - [Detailed Documentation](/documentation/hellow-dll.md)

### Technical Behavior
* **Immediate Entry-Point Execution:** Upon instantiation by any process reason (such as `DLL_PROCESS_ATTACH`), `DllMain` directly invokes the internal logic. This structure is ideal for testing immediate, inline execution hooks.
* **Explicit Export Exposure:** Features a standalone exported function (`voidRunTest`) that prints a validation message to the standard output, allowing testers to verify that application loaders can properly locate and resolve the DLL's export address table (EAT).

### Compilation
To compile the source code into a DLL using the Microsoft Visual C++ compiler (`cl.exe`), execute the following command in a Developer Command Prompt:

```cmd
cl.exe /LD hellow-dll.c /Fe:hellow.dll
```

## Component: `notabeacon`

`notabeacon.c` is a benign, non-malicious C-based DLL designed to mimic the execution flow and timing footprints typically associated with command-and-control (C2) agents or beacons. It serves as a safe testing utility for verifying detection pipelines, telemetry collection, and binary side-loading or manual mapping techniques without introducing actual risk to the environment.
    - [Detailed Documentation](/documentation/notabeacon.md)

### Technical Behavior
* **Asynchronous Execution**: Upon initialization (DLL_PROCESS_ATTACH), the DLL immediately spawns a detached background worker thread (AgentLoop) to avoid blocking the host process.
* **Heartbeat Simulation**: The background loop mimics C2 beaconing by executing a standard, non-malicious workload (fetching local system time) on a periodic 30-second interval.
* **Evasion Simulation Controls**: Calls DisableThreadLibraryCalls to prevent thread attach/detach overhead and locking overhead, simulating optimization techniques often observed in stealthier loaders.
* **Export Verification**: Includes an explicitly exported function (voidRunTest) for manual verification of export tables.

### Compilation
To compile the source code into a DLL using the Microsoft Visual C++ compiler (`cl.exe`), execute the following command in a Developer Command Prompt:

```cmd
cl.exe /LD notabeacon.c /Fe:notabeacon.dll
```

## Component 'whereami'

`whereami-lib.c` todo: write summary
    - Detailed Documentation

`whereami.c` todo: write summary
    - Detailed Documentation

### Compilation
First, compile the whereami-lib source code into a DLL using the Microsoft Visual C++ compiler (`cl.exe`), execute the following command in a Developer Command Prompt:


## Component 'whereami'

`pebwalk-lib.c` todo: write summary
    - Detailed Documentation

`pebwalk.c` todo: write summary
    - Detailed Documentation

### Compilation
First, compile the whereami-lib source code into a DLL using the Microsoft Visual C++ compiler (`cl.exe`), execute the following command in a Developer Command Prompt:

```cmd
c:\dev\dll-research\dll-development>cl /LD whereami-lib.c
Microsoft (R) C/C++ Optimizing Compiler Version 19.16.27054 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

whereami-lib.c
Microsoft (R) Incremental Linker Version 14.16.27054.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:whereami-lib.dll
/dll
/implib:whereami-lib.lib
whereami-lib.obj
   Creating library whereami-lib.lib and object whereami-lib.exp

c:\dev\dll-research\dll-development>
```

Second, compile the wheream source code into an executable using the Microsoft Visual C++ compiler (`cl.exe`), execute the following command in a Developer Command Prompt:

```cmd
c:\dev\dll-research\dll-development>cl.exe whereami.c
Microsoft (R) C/C++ Optimizing Compiler Version 19.16.27054 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

whereami.c
Microsoft (R) Incremental Linker Version 14.16.27054.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:whereami.exe
whereami.obj

c:\dev\dll-research\dll-development>
```

### Execution

```cmd 
c:\dev\dll-research\dll-development> whereami.exe
[test] Loading whereami-lib.dll...
[test] DLL loaded at: 00007FF849740000
[test] Resolving target function
[test] Target function found at: 00007FF849741010
[test] Calling WhereAmI...

[whereami] Found my base address: 00007FF849740000

[test] Done.

c:\dev\dll-research\dll-development>
```