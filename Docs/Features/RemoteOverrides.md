# Remote Overrides

Live Config supports a customizable method of providing remote overrides. By default, the HTTP CSV provider fetches a CSV file.

Remote overrides are loaded on startup, on world load, and on a polling timer. An internal rate limit prevents requests from happening more than once per 5 seconds.

## Default setup with Google Sheets

Google Sheets is a convenient and accessible method of getting remote overrides going that is well suited to development environments / smaller playtests.

To get your Google Sheets CSV URL:
	 
Use **File -> Share -> Publish to web**. Pick the tab with your data and the CSV type
- Ensure "Automatically republish when changes are made" is enabled.
- Type is set to Comma Separated Values
- Only the tab with your overrides is selected

To make selecting properties in Sheets easier, see [Sync Google Sheets](/Features/SyncGoogleSheets)

## Scaling Beyond Sheets

To move beyond the scaling limitations of Sheets you can consider a few options:
- Sync your Sheets data to somewhere like S3. This can be done in AppScript
- Provide a CSV over HTTP via any other means. This only needs to have your property names in the first column, and values in the second.
- Implement a custom remote override provider to integrate with a service you are already using

## Changing the remote override provider

If your project already has a backend, you can consider implemeting your own remote override provider.

This provider implements a `FetchOverrides` method which

See `ULiveConfigHttpCsvProvider` for reference. 

