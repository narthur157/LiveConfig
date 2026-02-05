import json
import os
import csv
import argparse

def collect_config_data(source_dir):
    """
    Recursively walks through source_dir and collects data from all .json files.
    Returns a dictionary mapping property names to their data.
    """
    config_data = {}
    if not os.path.exists(source_dir):
        print(f"Error: Source directory {source_dir} does not exist.")
        return config_data

    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if file.endswith('.json'):
                file_path = os.path.join(root, file)
                
                # Calculate expected property name based on path
                rel_path = os.path.relpath(file_path, source_dir)
                expected_name = os.path.splitext(rel_path)[0].replace(os.sep, '.')
                
                try:
                    with open(file_path, 'r') as f:
                        data = json.load(f)
                        
                        # Validate propertyName matches path
                        actual_name = expected_name
                        if 'propertyName' in data:
                            actual_name_field = data['propertyName']
                            # propertyName can be a string or a dict with a propertyName field
                            if isinstance(actual_name_field, dict):
                                actual_name = actual_name_field.get('propertyName', expected_name)
                            else:
                                actual_name = actual_name_field

                            if actual_name != expected_name:
                                print(f"Warning: Property name mismatch in {file_path}. Expected: '{expected_name}', Found: '{actual_name}'")
                        else:
                            print(f"Warning: Missing 'propertyName' field in {file_path}. Expected: '{expected_name}'")
                        
                        config_data[actual_name] = data
                except Exception as e:
                    print(f"Warning: Failed to read {file_path}: {e}")
    
    return config_data

def export_to_json(data, output_path):
    """
    Exports the collected data to a single JSON file.
    """
    try:
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=4)
        print(f"Successfully exported to JSON: {output_path}")
    except Exception as e:
        print(f"Error: Failed to export to JSON: {e}")

def export_to_csv(data, output_path):
    """
    Exports the collected data to a CSV file.
    """
    if not data:
        print("No data to export to CSV.")
        return

    # Define requested headers and their mapping to JSON fields
    headers = ["Name", "Type", "Value", "Tags", "Description"]
    field_mapping = {
        "Name": "propertyName",
        "Type": "propertyType",
        "Value": "value",
        "Tags": "tags",
        "Description": "description"
    }
    
    try:
        with open(output_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=headers)
            writer.writeheader()
            
            for property_name, entry in data.items():
                row = {}
                for header, json_field in field_mapping.items():
                    value = entry.get(json_field, "")
                    
                    # Special handling for propertyName if it's a dict (as seen in some files)
                    if json_field == "propertyName":
                        if isinstance(value, dict):
                            value = value.get("propertyName", property_name)
                        elif not value:
                            value = property_name

                    if isinstance(value, list):
                        row[header] = ';'.join(map(str, value))
                    elif isinstance(value, dict):
                        row[header] = json.dumps(value)
                    else:
                        row[header] = value
                writer.writerow(row)
        print(f"Successfully exported to CSV: {output_path}")
    except Exception as e:
        print(f"Error: Failed to export to CSV: {e}")

def main():
    # Calculate base directory relative to this script: ../../../
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))

    parser = argparse.ArgumentParser(description="Export Live Config JSON files to a combined JSON or CSV.")
    parser.add_argument('--source', default=os.path.join(project_root, 'Config', 'LiveConfig'), help="Directory containing live config JSON files.")
    parser.add_argument('--output', required=True, help="Path for the output file.")
    parser.add_argument('--format', choices=['json', 'csv'], help="Export format (json or csv). If not specified, inferred from output extension.")

    args = parser.parse_args()

    source_dir = args.source
    output_path = args.output
    format_type = args.format

    if not format_type:
        if output_path.lower().endswith('.json'):
            format_type = 'json'
        elif output_path.lower().endswith('.csv'):
            format_type = 'csv'
        else:
            print("Error: Could not infer format from output path extension. Please use --format.")
            return

    print(f"Collecting data from {source_dir}...")
    data = collect_config_data(source_dir)
    print(f"Collected {len(data)} properties.")

    if format_type == 'json':
        export_to_json(data, output_path)
    elif format_type == 'csv':
        export_to_csv(data, output_path)

if __name__ == "__main__":
    main()
