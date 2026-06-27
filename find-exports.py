import os
import csv
import argparse
import pefile

def load_modules_from_csv(file_path):
    """Loads and sanitizes module filter names from the 'Name' column of a CSV file."""
    if not os.path.exists(file_path):
        print(f"[ERROR] Specified module filter file not found: {file_path}")
        return None
        
    try:
        with open(file_path, 'r', newline='', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            
            # Verify that the 'Name' column exists in the CSV header
            if 'Name' not in reader.fieldnames:
                print(f"[ERROR] Critical header missing. Could not locate 'Name' column in: {file_path}")
                return None
                
            # Extract names, strip whitespace, normalize to lowercase, and skip blanks
            modules = set()
            for row in reader:
                name_val = row.get('Name')
                if name_val and name_val.strip():
                    modules.add(name_val.strip().lower())
                    
            return modules
    except Exception as e:
        print(f"[ERROR] Failed reading module filter CSV: {e}")
        return None

def search_dll_exports(target_directory, target_exports, module_file_path=None, output_csv_path=None):
    """
    Walks through a directory, parses DLL export tables, and searches for specific export names.
    If matches are found, logs metadata and optionally exports them in a standard CSV schema.
    """
    target_set = set(target_exports)
    module_filter = load_modules_from_csv(module_file_path) if module_file_path else None
    
    print(f"Scanning '{target_directory}' for exports: {list(target_set)}")
    if module_filter:
        print(f"Filtering search using modules listed in CSV: {module_file_path}")
        print(f"Active filter list ({len(module_filter)} entries): {list(module_filter)}\n")
    else:
        print("Scanning all available DLLs globally...\n")
        
    matched_records = []

    for root, dirs, files in os.walk(target_directory):
        for file in files:
            if file.lower().endswith('.dll'):
                # Short-circuit parsing if a filter is active and the file isn't on the list
                if module_filter and file.lower() not in module_filter:
                    continue
                    
                dll_path = os.path.join(root, file)
                dll_name = os.path.basename(dll_path)
                
                try:
                    # Full parse needed to pull structural data directory sizing
                    pe = pefile.PE(dll_path, fast_load=False)
                    
                    # Extract .text boundaries for the stability blueprint structure
                    text_size = 0
                    for section in pe.sections:
                        sec_name = section.Name.decode('utf-8', errors='ignore').strip('\x00')
                        if sec_name == '.text':
                            text_size = section.Misc_VirtualSize if section.Misc_VirtualSize else section.SizeOfRawData
                            break

                    # Map structural sizes from runtime exception directory tables
                    pdata_sizes = {}
                    exception_dir_idx = pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXCEPTION']
                    if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > exception_dir_idx:
                        exception_dir = pe.OPTIONAL_HEADER.DATA_DIRECTORY[exception_dir_idx]
                        if exception_dir.VirtualAddress != 0 and exception_dir.Size != 0:
                            pe.parse_data_directories(directories=[exception_dir_idx])
                            if hasattr(pe, 'DIRECTORY_ENTRY_EXCEPTION'):
                                for entry in pe.DIRECTORY_ENTRY_EXCEPTION:
                                    pdata_sizes[entry.struct.BeginAddress] = entry.struct.EndAddress - entry.struct.BeginAddress

                    if hasattr(pe, 'DIRECTORY_ENTRY_EXPORT'):
                        for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
                            if exp.name:
                                export_name = exp.name.decode('utf-8', errors='ignore')
                                
                                if export_name in target_set:
                                    is_forwarded = bool(exp.forwarder)
                                    rva = exp.address if exp.address else 0
                                    calculated_size = pdata_sizes.get(rva, 64) if not is_forwarded else 0
                                    
                                    print(f"[MATCH] Found '{export_name}' in: {dll_path} (RVA: {hex(rva)})")
                                    
                                    # Collect the structured metrics matching dump-exports schema
                                    matched_records.append({
                                        'ModuleName': dll_name,
                                        'FullTextSectionSize': text_size,
                                        'FunctionName': export_name,
                                        'Offset': hex(rva),
                                        'FuncSize': calculated_size,
                                        'IsForwarded': is_forwarded
                                    })
                                    
                except pefile.PEFormatError:
                    continue
                except Exception as e:
                    print(f"[ERROR] Could not parse {dll_path}: {e}")

    # Write out data to file if requested and matches occurred
    if output_csv_path:
        if not matched_records:
            print("\n[*] Operational Note: No matching exports discovered. CSV generation skipped.")
            return

        fieldnames = ['ModuleName', 'FullTextSectionSize', 'FunctionName', 'Offset', 'FuncSize']
        try:
            with open(output_csv_path, mode='w', newline='', encoding='utf-8') as f:
                writer = csv.DictWriter(f, fieldnames=fieldnames)
                writer.writeheader()
                for record in matched_records:
                    if record['IsForwarded']:
                        continue  # Keep identical parity with execution path B of dump-exports
                    writer.writerow({
                        'ModuleName': record['ModuleName'],
                        'FullTextSectionSize': record['FullTextSectionSize'],
                        'FunctionName': record['FunctionName'],
                        'Offset': record['Offset'],
                        'FuncSize': record['FuncSize']
                    })
            print(f"\n[+] Stability blueprint successfully committed to: '{output_csv_path}'")
        except Exception as e:
            print(f"\n[ERROR] Failed to save CSV output manifest: {e}")

if __name__ == "__main__":
    # Set up argument parsing
    parser = argparse.ArgumentParser(
        description="Search for specific exported functions within DLLs and export to a stability blueprint schema."
    )
    
    # Switch-based required options
    parser.add_argument(
        "-d", "--directory", 
        type=str, 
        required=True,
        help="The root directory to start scanning for DLLs."
    )
    parser.add_argument(
        "-e", "--exports", 
        type=str, 
        nargs="+", 
        required=True,
        help="One or more export names to search for (space-separated)."
    )
    
    # Optional filtering flag (expects path to a CSV file with a 'Name' header column)
    parser.add_argument(
        "-m", "--modules",
        type=str,
        default=None,
        help="Path to a CSV file containing target module names under a 'Name' column header."
    )

    # Output file generation matching dump-exports paradigm
    parser.add_argument(
        "-o", "--output",
        type=str,
        default=None,
        help="Enable pipeline CSV generation path and specify output location."
    )
    
    args = parser.parse_args()
    
    # Run the search with the provided arguments
    search_dll_exports(
        target_directory=args.directory, 
        target_exports=args.exports, 
        module_file_path=args.modules,
        output_csv_path=args.output
    )