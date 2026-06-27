import argparse
import csv
import json
import sys

DEFAULT_TIMEOUT = 60

def transform_data(csv_file_path, json_file_path, stomp_path, timeout_seconds, process_name):
    transformed_data = []

    try:
        with open(csv_file_path, mode="r", encoding="utf-8") as csv_file:
            # Read the first line to sniff out the field headers automatically
            sample = csv_file.readline()
            csv_file.seek(0)
            
            headers = [h.strip() for h in sample.split(",")]
            
            # --- AUTO-DETECTION MATRIX ---
            keys = {
                "Name": "Name",
                "TextSectionSize": "TextSectionSize",
                "FunctionName": "FunctionName",
                "Offset": "Offset",
                "FuncSize": "FuncSize"
            }
            
            if "TargetProcess" in headers:
                print("[+] Auto-detected 'list-process-dlls' input profile schema.")
                # Uses default mapping keys
            elif "FuncSize" in headers:
                print("[+] Auto-detected 'dump-exports' input profile schema.")
                # Uses default mapping keys tailored by dump-exports layout
                keys["Name"] = "ModuleName"
                keys["TextSectionSize"] = "FullTextSectionSize"                
            else:
                print("[-] Warning: Unrecognized CSV header format. Attempting fallback mapping parsing...")

            csv_reader = csv.DictReader(csv_file)

            for row in csv_reader:
                # Safely normalize the target process name if it was supplied
                target_process = process_name.strip() if process_name else None
                
                module_name = row.get(keys["Name"], "").strip()
                full_text_size = row.get(keys["TextSectionSize"], "").strip()
                func_name = row.get(keys["FunctionName"], "").strip()
                offset = row.get(keys["Offset"], "").strip()
                func_size = row.get(keys["FuncSize"], "").strip()

                if not module_name:
                    continue

                if module_name.endswith(".exe"):
                    continue

                # Initialize standard base parameters
                cmd_args = ["-p", "{pid}", "-d", module_name]

                # --- CONDITIONAL FLEXIBILITY SWITCH ---
                if func_name:
                    if func_name.lower().startswith("ordinal_"):
                        # CASE 1A: Anonymous Ordinal Export - Shift to structural offset flag
                        cmd_args += ["-o", offset, "-s", func_size]
                    else:
                        # CASE 1B: Precise function-granular target campaign
                        cmd_args += ["-f", func_name, "-s", func_size]
                elif offset and func_size:
                    # If function name is missing but offset and size exist, use the raw offset directly
                    #cmd_args += ["-o", offset, "-s", func_size]
                    cmd_args += ["-o", offset]
                else:
                    # CASE 2: Fallback to Full Module Stomp Campaign (from list-process-dlls)
                    if full_text_size and full_text_size != "0":
                        cmd_args += ["-s", full_text_size]
                    else:
                        print(f"[-] Warning: Skipping empty function row for {module_name} due to missing Target Parameters.")
                        continue

                # Add standard operational switch flags
                cmd_args.append("-n")

                json_entry = {
                    "target_executable": target_process, 
                    "trigger_dll": module_name,
                    "secondary_tool": stomp_path,
                    "tool_arguments": cmd_args,
                    "timeout_seconds": timeout_seconds,
                }

                transformed_data.append(json_entry)

        with open(json_file_path, mode="w", encoding="utf-8") as json_file:
            json.dump(transformed_data, json_file, indent=2)

        print(f"Success! Generated {len(transformed_data)} campaigns in the stability plan.")

    except FileNotFoundError:
        print(f"Error: The file '{csv_file_path}' was not found.")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate adaptive module-stomping plans dynamically from multi-format tool outputs.")
    parser.add_argument("-i", "--input", required=True, help="Path to input targets CSV file (from list-process-dlls or dump-exports).")
    parser.add_argument("-o", "--output", required=True, help="Destination stability campaign configuration json.")
    parser.add_argument("-s", "--stomp-path", required=True, help="Absolute path to the stomp engineering utility.")
    parser.add_argument("-p", "--process", default=None, help="Optional: Target application executable path to use as target_executable.")
    parser.add_argument("-t", "--timeout", type=int, default=DEFAULT_TIMEOUT, help="Timeout value per execution step.")

    args = parser.parse_args()
    transform_data(args.input, args.output, args.stomp_path, args.timeout, args.process)