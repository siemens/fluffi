<#
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann, Thomas Riedmaier
#>

# detect if win server 2008, if yes, install 

Write-Host (Get-WmiObject -class Win32_OperatingSystem).Caption

If((Get-WmiObject -class Win32_OperatingSystem).Caption -like '*2008*'){
	$scriptRoot = Split-Path $script:MyInvocation.MyCommand.Path
	$Source = "$scriptRoot\updatesWS2008"
	$Destination = "C:\updatesWS2008\"
	New-Item -ItemType directory -Path $Destination -Force
	Copy-Item -Path $Source\*.* -Destination $Destination -Force

	$dotNetPath = "C:\updatesWS2008\NDP452-KB2901907-x86-x64-AllOS-ENU.exe"
	$proc1 = Start-Process $dotNetPath -argumentlist "/passive /norestart" -Passthru
	do {start-sleep -Milliseconds 500}
	until ($proc1.HasExited)

	$updatePath = "C:\updatesWS2008\Windows6.1-KB2819745-x64-MultiPkg.msu" 
	$proc2 = Start-Process "wusa" -argumentlist "$updatePath /quiet /norestart" -Passthru
	do {start-sleep -Milliseconds 500}
	until ($proc2.HasExited)

}
