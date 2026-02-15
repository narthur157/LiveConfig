import os
import json
import argparse
import gspread
from google.oauth2.service_account import Credentials
from ExportLiveConfig import collect_config_data

def sync_to_google_sheets(spreadsheet_id, sheet_name, source_dir, credentials_path=None, credentials_json=None):
    """
    Collects live config data and uploads it to the specified Google Sheet tab.
    """
    # Collect Data
    print(f"Collecting data from {source_dir}...")
    data = collect_config_data(source_dir)
    print(f"Collected {len(data)} properties.")

    if not data:
        print("No data found to sync.")
        return

    # Prepare Data for Sheets
    # Columns: Name, Type, Value, Tags, Description
    headers = ["Name", "Type", "Value", "Tags", "Description"]
    rows = [headers]
    
    # Sort by name for consistency
    for property_name in sorted(data.keys()):
        entry = data[property_name]
        
        # Extract fields (matching ExportLiveConfig.py's CSV logic)
        prop_name = entry.get('propertyName', property_name)
        if isinstance(prop_name, dict):
            prop_name = prop_name.get('propertyName', property_name)
        
        prop_type = entry.get('propertyType', "")
        value = entry.get('value', "")
        tags = entry.get('tags', [])
        description = entry.get('description', "")

        # Format list/dict fields
        tags_str = ';'.join(map(str, tags)) if isinstance(tags, list) else str(tags)
        if isinstance(value, dict):
            value = json.dumps(value)
        
        rows.append([prop_name, prop_type, value, tags_str, description])

    # Authenticate with Google
    print("Authenticating with Google Sheets API...")
    scopes = [
        'https://www.googleapis.com/auth/spreadsheets',
        'https://www.googleapis.com/auth/drive'
    ]

    try:
        if credentials_json:
            # Use JSON string from environment variable
            creds_dict = json.loads(credentials_json)
            creds = Credentials.from_service_account_info(creds_dict, scopes=scopes)
        elif credentials_path and os.path.exists(credentials_path):
            # Use credentials file
            creds = Credentials.from_service_account_file(credentials_path, scopes=scopes)
        else:
            raise ValueError("No valid Google credentials provided. Use --creds-file or GOOGLE_APPLICATION_CREDENTIALS_JSON env var.")

        client = gspread.authorize(creds)
    except Exception as e:
        print(f"Authentication Error: {e}")
        return

    # Update the Spreadsheet
    try:
        print(f"Opening spreadsheet: {spreadsheet_id}...")
        spreadsheet = client.open_by_key(spreadsheet_id)
        
        # Find or create the worksheet
        try:
            worksheet = spreadsheet.worksheet(sheet_name)
            print(f"Found existing tab: '{sheet_name}'")
        except gspread.exceptions.WorksheetNotFound:
            print(f"Creating new tab: '{sheet_name}'")
            worksheet = spreadsheet.add_worksheet(title=sheet_name, rows="100", cols="5")

        # Clear and update
        print(f"Updating '{sheet_name}' with {len(rows)-1} rows of data...")
        worksheet.clear()
        worksheet.update(values=rows, range_name='A1')
        
        # Optional: Formatting (bold headers)
        worksheet.format('A1:E1', {'textFormat': {'bold': True}})
        
        print("Sync complete!")
    except Exception as e:
        print(f"Sync Error: {e}")

def main():
    # Calculate base directory relative to this script: ../../../ (assume we're coming from Plugins/LiveConfig/Scripts)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))

    parser = argparse.ArgumentParser(description="Sync Live Config data to a Google Sheets tab.")
    parser.add_argument('--spreadsheet-id', required=True, help="The ID of the Google Spreadsheet.")
    parser.add_argument('--sheet-name', default="Data", help="The name of the tab to sync to (default: 'Data').")
    parser.add_argument('--source', default=os.path.join(project_root, 'Config', 'LiveConfig'), help="Directory containing live config JSON files.")
    parser.add_argument('--creds-file', help="Path to the Google service account JSON key file.")
    
    args = parser.parse_args()

    # Get credentials from env var if not provided as file
    creds_json = os.environ.get('GOOGLE_APPLICATION_CREDENTIALS_JSON')

    sync_to_google_sheets(
        spreadsheet_id=args.spreadsheet_id,
        sheet_name=args.sheet_name,
        source_dir=args.source,
        credentials_path=args.creds_file,
        credentials_json=creds_json
    )

if __name__ == "__main__":
    main()
