# API Monitor
## Overview
Monitoring active APIs.

## Index
* [`api-monitor.py`](api-monitor.py)
    - **Frida Dynamic API Monitoring Telemetry Agent**: A dynamic instrumentation tool that injects a runtime JavaScript payload into target processes to automatically hook and track exported functions. It leverages asynchronous retry loops for deferred modules and filters out duplicate calls to stream clean, structured API telemetry directly to a JSONL log.
* [`find-dormant-blocks.py`](find-dormant-blocks.py)
    - **PE Dormant Export Analyzer**: A static analysis tool that parses a DLL's export table and SEH exception directories (`.pdata`) to map function boundaries linearly in memory. It cross-references these mappings against runtime telemetry and custom blacklists to identify contiguous, unused code blocks that meet specified capacity thresholds.
    - [Documentation](/documentation/find-dormant-blocks.md)