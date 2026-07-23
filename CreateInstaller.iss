[Setup]
AppId=Rigaku.{#RepoName}
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
LicenseFile=License.txt

[Tasks]
Name: "desktopicon"; Description: "Create a desktop icon"; GroupDescription: "Additional options:"; Flags: checkedonce
Name: "install_vcredist_2013"; Description: "Install Microsoft Visual C++ 2013 Redistributable (x64)"; GroupDescription: "Additional options:"; Flags: checkedonce
Name: "install_vcredist"; Description: "Install Microsoft Visual C++ 2015-2022 Redistributable (x64)"; GroupDescription: "Additional options:"; Flags: checkedonce

[Dirs]
Name: "{localappdata}\Programs"; Permissions: users-full

[Files]
Source: "{#OutputDir}\src\*"; DestDir: "{app}\src"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#OutputDir}\hwlibs\*"; DestDir: "{app}\hwlibs"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#OutputDir}\pixet.ini"; DestDir: "{app}"; Flags: ignoreversion
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

Source: "redist\vcredist_2013_x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
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
; Install VC++ 2013 Redistributable only when:
; 1. the user selected the checkbox,
; 2. it is not already installed.
Filename: "{tmp}\vcredist_2013_x64.exe"; \
    Parameters: "/install /quiet /norestart"; \
    StatusMsg: "Installing Microsoft Visual C++ 2013 Redistributable..."; \
    Flags: waituntilterminated; \
    Tasks: install_vcredist_2013; \
    Check: ShouldInstallVC2013Redist
; Install VC++ Redistributable only when:
; 1. the user selected the checkbox,
; 2. it is not already installed.
Filename: "{tmp}\VC_redist.x64.exe"; \
    Parameters: "/install /quiet /norestart"; \
    StatusMsg: "Installing Microsoft Visual C++ Redistributable..."; \
    Flags: waituntilterminated; \
    Tasks: install_vcredist; \
    Check: ShouldInstallVCRedist

; Launch app after install
Filename: "{app}\{#RepoName}.exe"; Description: "{cm:LaunchProgram,{#RepoName}}"; Flags: nowait postinstall skipifsilent

[Code]
function GetInstallPath: string;
var
  InstallPath: string;
begin
  Result := '';

  if RegQueryStringValue(HKCU, 'SOFTWARE\RITE\{#RepoName}', 'InstallPath', InstallPath) then
  begin
    Result := InstallPath;
  end;
end;

// ---------- VC++ 2013 x64 Runtime Detection ----------

function IsVC2013RedistInstalled: Boolean;
var
  Installed: Cardinal;
  Version: string;
begin
  Result := False;

  if RegQueryDWordValue(
       HKLM64,
       'SOFTWARE\Microsoft\VisualStudio\12.0\VC\Runtimes\x64',
       'Installed',
       Installed) then
  begin
    if Installed = 1 then
    begin
      if RegQueryStringValue(
           HKLM64,
           'SOFTWARE\Microsoft\VisualStudio\12.0\VC\Runtimes\x64',
           'Version',
           Version) then
      begin
        Log(Format('VC++ 2013 Redistributable x64 detected. Version: %s', [Version]));
      end
      else
      begin
        Log('VC++ 2013 Redistributable x64 detected, but version value was not found.');
      end;

      Result := True;
      Exit;
    end;
  end;

  Log('VC++ 2013 Redistributable x64 was not detected.');
end;


function ShouldInstallVC2013Redist: Boolean;
begin
  Result := False;

  if not WizardIsTaskSelected('install_vcredist_2013') then
  begin
    Log('VC++ 2013 Redistributable installation skipped because the user did not select the task.');
    Exit;
  end;

  if IsVC2013RedistInstalled then
  begin
    Log('VC++ 2013 Redistributable installation skipped because it is already installed.');
    Exit;
  end;

  Log('VC++ 2013 Redistributable installation will run.');
  Result := True;
end;

// ---------- VC++ 2015-2022 x64 Runtime Detection ----------

function IsVCRedistInstalled: Boolean;
var
  Installed: Cardinal;
  Version: string;
begin
  Result := False;

  if RegQueryDWordValue(
       HKLM,
       'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64',
       'Installed',
       Installed) then
  begin
    if Installed = 1 then
    begin
      if RegQueryStringValue(
           HKLM,
           'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64',
           'Version',
           Version) then
      begin
        Log(Format('VC++ Redistributable x64 detected. Version: %s', [Version]));
      end
      else
      begin
        Log('VC++ Redistributable x64 detected, but version value was not found.');
      end;

      Result := True;
      Exit;
    end;
  end;

  Log('VC++ Redistributable x64 was not detected.');
end;


function ShouldInstallVCRedist: Boolean;
begin
  Result := False;

  if not WizardIsTaskSelected('install_vcredist') then
  begin
    Log('VC++ Redistributable installation skipped because the user did not select the task.');
    Exit;
  end;

  if IsVCRedistInstalled then
  begin
    Log('VC++ Redistributable installation skipped because it is already installed.');
    Exit;
  end;

  Log('VC++ Redistributable installation will run.');
  Result := True;
end;