; Copyright © 2015-2026 Pascal JEAN aka epsilonRT, All rights reserved.
;
; VERSION can be overridden from the command line:
;   iscc /DVERSION=1.5.3 mbpoll.iss
; BUILDDIR can be overridden to point to the CMake build output:
;   iscc /DBUILDDIR=..\..\build\bin\Release mbpoll.iss

#ifndef VERSION
  #define VERSION "1.5.3"
#endif
#ifndef BUILDDIR
  #define BUILDDIR "..\..\build\bin\Release"
#endif

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
AppId={{B380C12E-9574-4C33-B779-CEBBEEAE18C8}
AppName=MBPoll
AppVerName=MBPoll {#VERSION}
AppVersion={#VERSION}
AppPublisher=Pascal Jean aka epsilonRT
AppPublisherURL=https://github.com/epsilonrt/mbpoll
AppSupportURL=https://github.com/epsilonrt/mbpoll
AppUpdatesURL=https://github.com/epsilonrt/mbpoll
DefaultDirName={autopf}\MBPoll
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=mbpoll.ico
DisableDirPage=yes
AlwaysShowDirOnReadyPage=yes
OutputDir=installer
OutputBaseFilename=mbpoll-setup-{#VERSION}
Compression=lzma
SolidCompression=yes
DefaultGroupName=MBPoll
ChangesAssociations=yes
ChangesEnvironment=true

[Languages]
Name: en; MessagesFile: compiler:Default.isl; LicenseFile: license.txt

[CustomMessages]
en.MainFiles=MBPoll
en.MainDescription=A command line user interface allows easy communication with ModBus RTU and TCP slave
en.Sources=Source Files
en.Redist=Microsoft Visual C++ Redistributable (x64)

[Components]
Name: main; Description: {cm:MainFiles}; Types: full compact custom; Flags: fixed
Name: vc_redist; Description: {cm:Redist}; Types: custom
Name: source; Description: {cm:Sources}; Types: custom

[Tasks]
Name: modifypath; Description: Add application directory to your environmental path; Flags: checkedonce

[Files]
Source: {#BUILDDIR}\*; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: license*.txt; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: mbpoll.ico; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: ..\..\*; Excludes: \build,\cmake-build-*,\package,\libmodbus,\3rdparty,\.git,\.github,\tests,.*; DestDir: {app}\src; Flags: ignoreversion recursesubdirs createallsubdirs; Components: source
Source: tmp\vc_redist.x64.exe; DestDir: {tmp}; Flags: deleteafterinstall

[Run]
Filename: {tmp}\vc_redist.x64.exe; Parameters: "/install /quiet /norestart"; Components: vc_redist; StatusMsg: Installing Visual C++ Redistributable...

; ModPathName defines the name of the task defined above.
; ModPathType defines whether the user or system path will be modified;
;   this must be either system or user.
; setArrayLength must specify the total number of dirs to be added;
; Result[0] contains first directory, Result[1] contains second (optional), etc.
[Code]
const
    ModPathName = 'modifypath';
    ModPathType = 'system';
function ModPathDir(): TArrayOfString;
begin
    setArrayLength(Result, 1)
    Result[0] := ExpandConstant('{app}');
end;
#include "modpath.iss"
