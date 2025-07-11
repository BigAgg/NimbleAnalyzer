#define Name "NimbleAnalyzer"
#define Version "0.2.0"
#define InstallDir "{commonpf}\NimbleAnalyzer"


[Setup]
; Setup options
AppName={#Name}
AppVersion={#Version}
DefaultDirName={#InstallDir}
DisableDirPage=no
DefaultGroupName={#Name}
OutputDir=.\output
OutputBaseFilename=setup_{#Name}
Compression=lzma
SolidCompression=yes
LicenseFile=LICENSE
SetupIconFile=.\res\{#Name}.ico

[FILES]
; Exectuable and needed files (dlls etc.)
Source: ".\build\Release\{#Name}.exe"; DestDir: "{app}"; Flags: ignoreversion
; Adding resource folders
Source: ".\res\*"; DestDir: "{app}\"; Flags: ignoreversion recursesubdirs createallsubdirs
; Adding dlls here same like above
Source: ".\build\Release\xlnt.dll"; DestDir: "{app}"; Flags: ignoreversion

[Dirs]
; Setting folder permissions to be editable
Name: "{app}"; Permissions: users-modify
Name: "{app}"; Flags: uninsalwaysuninstall

[Icons]
Name: "{group}\{#Name}"; Filename: "{app}\{#Name}.exe"; IconFilename: "{app}\{#Name}.ico"

[Registry]
; Optional registry entries (e.g., add app to start Menu or uninstall registry)
; Root: HKCU; Subkey: "Software\{#Name}"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"

[Run]
; Optional: Auto-run the app after installation
Filename: "{app}\{#Name}.exe"; Description: "{cm:LaunchProgram,{#Name}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Uninstall all files and dirs
Type: filesandordirs; Name: "{app}"
; Remove registry keys