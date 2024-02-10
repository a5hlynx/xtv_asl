# ASL Viewer X-Tension

A Viewer X-Tension for X-Ways Forensics, which parses and previews the selected Apple System Log (asl).

## Build

Open xtv_asl.sln and Build solution for the desired platform. xtv_asl.dll will be created at the path below. The solution is confirmed to be built by Visual Studio 2022.
* x86
  * xtv_asl\Release\xtv_asl.dll
* x64
  * xtv_asl\x64\Release\xtv_asl.dll

The dll for both x86 and x64 already build are also available at the [release](https://github.com/a5hlynx/xtv_asl/releases).

## Usage
* Load
  1. Click "File Viewing..." in "Options" from the Menu bar.
  2. Check "Load viewer X-Tensions" and click "..." in the "File Viewing" Window.
  3. Select xtv_asl.dll in the "Load Viewer X-Tensions" Window.
* Parse & Preview
  1. Select the Apple System Log in the Directory Browser.
  2. Click the Preview tab in the Data View to show the raw data.
  3. Click the XT tab to show the parsed data. XT tab switches raw and parsed.

## Requirements
xtv_asl.dll is confirmed to work with X-Ways Forensics 21.0 installed on x64 Windows 10.

## License
GNU Affero General Public License v3.0.

## Documentation
The usage Example is posted at https://9ood4nothin9.blogspot.com/2020/05/asl-viewer-x-tension.html
