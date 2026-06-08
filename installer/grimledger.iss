; GrimLedger Windows installer (Inno Setup 6)
; Build: .\installer\build-installer.ps1  →  dist\GrimLedger-Setup.exe

#define MyAppName "GrimLedger"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "grimsec-labs"
#define MyAppURL "https://github.com/grimsec-labs/GrimLedger"
#define MyAppExeName "GrimLedger.exe"
#define StageDir "..\dist\GrimLedger"

[Setup]
AppId={{A7B3C9D1-4E2F-4A8B-9C1D-2F3E4A5B6C7D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\GrimLedger\app
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=GrimLedger-Setup
#ifexist "..\resources\icon.ico"
SetupIconFile=..\resources\icon.ico
#endif
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
InfoAfterFile=..\installer\post-install.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked
Name: "openextfolder"; Description: "Open the browser extension folder when finished"; GroupDescription: "Browser bridge:"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "install.ps1,README.md,browser-extension\*"
Source: "{#StageDir}\browser-extension\*"; DestDir: "{localappdata}\GrimLedger\browser-extension"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion; DestName: "INSTALL.txt"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{group}\Browser extension folder"; Filename: "{localappdata}\GrimLedger\browser-extension"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "explorer.exe"; Parameters: "{localappdata}\GrimLedger\browser-extension"; Description: "Open extension folder for ""Load unpacked"""; Flags: postinstall nowait skipifsilent; Tasks: openextfolder
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent unchecked

[UninstallRun]
Filename: "powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\native-host\uninstall-windows.ps1"" -Browser Both"; Flags: runhidden waituntilterminated

[Code]
var
  ExtensionIdPage: TInputQueryWizardPage;

procedure InitializeWizard;
begin
  ExtensionIdPage := CreateInputQueryPage(wpSelectTasks,
    'Browser extension', 'Native messaging host (optional)',
    'After setup:' + #13#10 +
    '1. Chrome or Edge → chrome://extensions' + #13#10 +
    '2. Enable Developer mode → Load unpacked' + #13#10 +
    '3. Select the extension folder (opened for you if checked below)' + #13#10 +
    '4. Paste the 32-character extension ID here to register the native host' + #13#10 +
    '   (or leave blank and run native-host\install-windows.ps1 later)');
  ExtensionIdPage.Add('Extension ID (optional):', False);
end;

function IsValidExtensionId(const Id: string): Boolean;
var
  I: Integer;
  C: Char;
begin
  Result := Length(Id) = 32;
  if not Result then Exit;
  for I := 1 to Length(Id) do
  begin
    C := Id[I];
    if not ((C >= 'a') and (C <= 'p')) then
    begin
      Result := False;
      Exit;
    end;
  end;
end;

procedure RegisterNativeHost(const ExtId: string);
var
  HostExe, ScriptPath, Params: string;
  ResultCode: Integer;
begin
  HostExe := ExpandConstant('{app}\grimledger_host.exe');
  ScriptPath := ExpandConstant('{app}\native-host\install-windows.ps1');
  Params := '-ExecutionPolicy Bypass -File "' + ScriptPath + '" -HostExe "' + HostExe + '" -ExtensionId ' + ExtId + ' -Browser Both';
  if not Exec('powershell.exe', Params, '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
    MsgBox('Native host registration failed. Run manually:' + #13#10 + ScriptPath, mbError, MB_OK);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ExtId: string;
begin
  if CurStep <> ssPostInstall then Exit;
  ExtId := Trim(ExtensionIdPage.Values[0]);
  if ExtId = '' then Exit;
  if not IsValidExtensionId(ExtId) then
  begin
    MsgBox('Extension ID must be exactly 32 characters (a-p). Skipping native-host registration.', mbInformation, MB_OK);
    Exit;
  end;
  RegisterNativeHost(ExtId);
end;
