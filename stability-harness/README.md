# Stability Harness
## Overview
Testing harness to evaluate the stability of an application.

## Index
* [`make-stability-plan.py`](make-stability-plan.py)
    - **Stability Campaign Plan Generator**: A data transformation utility that converts raw CSV binary audit sheets into structured JSON execution plans. It adaptively shifts between granular function-level testing arguments or broad full-module section overwrites based on available metadata.
    - [Documentation](/documentation/make-stability-plan.md)
* [`stability-harness.py`](stability-plan.py)
    - **Process Stability Campaign Harness**: An automated testing orchestration engine designed to execute and monitor the generated parametric process stability profiles. It launches target binaries, tracks the lazy loading of associated DLL runtime dependencies, integrates concurrent module-stomping companion tools, and captures kernel exit codes into structured JSONL logging manifests. 
    - [Documentation](/documentation/stability-harness.md)