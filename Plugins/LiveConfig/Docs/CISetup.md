# (Optional) CI/CD Setup for LiveConfig Sync

This document explains how to integrate the `SyncLiveConfigToSheets.py` utility into various CI/CD pipelines to automatically sync your LiveConfig data to a Google Sheet whenever changes are committed. This makes it easier to see your data outside of the editor and add data validation to overrides.

This plugin may be used without sheets entirely, or without updating the sheets with the base data. The data can also be updated manually, or by polling a publicly accessible CSV file from Apps Script.

This is the most robust method, but requires some setup. If experimenting with the plugin, it is recommended to export your data as CSV and import manually to your favorite tabular spreadsheet editor.

## Prerequisites

Before setting up any CI pipeline, you need:

1.  **Google Service Account**:
    *   Create a project in the [Google Cloud Console](https://console.cloud.google.com/).
    *   Enable the **Google Sheets API** and **Google Drive API**.
    *   Create a **Service Account** and download the **JSON Key file**.
    *   **Share your Google Sheet** with the service account's email address (found in the JSON key) with "Editor" permissions.
2.  **Spreadsheet ID**:
    *   The ID is the long string in the Sheet's URL: `https://docs.google.com/spreadsheets/d/SPREADSHEET_ID/edit`.
3.  **Python Environment**:
    *   The CI runner must have Python installed.
    *   Required packages: `pip install gspread google-auth`.

---

## GitHub Actions

Create a file named `.github/workflows/sync-config.yml`:

```yaml
name: Sync LiveConfig to Sheets

on:
  push:
    paths:
      - 'Config/LiveConfig/**'
    branches:
      - main # or your default branch

jobs:
  sync:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout Repository
        uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.x'

      - name: Install Dependencies
        run: |
          pip install gspread google-auth

      - name: Sync to Google Sheets
        env:
          GOOGLE_APPLICATION_CREDENTIALS_JSON: ${{ secrets.GCP_SERVICE_ACCOUNT_KEY }}
        run: |
          python SyncLiveConfigToSheets.py --spreadsheet-id "YOUR_SPREADSHEET_ID_HERE"
```

**Note**: Add the content of your Service Account JSON file as a Repository Secret named `GCP_SERVICE_ACCOUNT_KEY`.

---

## GitLab CI

Add the following to your `.gitlab-ci.yml`:

```yaml
sync_live_config:
  stage: deploy
  image: python:3.9
  only:
    changes:
      - Config/LiveConfig/**/*
    refs:
      - main
  variables:
    SPREADSHEET_ID: "YOUR_SPREADSHEET_ID_HERE"
  script:
    - pip install gspread google-auth
    - python SyncLiveConfigToSheets.py --spreadsheet-id "$SPREADSHEET_ID"
```

**Note**: Go to **Settings > CI/CD > Variables** and add `GOOGLE_APPLICATION_CREDENTIALS_JSON` with the content of your service account JSON.

---

## Jenkins

1.  **Credentials**: Add a "Secret text" credential containing the content of your Google Service Account JSON. Give it an ID like `gcp-liveconfig-key`.
2.  **Pipeline**: Use the following snippet in your `Jenkinsfile`:

```groovy
pipeline {
    agent any
    environment {
        SPREADSHEET_ID = 'YOUR_SPREADSHEET_ID_HERE'
        GCP_KEY = credentials('gcp-liveconfig-key')
    }
    stages {
        stage('Sync Config') {
            steps {
                sh '''
                    pip install gspread google-auth
                    export GOOGLE_APPLICATION_CREDENTIALS_JSON="$GCP_KEY"
                    python SyncLiveConfigToSheets.py --spreadsheet-id "$SPREADSHEET_ID"
                '''
            }
        }
    }
}
```

---

## Unreal Engine BuildGraph

If you are using BuildGraph for your build automation, you can trigger the sync as a post-build step or a standalone task.

```xml
<Node Name="Sync Config to Sheets">
    <Command Name="RunPython" Arguments="SyncLiveConfigToSheets.py --spreadsheet-id YOUR_SPREADSHEET_ID_HERE"/>
</Node>
```

**Implementation Details**:
Since BuildGraph doesn't handle environment variables as natively as GitHub/GitLab, you should ensure the environment variable `GOOGLE_APPLICATION_CREDENTIALS_JSON` is set on the machine running the BuildGraph agent, or pass the credentials file path:

```xml
<Node Name="Sync Config to Sheets">
    <!-- Assuming the creds file is placed in a known location on the build machine -->
    <Command Name="RunPython" Arguments="SyncLiveConfigToSheets.py --spreadsheet-id YOUR_SPREADSHEET_ID_HERE --creds-file C:/BuildData/gcp-key.json"/>
</Node>
```

---

## Troubleshooting

*   **Authentication Error**: Ensure the Service Account email has been shared into the Google Sheet.
*   **ModuleNotFound**: Ensure `gspread` and `google-auth` are installed in the Python environment being used by the CI.
*   **File Path Warnings**: If the script warns about name mismatches, ensure the `propertyName` in your JSON files matches the folder structure.
