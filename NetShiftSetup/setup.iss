[Setup]
AppName=NetShift
AppVersion={#MyAppVersion}
DefaultDirName={pf64}\ZentrixLabs\NetShift
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
ArchitecturesInstallIn64BitMode=x64
UninstallSilent=yes
UninstallLogMode=append

[Files]
Source: "..\NetShiftMain\bin\x64\Release\net8.0-windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; Excludes: "*.resources.dll"
Source: "..\NetShiftServiceCpp\x64\Release\NetShiftServiceCpp.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\NetShiftServiceInstaller\x64\Release\NetShiftServiceInstaller.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\NetShiftIcon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\NetShift"; Filename: "{app}\net8.0-windows\NetShiftMain.exe"; IconFilename: "{app}\NetShiftIcon.ico"; WorkingDir: "{app}"
Name: "{autodesktop}\NetShift"; Filename: "{app}\net8.0-windows\NetShiftMain.exe"; IconFilename: "{app}\NetShiftIcon.ico"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\NetShiftServiceInstaller.exe"; Parameters: "--install \"{app}\NetShiftServiceCpp.exe\""; Flags: runhidden waituntilterminated; Description: "Install NetShift Service"

[UninstallRun]
Filename: "{app}\NetShiftServiceInstaller.exe"; Parameters: "--uninstall"; Flags: runhidden waituntilterminated

[Code]
function IsDotNet8Installed: Boolean;
var
  SubKeyNames: TArrayOfString;
  SubKey: String;
  DisplayName: String;
  I: Integer;
begin
  Result := False;
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
  if not IsDotNet8Installed then
  begin
    if not WizardSilent then
    begin
      MsgBox('.NET 8 Desktop Runtime is required. Please download and install it.', mbInformation, MB_OK);
      ShellExec('open', 'https://dotnet.microsoft.com/en-us/download/dotnet/8.0', '', '', SW_SHOW, ewNoWait, ErrorCode);
      WizardForm.Close;
    end else
    begin
      WizardForm.Close;
    end;
  end;
end;

procedure DeinitializeUninstall();
var
  ProgramDataPath: string;
begin
  Log('Performing final cleanup...');

  if GetEnv('ProgramData') <> '' then
  begin
    ProgramDataPath := ExpandConstant('{pf64}') + '\ZentrixLabs\NetShift';
    DelTree(ProgramDataPath, True, True, True);
    Log('Deleted: ' + ProgramDataPath);
  end;

  if not WizardSilent then
  begin
    MsgBox('NetShift has been completely uninstalled.', mbInformation, MB_OK);
  end;
end;
