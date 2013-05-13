;====================================================================
;
; Universal Proxy
; Inno Setup 5 Install Script
;
; Core application
;--------------------------------------------------------------------
;
; This version is released as part of the European Union sponsored
; project Mona Lisa work package 4 for the Universal Proxy Application
;
; This version is released under the GNU General Public License with restrictions.
; See the doc/license.txt file.
;
; Copyright (C) 2011-2013 by GateHouse A/S
; All Rights Reserved.
; http://www.gatehouse.dk
; mailto:gh@gatehouse.dk
;====================================================================

#include "include_versions.iss"
#include "include_code_w32_services.iss"

[Setup]
AppName=Uniproxy
AppVerName=Uniproxy {#VERSIONNUMBER} build {#BUILDNUMBER}
DefaultDirName={pf}\uniproxy
DefaultGroupName=Uniproxy
UninstallDisplayIcon={app}\uniproxy.exe
OutputBaseFilename=uniproxy_{#VERSIONNUMBER}

PrivilegesRequired=admin
Compression=lzma
SolidCompression=yes

[Files]
Source: "../build/release/uniproxy.exe"; DestDir: "{app}"

Source: "third/vcredist_x86.exe"; Flags: dontcopy
Source: "third/openssl.exe"; DestDir: "{app}"
Source: "third/libeay32.dll"; DestDir: "{app}"
Source: "third/ssleay32.dll"; DestDir: "{app}"
Source: "third/nssm.exe"; DestDir: "{app}"; Check: GetInstallService()
Source: "third/zlib.dll"; DestDir: "{app}"

Source: "../script/jquery.js"; DestDir: "{app}/script"
Source: "../uniproxy.json.sample"; DestDir: "{app}/doc"

[Run]
Filename: "{app}\nssm.exe"; Parameters: "install Uniproxy uniproxy.exe -w {app}"; WorkingDir: "{app}"; StatusMsg: Installing Uniproxy as a service; Check: GetInstallService()
Filename: "sc"; Parameters: "start Uniproxy"; StatusMsg: "Starting Uniproxy service"; Check: GetInstallService()

[UninstallRun]
Filename: "sc"; Parameters: "stop Uniproxy"; StatusMsg: "Stopping Uniproxy service"
Filename: "{app}\nssm.exe"; Parameters: "remove Uniproxy confirm"; WorkingDir: "{app}"; StatusMsg: Uninstalling Uniproxy as a service

[Tasks]
;Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce

[Code]
var
   ServicePage                  : TInputOptionWizardPage;
   UniproxyServiceRunning       : Boolean;
   UniproxyServiceName          : String;

procedure InstallNamedRuntime(Filename : String);
var
   path, arguments : String;
   resultCode      : Integer;
begin
   Log('InstallNamedRuntime('+Filename+')');
   ExtractTemporaryFile(Filename);
   path := AddBackslash(ExpandConstant('{tmp}')) + Filename;
   if (Pos('.msi', Filename) = 0) then
   begin
      // Not a .msi, run directly
      Log('Running: '+path);
      if not Exec(path, '/q', ExpandConstant('{tmp}'), SW_SHOW, ewWaitUntilTerminated, resultCode) then
      begin
         MsgBox('Failed to install Microsoft Visual C++ runtime:' + #13 'Windows error code: ' + IntToStr(resultCode) + #13 +
         SysErrorMessage(resultCode) + #13 + '(' + arguments + ')', mbError, MB_OK);
      end
   end
   else
   begin
      arguments := '/quiet /norestart /i ' + path;
      Log('Running: msiexec with args: ' + arguments + ' in ' + ExpandConstant('{tmp}'));
      if not Exec('msiexec', arguments, ExpandConstant('{tmp}'), SW_SHOW, ewWaitUntilTerminated, resultCode) then
      begin
         MsgBox('Failed to install Microsoft Visual C++ runtime:' + #13 'Windows error code: ' + IntToStr(resultCode) + #13 +
         SysErrorMessage(resultCode) + #13 + '(' + arguments + ')', mbError, MB_OK);
      end;
   end;
   Log('InstallNamedRuntime done');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
   if (CurStep = ssInstall) then
   begin
      UniproxyServiceName := 'Uniproxy';
      UniproxyServiceRunning := IsServiceRunning(UniproxyServiceName);
      if (UniproxyServiceRunning) then
      begin
         Log('Uniproxy service is running');
         StopService(UniproxyServiceName, true);
      end;

      InstallNamedRuntime('vcredist_x86.exe');
   end;
end;

procedure InitializeWizard;
begin
   { Custom page selecting installation of service}
   ServicePage := CreateInputOptionPage(wpLicense,
                                        'Service installation',
                                        '',
                                        'Do you want to install and run the Uniproxy as a windows service?.',
                                        False,
                                        False);
   ServicePage.Add('Install Uniproxy as a Windows service');   
   ServicePage.Values[0] := True;
end;

function GetInstallService(): Boolean;
begin
   Result := ServicePage.Values[0];
end;

