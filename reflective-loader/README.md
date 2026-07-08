# Reflective Loader
Example reflective loader.

## Example
* Compile the loader
```
c:\dev\dll-research\reflective-loader>cl.exe loader.c
Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35228 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

loader.c
Microsoft (R) Incremental Linker Version 14.44.35228.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:loader.exe
loader.obj

c:\dev\dll-research\reflective-loader>
```
* Run the loader with the hellow example DLL
```
c:\dev\dll-research\reflective-loader>loader.exe ..\dll-development\hellow.dll
[*] Entering Restoration Pipeline phase.
[+] Restoration complete. Stream memory buffer decrypted successfully.
[+] Base allocated locally at: 0x0000020AD2220000
[*] Adjusting mapped section permissions to strict values...
    -> Section: .text    | Flag: 0x60000020 -> Applied Protect: 0x20
    -> Section: .rdata   | Flag: 0x40000040 -> Applied Protect: 0x02
    -> Section: .data    | Flag: 0xC0000040 -> Applied Protect: 0x04
    -> Section: .pdata   | Flag: 0x40000040 -> Applied Protect: 0x02
    -> Section: .fptable | Flag: 0xC0000040 -> Applied Protect: 0x04
    -> Section: .reloc   | Flag: 0x42000040 -> Applied Protect: 0x02
[+] Executing entry point handoff at: 0x0000020AD222141C
Hello from the test export.
[+] Processing complete. Status: SUCCESS
[*] Host engine holding process boundary open.
[*] Monitoring background agent thread loop... (Press Ctrl+C to exit)

^C
c:\dev\dll-research\reflective-loader>
```
* Run the loader with the fake beacon
```
c:\dev\dll-research\reflective-loader>loader.exe ..\dll-development\notabeacon.dll
[*] Entering Restoration Pipeline phase.
[+] Restoration complete. Stream memory buffer decrypted successfully.
[+] Base allocated locally at: 0x000001BEC59B0000
[*] Adjusting mapped section permissions to strict values...
    -> Section: .text    | Flag: 0x60000020 -> Applied Protect: 0x20
    -> Section: .rdata   | Flag: 0x40000040 -> Applied Protect: 0x02
    -> Section: .data    | Flag: 0xC0000040 -> Applied Protect: 0x04
    -> Section: .pdata   | Flag: 0x40000040 -> Applied Protect: 0x02
    -> Section: .fptable | Flag: 0xC0000040 -> Applied Protect: 0x04
    -> Section: .reloc   | Flag: 0x42000040 -> Applied Protect: 0x02
[+] Executing entry point handoff at: 0x000001BEC59B1614
[PAYLOAD] DllMain triggered with DLL_PROCESS_ATTACH.
[+] Processing complete. Status: SUCCESS
[*] Host engine holding process boundary open.
[*] Monitoring background agent thread loop... (Press Ctrl+C to exit)

[PAYLOAD] Asynchronous agent validation thread started successfully.
[PAYLOAD] Heartbeat Event #1 - Dispatching local diagnostics check...
[PAYLOAD] Local System Time: 01:39:01
[PAYLOAD] Entering interval sleep for 5000 ms.

[PAYLOAD] Heartbeat Event #2 - Dispatching local diagnostics check...
[PAYLOAD] Local System Time: 01:39:06
[PAYLOAD] Entering interval sleep for 5000 ms.

[PAYLOAD] Heartbeat Event #3 - Dispatching local diagnostics check...
[PAYLOAD] Local System Time: 01:39:11
[PAYLOAD] Entering interval sleep for 5000 ms.

[PAYLOAD] Heartbeat Event #4 - Dispatching local diagnostics check...
[PAYLOAD] Local System Time: 01:39:16
[PAYLOAD] Entering interval sleep for 5000 ms.

^C
c:\dev\dll-research\reflective-loader>
```