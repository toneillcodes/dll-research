# dll-research
Windows Dynamic-Link Library research & offensive tooling.

## Blog Posts
* [Introduction to Module Stomping](https://medium.com/bugbountywriteup/an-introduction-to-module-stomping-26238af76d43)
* [Advanced Evasion Tradecraft: Precision Module Stomping](https://medium.com/@toneillcodes/advanced-evasion-tradecraft-precision-module-stomping-b51feb0978fe)

## Research and Projects

### [API Monitor](api-monitor.md)
#### Overview
Passively monitoring active APIs.
* **Frida Dynamic API Monitoring Telemetry Agent (`api-monitor.py`):** A dynamic instrumentation tool that injects a runtime JavaScript payload into target processes to automatically hook and track exported functions. It leverages asynchronous retry loops for deferred modules and filters out duplicate calls to stream clean, structured API telemetry directly to a JSONL log.
* **PE Dormant Export Analyzer (`find-dormant-blocks.py`):** A static analysis tool that parses a DLL's export table and SEH exception directories (`.pdata`) to map function boundaries linearly in memory. It cross-references these mappings against runtime telemetry and custom blacklists to identify contiguous, unused code blocks that meet specified capacity thresholds.
- [Documentation](/documentation/find-dormant-blocks.md)

### [Stability Harness](stability-harness.md)
* **Stability Campaign Plan Generator (`make-stability-plan.py`):** A data transformation utility that converts raw CSV binary audit sheets into structured JSON execution plans. It adaptively shifts between granular function-level testing arguments or broad full-module section overwrites based on available metadata.
    - [Documentation](\documentation\make-stability-plan.md)
* **Process Stability Campaign Harness (`stability-harness.py`):** An automated testing orchestration engine designed to execute and monitor the generated parametric process stability profiles. It launches target binaries, tracks the lazy loading of associated DLL runtime dependencies, integrates concurrent module-stomping companion tools, and captures kernel exit codes into structured JSONL logging manifests.
    - [Documentation](/documentation/stability-harness.md)

### [Process Profiles](process-profiles/README.md)
#### Summary
Compiled process profiles for common applications and Operating Systems. YMMY.
* [Windows Server 2025 DC](process-profiles/Windows-Server-2025-DC/README.md)