[Setup]
AppName=NetShift
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\NetShift
DefaultGroupName=NetShift
OutputDir=..\artifacts
OutputBaseFilename=NetShiftInstaller
SetupIconFile=..\..\NetShiftIcon.ico
UninstallDisplayIcon={app}\NetShiftIcon.ico
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
AppPublisher=ZentrixLabs
AppPublisherURL=https://zentrixlabs.com
AppSupportURL=https://zentrixlabs.com/support
AppUpdatesURL=https://zentrixlabs.com/updates
SetupLogging=yes

[Files]
Source: "..\NetShiftMain\bin\x64\Release\net8.0-windows\net8.0-windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; Excludes: "*.resources.dll"
Source: "..\NetShiftMain\bin\x64\Release\net8.0-windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs onlyifdoesntexist; Excludes: "*.resources.dll"
Source: "..\NetShiftService\bin\x64\Release\NetShiftService.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\NetShiftIcon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\NetShift"; Filename: "{app}\NetShiftMain.exe"
Name: "{autodesktop}\NetShift"; Filename: "{app}\NetShiftMain.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{dotnet40}\InstallUtil.exe"; Parameters: """{app}\NetShiftService.exe"""; Flags: runhidden; Description: "Install NetShift Service"

[UninstallRun]
Filename: "{dotnet40}\InstallUtil.exe"; Parameters: "/u ""{app}\NetShiftService.exe"""; Flags: runhidden

[Code]
function IsDotNet48Installed: Boolean;
var
  regValue: Cardinal;
begin
  Result := RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full', 'Release', regValue) and (regValue >= 528040);
end;

function IsDotNet8Installed: Boolean;
var
  SubKeyNames: TArrayOfString;
  SubKey: String;
  DisplayName: String;
  I: Integer;
begin
  Result := False;
  // Check 64-bit uninstall hive
  if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall', SubKeyNames) then
  begin
    for I := 0 to GetArrayLength(SubKeyNames) - 1 do
    begin
      SubKey := 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\' + SubKeyNames[I];
      if RegQueryStringValue(HKEY_LOCAL_MACHINE, SubKey, 'DisplayName', DisplayName) then
      begin
        if (Pos('Microsoft Windows Desktop Runtime', DisplayName) > 0) and (Pos('8.0', DisplayName) > 0) then
        begin
          Result := True;
          Exit;
        end;
      end;
    end;
  end;
  // Check 32-bit uninstall hive (WOW6432Node) on 64-bit systems
  if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall', SubKeyNames) then
  begin
    for I := 0 to GetArrayLength(SubKeyNames) - 1 do
    begin
      SubKey := 'SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\' + SubKeyNames[I];
      if RegQueryStringValue(HKEY_LOCAL_MACHINE, SubKey, 'DisplayName', DisplayName) then
      begin
        if (Pos('Microsoft Windows Desktop Runtime', DisplayName) > 0) and (Pos('8.0', DisplayName) > 0) then
        begin
          Result := True;
          Exit;
        end;
      end;
    end;
  end;
end;

procedure InitializeWizard;
var
  ErrorCode: Integer;
begin
  if not IsDotNet48Installed then
  begin
    if not WizardSilent then
    begin
      MsgBox('.NET Framework 4.8 is required. Please download and install it.', mbInformation, MB_OK);
      ShellExec('open', 'https://dotnet.microsoft.com/download/dotnet-framework/net48', '', '', SW_SHOW, ewNoWait, ErrorCode);
      WizardForm.Close;
    end else
    begin
      // In silent mode, fail the installation
      WizardForm.Close;
    end;
  end;
  if not IsDotNet8Installed then
  begin
    if not WizardSilent then
    begin
      MsgBox('.NET 8 Desktop Runtime is required. Please download and install it.', mbInformation, MB_OK);
      ShellExec('open', 'https://dotnet.microsoft.com/en-us/download/dotnet/8.0', '', '', SW_SHOW, ewNoWait, ErrorCode);
      WizardForm.Close;
    end else
    begin
      // In silent mode, fail the installation
      WizardForm.Close;
    end;
  end;
end;