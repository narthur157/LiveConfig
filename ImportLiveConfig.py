import json
import os
import sys

def infer_property_type(value):
    if isinstance(value, bool):
        return "Bool"
    if isinstance(value, int):
        return "Int"
    if isinstance(value, float):
        return "Float"
    return "String"

def import_live_config(input_file, output_dir):
    if not os.path.exists(input_file):
        print(f"Error: Input file {input_file} not found.")
        return

    print(f"Reading {input_file}...")
    try:
        with open(input_file, 'r') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error: Failed to parse {input_file}: {e}")
        return

    # Ensure output directory exists
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    properties_count = 0
    if isinstance(data, dict):
        for key, entry in data.items():
            if not isinstance(entry, dict):
                # Simple key-value fallback
                prop_def = {
                    "propertyName": {"propertyName": key},
                    "description": "",
                    "propertyType": infer_property_type(entry),
                    "tags": [],
                    "value": str(entry)
                }
            else:
                # Use entry data
                val = entry.get("Value", "")
                
                # Sample data uses "Type" as a category, we'll treat it as a tag
                tags = entry.get("Tags", [])
                category_type = entry.get("Type")
                if category_type and category_type not in tags:
                    tags.append(category_type)

                prop_def = {
                    "propertyName": {"propertyName": key},
                    "description": entry.get("Description", ""),
                    "propertyType": infer_property_type(val),
                    "tags": tags,
                    "value": str(val).lower() if isinstance(val, bool) else str(val)
                }
            
            # Handle property name dots as directory separators
            relative_path = key.replace('.', '/') + '.json'
            full_path = os.path.join(output_dir, relative_path)
            
            os.makedirs(os.path.dirname(full_path), exist_ok=True)
            
            with open(full_path, 'w') as f:
                json.dump(prop_def, f, indent=4)
            
            properties_count += 1
            if properties_count % 100 == 0:
                print(f"Processed {properties_count} properties...")

    print(f"Successfully imported {properties_count} properties to {output_dir}")

if __name__ == "__main__":
    # Default paths based on project structure
    default_input = os.path.join("Config", "LiveConfig.json")
    default_output = os.path.join("Config", "LiveConfig")
    
    input_path = sys.argv[1] if len(sys.argv) > 1 else default_input
    output_path = sys.argv[2] if len(sys.argv) > 2 else default_output
    
    import_live_config(input_path, output_path)
