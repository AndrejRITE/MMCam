[Setup]
AppId=Rigaku.MMCam
AppName={#RepoName}
AppVersion={#Major}.{#Minor}.{#Build}
AppVerName={#RepoName} - {#Major}.{#Minor}.{#Build}
AppPublisher=Rigaku Innovative Technologies Europe
VersionInfoCompany=Rigaku Innovative Technologies Europe
DefaultDirName={localappdata}\Programs\{#RepoName}
DefaultGroupName={#RepoName}
OutputBaseFilename={#OutputBaseFilename}
OutputDir={#OutputDir}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
DisableWelcomePage=no
SetupIconFile={#IconFullPath}
DisableDirPage=no
UninstallDisplayIcon={app}\{#RepoName}.exe
PrivilegesRequired=admin
; x64-only build
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Dirs]
Name: "{localappdata}\Programs"; Permissions: users-full

[Files]
Source: "{#OutputDir}\src\*"; DestDir: "{app}\src"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#OutputDir}\About.zip"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\keyfile.sqlite"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\{#RepoName}.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\{#RepoName}.ini"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\table.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\requirements.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\Xeryon.py"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\xeryon_goCenter.py"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#OutputDir}\xeryon_setAbsolutePosition.py"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#IconFullPath}"; DestDir: "{app}"; Flags: ignoreversion

; --- Bundle VC++ 2015–2022 (x64) redist ---
; Place the official Microsoft installer at: .\redist\VC_redist.x64.exe
Source: "redist\VC_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Code]
procedure DeleteOptionalDirs;
var
  VenvPath: string;
begin
  VenvPath := ExpandConstant('{app}\src\ReportGenerator\.venv');
  if DirExists(VenvPath) then
  begin
    DelTree(VenvPath, True, True, True);
  end;
end;

procedure DeinitializeUninstall();
begin
  DeleteOptionalDirs;
end;

[Icons]
Name: "{group}\{#RepoName}"; Filename: "{app}\{#RepoName}.exe"; IconFilename: "{app}\logo.ico"
Name: "{commondesktop}\{#RepoName}"; Filename: "{app}\{#RepoName}.exe"; IconFilename: "{app}\logo.ico"
Name: "{commonprograms}\{#RepoName}"; Filename: "{app}\{#RepoName}.exe"; IconFilename: "{app}\logo.ico"

[Registry]
Root: HKCU; Subkey: "SOFTWARE\RITE\{#RepoName}"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: createvalueifdoesntexist uninsdeletekey

[Run]
; Install VC++ runtime only if missing
Filename: "{tmp}\VC_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ 2015–2022 Redistributable (x64)…"; Check: NeedsVC2015To2022x64;

Filename: "{app}\{#RepoName}.exe"; Description: "{cm:LaunchProgram,{#RepoName}}"; Flags: nowait postinstall skipifsilent

[Code]
function GetInstallPath: string;
var
  InstallPath: string;
begin
  // Initialize result to empty string
  Result := '';

  // Read the installation path from the registry
  if RegQueryStringValue(HKCU, 'SOFTWARE\RITE\{#RepoName}', 'InstallPath', InstallPath) then
  begin
    Result := InstallPath;
  end;
end;

// ---------- VC++ 2015–2022 presence check (x64) ----------
function IsVC2015To2022x64Installed: Boolean;
var
  Installed: Cardinal;
begin
  { Official unified key for VS 2015–2022 runtimes }
  Result :=
    RegQueryDWordValue(
      HKLM,
      'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64',
      'Installed',
      Installed) and (Installed = 1);
end;

function NeedsVC2015To2022x64: Boolean;
begin
  Result := not IsVC2015To2022x64Installed;
end;