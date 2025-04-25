# NetShift

## Download
The latest release of NetShift is available on the [Releases page](https://github.com/LazerPanth3r/NetShift-Public/releases). Download the `NetShiftInstaller.exe` file directly from GitHub Releases and run it as Administrator to install.

## Overview

NetShift is a Windows utility that simplifies the process of changing IP settings on network adapters. It allows users to quickly switch between static IP configurations and DHCP using predefined presets, making it ideal for environments where network settings need to be adjusted frequently (e.g., switching between different network locations). The app is designed to be user-friendly, secure, and efficient, with a focus on minimizing user privilege requirements while ensuring robust functionality.

### Key Features
- **Preset Management**: Save, apply, and delete IP configuration presets (IP address, subnet mask, gateway, DNS).
- **DHCP Support**: Easily reset network adapters to DHCP.
- **Network Adapter Detection**: Automatically detects and lists physical network adapters (excluding Bluetooth adapters).
- **Registry Fallback**: Supports preconfiguring IP settings via the registry when the network adapter is disconnected.
- **Service-Based Architecture**: Uses a Windows service (NetShiftService) running as LocalSystem to handle privileged operations, ensuring the main app (NetShift.exe) can run without elevation.
- **Installer**: Includes a self-contained installer (NetShiftInstaller.exe) that handles prerequisites (.NET Framework 4.8, .NET 8 Desktop Runtime) and installs the app and service.

## Security Concerns

NetShift is designed with security in mind, but there are a few considerations to be aware of:

### 1. Service Running as LocalSystem
- The NetShiftService runs as LocalSystem, which grants it full administrative privileges on the machine. This is necessary to perform operations like changing IP settings and modifying the registry.
- **Risk**: If the service is compromised, an attacker could potentially execute arbitrary code with elevated privileges.
- **Mitigation**:
  - The service communicates with the app via a named pipe (net.pipe://localhost/NetShiftService), which is restricted to local communication.
  - The service validates all input (e.g., IP addresses, subnet masks) to prevent injection attacks.
  - Ensure the service binary (NetShiftService.exe) is signed to prevent tampering.

### 2. Code Signing
- All executables (NetShift.exe, NetShiftService.exe, NetShiftSetup.msi, NetShiftInstaller.exe) are signed using a code signing certificate to ensure authenticity and integrity.
- **Risk**: If the code signing certificate is compromised, an attacker could sign malicious versions of the app.
- **Mitigation**:
  - Store the code signing certificate securely (e.g., on a hardware security module or encrypted storage).
  - Use a timestamp server during signing to ensure the signature remains valid even if the certificate expires.

### 3. User Privileges
- The main app (NetShift.exe) runs as a standard user (with requestedExecutionLevel set to asInvoker), avoiding UAC prompts.
- **Risk**: If the app is compromised, it could potentially send malicious commands to the service, which runs as LocalSystem.
- **Mitigation**:
  - The app validates all user input before sending commands to the service.
  - The service performs additional validation on received commands to prevent unauthorized actions.

### 4. Log Files
- The app and service write logs to C:\Users\<username>\AppData\Local\NetShift\Logs (e.g., netsh_output_static.log, service_error.log).
- **Risk**: Log files could potentially contain sensitive information (e.g., IP addresses) that might be useful to an attacker.
- **Mitigation**:
  - Ensure log files are only accessible to the user (the default permissions for C:\Users\<username>\AppData\Local\ should enforce this).
  - Avoid logging sensitive information unless absolutely necessary.

### 5. Network Communication
- The app communicates with the service via a named pipe (net.pipe://localhost/NetShiftService).
- **Risk**: If the named pipe is not properly secured, an attacker could potentially impersonate the app or service.
- **Mitigation**:
  - The named pipe is restricted to local communication (localhost), reducing the attack surface.
  - The service uses WCF with NetNamedPipeBinding, which provides built-in security for local communication.

## Installation

### Prerequisites
- **Operating System**: Windows 10 or later (64-bit).
- **.NET Framework 4.8**: Automatically installed by the installer if not present.
- **.NET 8 Desktop Runtime**: Automatically installed by the installer if not present.

### Steps to Install
1. Download the latest `NetShiftInstaller.exe` from the [Releases](https://github.com/<your-username>/NetShift-Public/releases) page.
2. Run `NetShiftInstaller.exe` as Administrator:
   - Right-click the file and select "Run as administrator".
   - The installer will install the required runtimes (.NET Framework 4.8, .NET 8 Desktop Runtime) if they’re not already present.
3. Follow the installation prompts to complete the setup.
4. After installation, NetShift will be available via the Start Menu and Desktop shortcuts.

### Silent Installation
For automated or silent installation (e.g., for IT administrators):
- Run the installer with the `/silent` switch: `NetShiftInstaller.exe /silent`

## Usage
1. Launch NetShift from the Start Menu or Desktop shortcut.
2. Select a network adapter from the dropdown list.
3. Choose a preset from the preset dropdown or enter a new IP configuration (IP address, subnet mask, gateway, DNS).
4. Click "Apply" to set the static IP configuration, or "Reset to DHCP" to revert to DHCP.
5. Save new presets by clicking "Save Preset" and entering a name.

## Building and Contributing

### Prerequisites
- **WiX Toolset**: Ensure the WiX Toolset (v5.0 or later) is installed for building the MSI.
- **Visual Studio**: Use Visual Studio to build the projects.

### Steps to Build
1. **Clone the Repository**:
   - Clone the repository: `git clone https://github.com/<your-username>/NetShift-Public.git`
   - Navigate to the solution directory: `cd NetShift-Public`

2. **Build the Projects**:
   - Open the solution (NetShift-Public.sln) in Visual Studio.
   - Set the solution configuration to Release.
   - Build the `NetShift` project: Right-click the `NetShift` project > Build.
   - Build the `NetShiftService` project: Right-click the `NetShiftService` project > Build.
   - Copy the `NetShiftService.exe` to the `NetShiftSetup` directory: `copy D:\Github\NetShift-Public\IPChangerService\bin\Release\NetShiftService.exe D:\Github\NetShift-Public\NetShiftetup\`
   - Harvest files with `heat.exe`: `"C:\Program Files\WiX Toolset v5.0\bin\x64\heat.exe" dir "D:\Github\NetShift-Public\NetShift\bin\Release\net8.0-windows" -gg -sfrag -out "D:\Github\NetShift-Public\NetShiftSetup\NetShiftFiles.wxs"`
   - Build the `NetShiftSetup` project: Right-click the `NetShiftSetup` project > Build.
   - Update the embedded resource in the `NetShiftInstaller` project with the new `NetShiftSetup.msi`.
   - Build the `NetShiftInstaller` project: Right-click the `NetShiftInstaller` project > Build.

3. **Sign the Components** (if you have a code signing certificate):
   - Sign `NetShift.exe`: `signapp -path "D:\Github\NetShift-Public\NetShift\bin\Release\net8.0-windows\NetShift.exe"`
   - Sign `NetShiftService.exe`: `signapp -path "D:\Github\NetShift-Public\NetShiftService\bin\Release\NetShiftService.exe"`
   - Sign `NetShiftSetup.msi`: `signapp -path "D:\Github\NetShift-Public\NetShiftSetup\bin\Release\NetShiftSetup.msi"`
   - Sign `NetShiftInstaller.exe`: `signapp -path "D:\Github\NetShift-Public\NetShiftInstaller\bin\Release\NetShiftInstaller.exe"`

4. **Test the Installer**:
   - Copy `NetShiftInstaller.exe` to a test machine: `copy D:\Github\NetShift-Public\NetShiftInstaller\bin\Release\NetShiftInstaller.exe C:\Temp\`
   - Run the installer: `cd C:\Temp && NetShiftInstaller.exe`
   - Verify the installation and test the app functionality.

### Contributing
Contributions are welcome! Please fork the repository, make your changes, and submit a pull request. Ensure your changes are well-tested and follow the project’s coding standards.

<a href="https://www.buymeacoffee.com/Mainframe79" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>


## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
