
# NetShift

![Build](https://img.shields.io/github/actions/workflow/status/Mainframe79/NetShift/deploy.yml?branch=main)
![Release](https://img.shields.io/github/v/release/Mainframe79/NetShift)
![License](https://img.shields.io/github/license/Mainframe79/NetShift)
![Last Commit](https://img.shields.io/github/last-commit/Mainframe79/NetShift)

## Download
The latest release of NetShift is available on the [Releases page](https://github.com/LazerPanth3r/NetShift-Public/releases).  
Download the `NetShiftInstaller.exe` file directly from GitHub Releases and run it as Administrator to install.

## Overview

NetShift is a Windows utility that simplifies switching IP settings on network adapters. It allows users to quickly toggle between static IP configurations and DHCP using predefined presets — ideal for environments where network settings frequently change.  
The app is designed to be **user-friendly**, **secure**, and **efficient**, minimizing user privilege requirements while maintaining robust functionality.

### Key Features
- **Preset Management**: Save, apply, and delete IP configuration presets.
- **DHCP Support**: Reset adapters to DHCP easily.
- **Network Adapter Detection**: Lists physical adapters (excludes Bluetooth adapters).
- **Registry Fallback**: Preconfigure IP settings when adapters are disconnected.
- **Service-Based Architecture**: A LocalSystem Windows service (NetShiftService) handles privileged operations.
- **Installer**: Self-contained installer with .NET Framework 4.8 and .NET 8 Desktop Runtime handling.

---

## Security Considerations

| Risk | Mitigation |
|:-----|:-----------|
| **Service runs as LocalSystem** | Local-only named pipe, input validation, and code signing. |
| **Code signing** | All binaries are signed via SSL.com cloud signing to preserve trust. |
| **User privilege model** | Main app runs standard user level (`asInvoker`), no UAC prompt. |
| **Log files** | Logs are stored under user-local AppData, access-controlled. |
| **Local network communication** | WCF NetNamedPipeBinding used, locked to localhost only. |

---

## Installation

### Prerequisites
- Windows 10 (x64) or later.
- .NET Framework 4.8 (auto-installed if missing).
- .NET 8 Desktop Runtime (auto-installed if missing).

### Steps to Install
1. Download the latest `NetShiftInstaller.exe` from the [Releases](https://github.com/LazerPanth3r/NetShift-Public/releases) page.
2. Run `NetShiftInstaller.exe` as Administrator.
3. Follow the installer prompts.

### Silent Install
For automated deployments:

```bash
NetShiftInstaller.exe /silent
```

---

## Usage
1. Launch NetShift from Start Menu or Desktop.
2. Select a network adapter.
3. Apply a preset or manually configure IP settings.
4. Click "Apply" or "Reset to DHCP."
5. Save new presets for quick switching.

---

## Building and Contributing

### Automated Build and Release

This project uses **GitHub Actions** to automatically:
- Build the NetShift application and service.
- Package the signed MSI and installer EXE.
- Cloud sign all binaries using SSL.com eSigner.
- Publish new signed releases on GitHub.

The pipeline **only triggers** when:
- Code is pushed to `main` (excluding README and workflow files).
- A version tag (e.g., `v1.0.1`) is pushed.

---

### Manual Local Build

If you want to build manually:

1. **Clone the Repository**:

```bash
git clone https://github.com/LazerPanth3r/NetShift-Public.git
cd NetShift-Public
```

2. **Install Prerequisites**:
- Visual Studio with .NET desktop workload
- [WiX Toolset v5.0](https://wixtoolset.org/releases/)

3. **Build and Sign Projects**:
- Build `NetShift` project
- `SignAppPersonal -Path "D:\Github\NetShift-Public\NetShift\bin\Release\net8.0-windows\NetShift.exe"`
- Build `NetShiftService` project
- `SignAppPersonal -Path "D:\Github\NetShift-Public\NetShiftService\bin\Release\NetShiftService.exe"`
- Copy signed `NetShiftService.exe` into `NetShiftSetup`.
- Run `heat.exe`:

```bash
"C:\Program Files\WiX Toolset v5.0\bin\x64\heat.exe" dir "D:\Github\NetShift-Public\NetShift\bin\Release\net8.0-windows" -dr APPFOLDER -cg NetShiftFiles -gg -scom -sreg -srd -template:fragment -out "D:\Github\NetShift-Public\NetShiftSetup\NetShiftFiles.wxs"
```

- Build `NetShiftSetup`
- `SignAppPersonal -Path "D:\Github\NetShift-Public\NetShiftSetup\bin\Release\NetShiftSetup.msi"`
- Embed new MSI in `NetShiftInstaller` project.
- Build `NetShiftInstaller`
- `SignAppPersonal -Path "D:\Github\NetShift-Public\NetShiftInstaller\bin\Release\NetShiftInstaller.exe"`

4. **Test Installer**:

```bash
copy D:\Github\NetShift-Public\NetShiftInstaller\bin\Release\NetShiftInstaller.exe C:\Temp\
cd C:\Temp
NetShiftInstaller.exe
```

---

## Contributing
Pull requests are welcome!  
Please fork the repository, make your changes, and submit a pull request.  
Ensure changes are well-tested and match the project's security-first standards.

If you'd like to support this project:

[![Buy Me A Coffee](https://cdn.buymeacoffee.com/buttons/default-orange.png)](https://www.buymeacoffee.com/Mainframe79)

---

## How It Works

```text
User --> NetShift.exe (runs as Standard User)
            |
            --> (Named Pipe Communication)
                    --> NetShiftService (runs as LocalSystem)
                               |
                               --> Windows Networking Stack (Change IP / DHCP Settings)
```

- The user interacts with a normal UI without needing elevation.
- All privileged operations (IP reconfiguration, registry edits) happen securely through the service.
- The service is isolated and only accessible locally.

---

## License
NetShift is licensed under the [MIT License](LICENSE).
