import argparse
import json
import sys
import os
import pefile

def get_pe_metadata(dll_path):
    """
    Parses the PE structure to extract function boundaries from the 
    exception unwind table (.pdata) and maps them to their export names.
    """
    metadata = {
        "pdata_sizes": {},
        "exports": [] # List of dicts: {"name": ..., "rva": ..., "size": ...}
    }
    try:
        pe = pefile.PE(dll_path, fast_load=False)
        
        # 1. Map function sizes via SEH Exception Directory
        exception_dir_idx = pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXCEPTION']
        if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > exception_dir_idx:
            exception_dir = pe.OPTIONAL_HEADER.DATA_DIRECTORY[exception_dir_idx]
            if exception_dir.VirtualAddress != 0 and exception_dir.Size != 0:
                pe.parse_data_directories(directories=[exception_dir_idx])
                if hasattr(pe, 'DIRECTORY_ENTRY_EXCEPTION'):
                    for entry in pe.DIRECTORY_ENTRY_EXCEPTION:
                        begin_rva = entry.struct.BeginAddress
                        end_rva = entry.struct.EndAddress
                        metadata["pdata_sizes"][begin_rva] = end_rva - begin_rva

        # 2. Extract all exported functions and pair them with sizes
        if hasattr(pe, 'DIRECTORY_ENTRY_EXPORT'):
            for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
                if exp.name:
                    name_str = exp.name.decode('utf-8', errors='ignore')
                    rva = exp.address
                    
                    # Lookup size from unwind data; fallback to minimum block if missing
                    size = metadata["pdata_sizes"].get(rva, 0)
                    if size == 0:
                        size = 64 
                        
                    metadata["exports"].append({
                        "name": name_str,
                        "rva": rva,
                        "size": size
                    })
                    
        # Sort exports linearly by their starting address in memory
        metadata["exports"].sort(key=lambda x: x["rva"])
        return metadata

    except Exception as e:
        print(f"[-] Error parsing PE metadata for {dll_path}: {e}")
        sys.exit(1)

def find_dormant_export_blocks(telemetry_file, unsafe_file, dll_path, payload_size):
    dll_name = os.path.basename(dll_path)
    
    print(f"[*] Parsing PE export table and unwind data linearly from: {dll_name}")
    pe_meta = get_pe_metadata(dll_path)
    
    if not pe_meta["exports"]:
        print("[-] Error: No exported functions found in the target DLL.")
        return

    # 1. Load active functions from telemetry
    excluded_functions = set()
    try:
        with open(telemetry_file, "r", encoding="utf-8") as f:
            for line in f:
                if line.strip():
                    entry = json.loads(line)
                    if entry.get("module", "").lower() == dll_name.lower():
                        excluded_functions.add(entry.get("function"))
    except FileNotFoundError:
        print(f"[-] Telemetry file '{telemetry_file}' not found.")
        sys.exit(1)

    print(f"[*] Loaded telemetry: {len(excluded_functions)} active functions identified.")

    # 2. Load explicitly unsafe APIs to exclude
    if unsafe_file:
        try:
            unsafe_count = 0
            with open(unsafe_file, "r", encoding="utf-8") as f:
                for line in f:
                    api_name = line.strip()
                    if api_name:
                        excluded_functions.add(api_name)
                        unsafe_count += 1
            print(f"[*] Loaded unsafe list: Added {unsafe_count} functions to exclusion set.")
        except FileNotFoundError:
            print(f"[-] Unsafe APIs file '{unsafe_file}' not found.")
            sys.exit(1)

    # 3. Linearly walk the sorted exports to find contiguous blocks of unused functions
    current_block = []
    valid_blocks = []
    
    for exp in pe_meta["exports"]:
        # An export is blocked if it is in the active telemetry OR explicitly marked unsafe
        is_excluded = exp["name"] in excluded_functions
        
        if not is_excluded:
            # Function is clean/dormant; append it to the current contiguous chain
            current_block.append(exp)
        else:
            # Function is active or unsafe; the current contiguous chain is broken. Evaluate it.
            if current_block:
                total_block_size = sum(f["size"] for f in current_block)
                if total_block_size >= payload_size:
                    valid_blocks.append((list(current_block), total_block_size))
                current_block = [] # Reset for next gap

    # Catch any remaining block trailing at the very end of the export table
    if current_block:
        total_block_size = sum(f["size"] for f in current_block)
        if total_block_size >= payload_size:
            valid_blocks.append((current_block, total_block_size))

    # Output Results
    print("\n" + "="*75)
    print(f"[*] CONTIGUOUS DORMANT EXPORT ANALYSIS FOR: {dll_name}")
    print(f"[*] Target Capacity Requirement: {payload_size} bytes")
    print("="*75)

    if not valid_blocks:
        print("[-] No blocks match your size and safety criteria.")
    else:
        print(f"[+] Found {len(valid_blocks)} clusters matching criteria:\n")
        
        for idx, (block, total_size) in enumerate(valid_blocks, start=1):
            start_f = block[0]
            end_f = block[-1]
            
            print(f"Cluster #{idx}: Total Capacity: {total_size} bytes")
            print(f"  --> Span: {hex(start_f['rva'])} to {hex(end_f['rva'] + end_f['size'])}")
            print(f"  --> Contains {len(block)} adjacent unused/safe exports:")
            
            # Print the functions that make up this specific safe zone
            for f in block:
                print(f"     [+] {f['name']} ({f['size']} bytes @ {hex(f['rva'])})")
            print("-" * 75)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Find groups of contiguous unused exports while filtering out unsafe entries.")
    parser.add_argument("-t", "--telemetry", required=True, help="Path to the active telemetry JSONL file.")
    parser.add_argument("-u", "--unsafe", required=False, help="Path to the newline-delimited file containing unsafe APIs to exclude.")
    parser.add_argument("-d", "--dll", required=True, help="Absolute path to the disk target DLL.")
    parser.add_argument("-s", "--size", type=int, required=True, help="Minimum continuous block size in bytes.")

    args = parser.parse_args()
    find_dormant_export_blocks(args.telemetry, args.unsafe, args.dll, args.size)