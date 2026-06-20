# Make Stability Plan

## Stability Campaign Plan Generator

This script is a automation pipeline utility designed to ingest raw CSV target lists (generated during binary auditing or dormant export analysis) and convert them into structured JSON stability profiles. These profiles are explicitly formatted for consumption by the process stability test harness.

### 🌟 Key Features
* **CSV-to-JSON Pipeline Automation:** Translates flat tabular target data directly into nested, deployment-ready diagnostic profiles.
* **Adaptive Fallback Logic:** Features an intelligent conditional switch that automatically toggles testing strategy based on row metadata completeness:
  * **Case 1 (Function-Granular):** Targets specific function names, sizes, and offsets when detailed export metadata exists.
  * **Case 2 (Module Stomp Fallback):** Falls back to broad, full text-section overwrites using overall section dimensions if individual function fields are missing.
* **Dynamic Command Generation:** Auto-constructs precision command-line arguments (including runtime parameters like `{pid}`) required by your lower-level testing utilities.
* **Defensive Data Sanitation:** Dynamically strips whitespace anomalies, skips corrupt rows lacking essential process/module handles, and issues descriptive console warnings for incomplete entries.

---

### ⚙️ How It Works
1. **Parsing:** The utility parses the input CSV file sequentially using a dictionary reader map (`csv.DictReader`).
2. **Evaluation:** It runs defensive checks to ensure both `TargetProcess` and `ModuleName` values are valid before proceeding.
3. **Compilation:** The script checks for a `FunctionName`. If found, it appends granular target flags (`-f`, `-s`). If absent, it queries `FullTextSectionSize` to implement a macro-level evaluation strategy.
4. **Exporting:** It bundles the resulting command arguments, custom timeout thresholds, and utility toolpaths into a neatly indented JSON manifest ready for production test runs.

---

### 🚀 Usage

#### Prerequisites
~~~bash
# Utilizes Python standard libraries (csv, json, sys, argparse)
python make-stability-plan.py --help
~~~

#### Command Line Arguments
| Argument | Description | Required | Default |
| :--- | :--- | :--- | :--- |
| `-i`, `--input` | Path to the source audit CSV schema containing raw target data. | **Yes** | N/A |
| `-o`, `--output` | Destination path where the output JSON plan should be saved. | **Yes** | N/A |
| `-s`, `--stomp-path` | Absolute path to the secondary evaluation tool/utility. | **Yes** | N/A |
| `-t`, `--timeout` | Maximum duration boundary (in seconds) assigned to each run. | No | `60` |

#### Expected Input CSV Layout (`targets.csv`)
~~~csv
TargetProcess,ModuleName,FullTextSectionSize,FunctionName,Offset,FuncSize
notepad++.exe,uxtheme.dll,145152,InitUserTheme,0x1020,128
winword.exe,custom_dep.dll,512000,,,
~~~
*Note: Row 2 shows the fallback mode triggers cleanly because individual function entries are left empty.*

#### Execution Example
~~~bash
python make-stability-plan.py -i audit_results.csv -o plans/stability_matrix.json -s "C:\Tools\stomp_util.exe" -t 45
~~~

---

### 📊 Output Format
The resulting file transforms structural parameters into clean JSON matrices matching your stability harness specifications:

~~~json
[
  {
    "target_executable": "notepad++.exe",
    "trigger_dll": "uxtheme.dll",
    "secondary_tool": "C:\\Tools\\stomp_util.exe",
    "tool_arguments": ["-p", "{pid}", "-d", "uxtheme.dll", "-f", "InitUserTheme", "-s", "128", "-n"],
    "timeout_seconds": 45
  },
  {
    "target_executable": "winword.exe",
    "trigger_dll": "custom_dep.dll",
    "secondary_tool": "C:\\Tools\\stomp_util.exe",
    "tool_arguments": ["-p", "{pid}", "-d", "custom_dep.dll", "-s", "512000", "-n"],
    "timeout_seconds": 45
  }
]
~~~