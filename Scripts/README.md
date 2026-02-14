# LiveConfig Scripts

This directory contains Python scripts for managing LiveConfig data. These scripts are designed to work with the LiveConfig Unreal Engine plugin, allowing for easy import, export, and synchronization of configuration properties.

## Scripts Overview

### 1. ExportLiveConfig.py
Combines LiveConfig JSON files into a single consolidated file (JSON or CSV).

**Usage:**
```bash
python ExportLiveConfig.py --output path/to/output.csv
```

**Arguments:**
* `--source`: (Optional) Directory containing live config JSON files. Defaults to `Config/LiveConfig` relative to the project root.
* `--output`: (Required) Path for the output file.
* `--format`: (Optional) Export format (`json` or `csv`). If not specified, it is inferred from the output file extension.

---

### 2. SyncLiveConfigToSheets.py
Synchronizes LiveConfig data directly to a Google Sheets spreadsheet.
This uses ExportLiveConfig.py.

**Requirements:**
* `gspread` and `google-auth` Python libraries.
* A Google Service Account with access to the target spreadsheet.

**Usage:**
```bash
python SyncLiveConfigToSheets.py --spreadsheet-id YOUR_SHEET_ID --creds-file path/to/service_account.json
```

**Arguments:**
* `--spreadsheet-id`: (Required) The ID of the Google Spreadsheet.
* `--sheet-name`: (Optional) The name of the tab to sync to. Defaults to `Data`.
* `--source`: (Optional) Directory containing live config JSON files. Defaults to `Config/LiveConfig`.
* `--creds-file`: (Optional) Path to the Google service account JSON key file. Alternatively, set the `GOOGLE_APPLICATION_CREDENTIALS_JSON` environment variable.

## Setup
It is recommended to use a virtual environment to manage dependencies for these scripts.

```bash
pip install gspread google-auth
```
