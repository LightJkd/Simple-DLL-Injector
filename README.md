# Simple DLL Injector

Simple DLL Injector is a versatile C++ application designed to inject DLL files into the `javaw.exe` process. It supports both local DLL files and downloading DLL files from a URL. The program also manages temporary files and directories created during the injection process.

## Features

- Inject DLL files into `javaw.exe`.
- Download DLL files from a specified URL.
- Manage temporary files and directories.

## Requirements

- Windows OS
- Administrator privileges

## Usage

### 1. Local DLL File Injection

1. Run the program.
2. Choose option `1` to input the path to a local DLL file.
3. Enter the full path to the DLL file when prompted.

### 2. Download and Inject DLL File from URL

1. Run the program.
2. Choose option `2` to download a DLL file from a URL.
3. Enter the URL of the DLL file when prompted.

### 3. Injection Process

- The program will find the `javaw.exe` process.
- It will inject the specified DLL file into the process.
- If a DLL file was downloaded, the temporary files and directories will be deleted after the injection process.

## Code Overview

### Main Functions

- `DownloadFile`: Downloads a file from a specified URL.
- `GetProcessIdByName`: Retrieves the process ID of a running process by its name.
- `InjectDLL`: Injects a DLL file into a specified process.
- `GetTempFolderPath`: Retrieves the path to the system's temporary folder.
- `CreateRandomFolder`: Creates a random folder in a specified directory.
- `DeleteFolder`: Deletes a specified folder and its contents.

## Notes

 - Ensure you have the necessary permissions to run the program.
 - The program targets the javaw.exe process specifically.
 - Temporary files and directories will be created and deleted during the injection process.
