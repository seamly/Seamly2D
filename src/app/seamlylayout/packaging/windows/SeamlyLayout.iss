; project: SeamlyLayout
; author: slspencer, copyright 2026
; MIT License: https://opensource.org/licenses/MIT
;
; SeamlyLayout.iss — Inno Setup 6 installer script for Windows
;
; Prerequisites before running this script:
;   1. Build a Release executable:
;        cd <repo-root>  &&  .\qr.ps1
;      Output: qt_frontend\build\Release\SeamlyLayout.exe
;   2. Run windeployqt6 to gather Qt runtime DLLs:
;        cd qt_frontend\build\Release
;        C:\Qt\6.11.1\msvc2022_64\bin\windeployqt6.exe ^
;            --qmldir ..\..\qml --release SeamlyLayout.exe
;      This deposits all required DLLs and QML plugins into
;        qt_frontend\build\Release\
;   3. Run Inno Setup 6 (https://jrsoftware.org/isinfo.php):
;        iscc.exe packaging\windows\SeamlyLayout.iss
;   The installer is written to packaging\windows\Output\SeamlyLayout-0.1.0-win64.exe
;
; Runtime folders (written by the app on first run, not by the installer):
;   %LOCALAPPDATA%\Seamly\SeamlyLayout\settings\      — layout settings JSON files
;   %LOCALAPPDATA%\Seamly\SeamlyLayout\preferences\   — user preferences JSON file
;
; Legacy migration (performed automatically at first launch):
;   1. Task 15 (2026-07): the organization name changed from "Seamly Systems" to the
;      shared "Seamly" (matching seamly2d/seamlyme), so the AppConfigLocation folder
;      moved from %LOCALAPPDATA%\Seamly Systems\SeamlyLayout\ to
;      %LOCALAPPDATA%\Seamly\SeamlyLayout\. PreferencesModel::appConfigRootPath()
;      copies every file across from the old organization folder the first time the
;      new one is resolved, leaving the old folder in place (non-destructive).
;   2. If layout-settings\ or layout-preferences\ files exist under AppConfigLocation
;      from a pre-0.1.0 install, they are copied to the new canonical folder names and
;      the old folders are left in place so the upgrade is non-destructive.
;
; LGPL-3.0 compliance:
;   Qt 6.11 is dynamically linked under LGPL-3.0.  The installer creates a
;   "Licenses" sub-folder with the LGPL-3.0 text and a notice directing users
;   to https://download.qt.io for Qt source code.

#define AppName      "SeamlyLayout"
#define AppVersion   "0.1.0"
#define AppPublisher "Seamly Technologies Inc."
#define AppURL       "https://seamly.io"
#define AppExeName   "SeamlyLayout.exe"
#define SourceDir    "..\..\qt_frontend\build\Release"
#define SettingsDir  "..\..\qt_frontend\settings"
#define LicensesDir  "..\..\packaging\licenses"

[Setup]
AppId={{D4E2F1A0-8C3B-4D7E-9F6A-2B5C8E1D3F0A}}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
AllowNoIcons=yes
; LGPL compliance: allow user to change install dir so Qt DLLs can be extracted
DisableDirPage=no
LicenseFile={#LicensesDir}\LGPL-3.0.txt
OutputDir=Output
OutputBaseFilename={#AppName}-{#AppVersion}-win64
SetupIconFile=..\..\qt_frontend\assets\images\seamly-layout.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main executable
Source: "{#SourceDir}\{#AppExeName}";  DestDir: "{app}"; Flags: ignoreversion

; Qt runtime DLLs and plugins (gathered by windeployqt6)
Source: "{#SourceDir}\*.dll";          DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\*.conf";         DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\*.qml";          DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; Packaged default settings files — used by legacy migration and first-run seed.
; preferences.json is intentionally excluded: it contains per-user paths.
Source: "{#SettingsDir}\default_settings.json"; DestDir: "{app}\settings"; Flags: ignoreversion
Source: "{#SettingsDir}\B0.json";               DestDir: "{app}\settings"; Flags: ignoreversion
Source: "{#SettingsDir}\roll_36in.json";         DestDir: "{app}\settings"; Flags: ignoreversion
Source: "{#SettingsDir}\roll_48in.json";         DestDir: "{app}\settings"; Flags: ignoreversion

; LGPL-3.0 compliance — Qt license and source notice
Source: "{#LicensesDir}\LGPL-3.0.txt";           DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#LicensesDir}\qt-source-notice.txt";    DestDir: "{app}\licenses"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}";                 Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";           Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Registry]
; Register in "Apps & Features" / Programs and Features
Root: HKCU; Subkey: "Software\{#AppPublisher}\{#AppName}"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\{#AppPublisher}\{#AppName}"; ValueType: string; ValueName: "Version";     ValueData: "{#AppVersion}"

; Associate .settings file extension with SeamlyLayout
Root: HKCU; Subkey: "Software\Classes\.settings";                         ValueType: string; ValueName: ""; ValueData: "SeamlyLayout.Settings"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\SeamlyLayout.Settings";             ValueType: string; ValueName: ""; ValueData: "SeamlyLayout Settings File"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\SeamlyLayout.Settings\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"
Root: HKCU; Subkey: "Software\Classes\SeamlyLayout.Settings\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[Code]
// ---------------------------------------------------------------------------
// Upgrade guard: warn if a pre-0.1.0 install is detected and the user chose
// to install to the same folder.  Pre-0.1.0 builds wrote settings under
// <exeDir>\settings and preferences under <exeDir>\layout-preferences.
// The application migrates these automatically at first run; we just surface
// a friendly note here.
//
// Task 15 (2026-07): also detect the pre-org-rename "Seamly Systems" AppConfigLocation
// folder so upgraders are told their settings are moving to the new shared "Seamly"
// organization folder alongside seamly2d/seamlyme, not just to a new folder name.
// ---------------------------------------------------------------------------
procedure CurPageChanged(CurPageID: Integer);
var
  LegacyPrefsPath: String;
  LegacyOrganizationPath: String;
begin
  if CurPageID = wpSelectDir then begin
    LegacyPrefsPath := ExpandConstant('{app}') + '\layout-preferences';
    LegacyOrganizationPath := ExpandConstant('{localappdata}') + '\Seamly Systems\SeamlyLayout';
    if DirExists(LegacyPrefsPath) then begin
      MsgBox(
        'A previous SeamlyLayout installation was found.' + #13#10 +
        'Your settings and preferences will be migrated automatically ' +
        'to the new "settings" and "preferences" folders when you first ' +
        'launch the updated application.' + #13#10#13#10 +
        'No data will be lost.',
        mbInformation, MB_OK);
    end // if LegacyPrefsPath exists
    else if DirExists(LegacyOrganizationPath) then begin
      MsgBox(
        'A previous SeamlyLayout installation was found.' + #13#10 +
        'Your settings and preferences have moved to a new shared location ' +
        '(the "Seamly" folder, alongside Seamly2D and SeamlyMe) and will be ' +
        'migrated automatically when you first launch the updated application.' +
        #13#10#13#10 +
        'No data will be lost.',
        mbInformation, MB_OK);
    end; // if LegacyPrefsPath / LegacyOrganizationPath exists
  end; // if wpSelectDir
end; // CurPageChanged
