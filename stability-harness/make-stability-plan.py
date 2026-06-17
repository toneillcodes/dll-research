import argparse
import csv
import json
import sys

DEFAULT_TIMEOUT = 60

def transform_data(csv_file_path, json_file_path, stomp_path, timeout_seconds):
    transformed_data = []

    try:
        with open(csv_file_path, mode="r", encoding="utf-8") as csv_file:
            csv_reader = csv.DictReader(csv_file)

            for row in csv_reader:
                target_process = row.get("TargetProcess", "").strip()
                module_name = row.get("ModuleName", "").strip()
                full_text_size = row.get("FullTextSectionSize", "").strip()
                func_name = row.get("FunctionName", "").strip()
                offset = row.get("Offset", "").strip()
                func_size = row.get("FuncSize", "").strip()

                if not target_process or not module_name:
                    continue

                # Initialize standard base parameters
                cmd_args = ["-p", "{pid}", "-d", module_name]

                # --- CONDITIONAL FLEXIBILITY SWITCH ---
                if func_name:
                    # CASE 1: Precise function-granular target campaign
                    cmd_args += ["-f", func_name, "-s", func_size]
                else:
                    # CASE 2: Fallback to Full Module Stomp Campaign
                    # Uses the full text section size and excludes function/offset flags
                    if full_text_size and full_text_size != "0":
                        cmd_args += ["-s", full_text_size]
                    else:
                        print(f"[-] Warning: Skipping empty function row for {module_name} due to missing FullTextSectionSize.")
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
    parser = argparse.ArgumentParser(description="Generate adaptive module-stomping plans dynamically from CSV layouts.")
    parser.add_argument("-i", "--input", required=True, help="Path to input targets CSV file.")
    parser.add_argument("-o", "--output", required=True, help="Destination stability campaign configuration json.")
    parser.add_argument("-s", "--stomp-path", required=True, help="Absolute path to the stomp engineering utility.")
    parser.add_argument("-t", "--timeout", type=int, default=DEFAULT_TIMEOUT, help="Timeout value per execution step.")

    args = parser.parse_args()
    transform_data(args.input, args.output, args.stomp_path, args.timeout)