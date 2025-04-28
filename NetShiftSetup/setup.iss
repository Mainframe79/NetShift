[Setup]
AppName=NetShift
AppVersion=1.0.0
DefaultDirName={autopf}\NetShift
OutputDir=artifacts
OutputBaseFilename=NetShiftInstaller
SetupIconFile=..\..\NetShiftIcon.ico
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
AppPublisher=ZentrixLabs
AppPublisherURL=https://zentrixlabs.com
AppSupportURL=https://zentrixlabs.com/support
AppUpdatesURL=https://zentrixlabs.com/updates
SetupLogging=yes

[Files]
Source: "NetShift\NetShiftMain\bin\x64\Release\net8.0-windows\net8.0-windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; Excludes: "*.resources.dll"
Source: "NetShift\NetShiftMain\bin\x64\Release\net8.0-windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs onlyifdoesntexist; Excludes: "*.resources.dll"
Source: "NetShift\NetShiftService\bin\x64\Release\NetShiftService.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\NetShift"; Filename: "{app}\NetShiftMain.exe"
Name: "{autodesktop}\NetShift"; Filename: "{app}\NetShiftMain.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{dotnet40}\InstallUtil.exe"; Parameters: """{app}\NetShiftService.exe"""; Flags: runhidden; Description: "Install NetShift Service"

[UninstallRun]
Filename: "{dotnet40}\InstallUtil.exe"; Parameters: "/u ""{app}\NetShiftService.exe"""; Flags: runhidden; Description: "Uninstall NetShift Service"

[Code]
function IsDotNet48Installed: Boolean;
var
  regValue: Cardinal;
begin
  Result := RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full', 'Release', regValue) and (regValue >= 528040);
end;

function IsDotNet8Installed: Boolean;
var
  regValue: Cardinal;
begin
  Result := RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\dotnet\Setup\InstalledVersions\x64\desktop', '8.0', regValue) and (regValue = 1);
end;

procedure InitializeWizard;
begin
  if not IsDotNet48Installed then
  begin
    MsgBox('.NET Framework 4.8 is required. Please download and install it.', mbInformation, MB_OK);
    ShellExec('open', 'https://dotnet.microsoft.com/download/dotnet-framework/net48', '', '', SW_SHOW, ewNoWait, nil);
    WizardForm.Close;
  end;
  if not IsDotNet8Installed then
  begin
    MsgBox('.NET 8 Desktop Runtime is required. Please download and install it.', mbInformation, MB_OK);
    ShellExec('open', 'https://dotnet.microsoft.com/en-us/download/dotnet/8.0', '', '', SW_SHOW, ewNoWait, nil);
    WizardForm.Close;
  end;
end;
