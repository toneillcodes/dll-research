## Frida Dynamic API Monitoring Telemetry Agent

This script is a dynamic instrumentation tool powered by **Frida**. It intercepts and logs the execution of exported functions from a specific target module (such as a Windows DLL) within a process. It is designed for reverse engineering, malware analysis, and API telemetry gathering.

### 🌟 Key Features
* **Automated Export Hooking:** Automatically enumerates and attaches hooks (`Interceptor.attach`) to every exported function of a specified module.
* **Smart De-duplication:** Tracks executed functions using a JavaScript `Set()` so that telemetry is only sent and logged the *first* time a unique function is called, preventing log bloat.
* **Flexible Execution Modes:** Can attach to a running process via PID or bootstrap a fresh, suspended instance (configured by default for Notepad++).
* **Child Process Gating:** Optional multi-process tracking capabilities to automatically catch, instrument, and resume spawned child processes.
* **JSON Lines (JSONL) Logging:** Streamlines analytical data logging directly to a structured `.jsonl` manifest file for easy post-analysis.

---

### ⚙️ How It Works
1. **Injection:** The script uses the Python `frida` API to inject a dynamic JavaScript payload into the target process.
2. **Hooking Loop:** The JS agent locates the specified module, pulls its export table, and wraps each function address with an `onEnter` callback.
3. **Telemetry Streaming:** When a hooked function is hit for the first time, an IPC event (`send()`) transmits the process ID, module, and function name back to the Python controller.
4. **Resilient Retries:** If a module hasn't been loaded into memory yet, the JS agent utilizes an asynchronous retry loop (`setTimeout`) until the target module becomes active.

---

### 🚀 Usage

#### Prerequisites
~~~bash
pip install frida argparse
~~~

#### Command Line Arguments
| Argument | Description | Required | Default |
| :--- | :--- | :--- | :--- |
| `-m`, `--module` | Name of the target module to instrument (e.g., `uxtheme.dll`). | **Yes** | N/A |
| `-p`, `--pid` | Target running process ID to attach to. If omitted, spawns a fresh instance. | No | `None` |
| `-l`, `--log` | Path where the output JSONL log manifest should be saved. | No | `active_telemetry.jsonl` |
| `--child-gating` | Enable automatic tracking and instrumentation of child processes. | No | `False` |

#### Examples

**Attach to an active process and track a specific DLL:**
~~~bash
python telemetry_agent.py -m uxtheme.dll -p 1234
~~~

**Spawn a new process instance with child process tracking and custom log output:**
~~~bash
python telemetry_agent.py -m kernel32.dll --child-gating -l my_analysis.jsonl
~~~

---

### 📊 Output Format
The resulting log file saves data in structured JSON Lines format, perfect for ingestion into SIEMs, pandas, or `jq`:

~~~json
{"pid": 1234, "module": "uxtheme.dll", "function": "InitUserTheme"}
{"pid": 1234, "module": "uxtheme.dll", "function": "GetThemeColor"}
~~~