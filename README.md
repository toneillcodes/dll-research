# dll-research
Windows Dynamic-Link Library research & offensive tooling.

## Blog Posts
* [Introduction to Module Stomping](https://medium.com/bugbountywriteup/an-introduction-to-module-stomping-26238af76d43)
* [Advanced Evasion Tradecraft: Precision Module Stomping](https://medium.com/@toneillcodes/advanced-evasion-tradecraft-precision-module-stomping-b51feb0978fe)

## Research and Projects

### API Monitor
#### Overview
Passively monitoring active APIs.

#### 🛠️ Tool Index
* **Frida Dynamic API Monitoring Telemetry Agent (`api-monitor.py`):** A dynamic instrumentation tool that injects a runtime JavaScript payload into target processes to automatically hook and track exported functions. It leverages asynchronous retry loops for deferred modules and filters out duplicate calls to stream clean, structured API telemetry directly to a JSONL log.
* **PE Dormant Export Analyzer (`find-dormant-blocks.py`):** A static analysis tool that parses a DLL's export table and SEH exception directories (`.pdata`) to map function boundaries linearly in memory. It cross-references these mappings against runtime telemetry and custom blacklists to identify contiguous, unused code blocks that meet specified capacity thresholds.

### Stability Harness
#### 🛠️ Tool Index
* **Stability Campaign Plan Generator:** A data transformation utility that converts raw CSV binary audit sheets into structured JSON execution plans. It adaptively shifts between granular function-level testing arguments or broad full-module section overwrites based on available metadata.

### Process Profiles
#### Summary
Compiled process profiles for common Operating Systems and applications. YMMV.