:: Copyright 2017-2020 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person
:: obtaining a copy of this software and associated documentation files (the
:: "Software"), to deal in the Software without restriction, including without
:: limitation the rights to use, copy, modify, merge, publish, distribute,
:: sublicense, and/or sell copies of the Software, and to permit persons to whom the
:: Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be
:: included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
:: EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
:: MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
:: SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
:: OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
:: ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
:: DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier, Pascal Eckmann

:: Inital configuration script to prepare a fresh linux system for FLUFFI. Essentially:
:: - Configure Networking
:: - Set Hostname
:: - Prepare for Ansible

:: prepare powershell
powershell -Command "Set-ExecutionPolicy -ExecutionPolicy Bypass -Force"
C:\Windows\SysWOW64\cmd.exe /c powershell -Command "Set-ExecutionPolicy -ExecutionPolicy Bypass -Force"

:: Configure WinRM
call winrm quickconfig -q
call winrm quickconfig -transport:http
call winrm set winrm/config @{MaxTimeoutms="1800000"}
call winrm set winrm/config/winrs @{MaxMemoryPerShellMB="300"}
call winrm set winrm/config/service @{AllowUnencrypted="true"}
call winrm set winrm/config/service/auth @{Basic="true"}
call winrm set winrm/config/service/auth @{Negotiate="true"}
call winrm set winrm/config/listener?Address=*+Transport=HTTP @{Port="5985"}
net stop winrm
sc config winrm start= auto
net start winrm

:: Disable Firewall 
%SystemRoot%\System32\netsh.exe advfirewall set allprofiles state off

:: Execute powershell scripts
Powershell.exe -executionpolicy bypass -File "%~dp0installUpdatesOnWS2008.ps1"
Powershell.exe -executionpolicy bypass -File "%~dp0setHostname.ps1"
Powershell.exe -executionpolicy bypass -File "%~dp0reboot.ps1"

