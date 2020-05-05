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

# note the difference between jobs and tasks: https://stackoverflow.com/questions/23467026/are-scheduled-job-and-scheduled-task-the-same-thing-in-powershell-context

# [System.IO.Path]::GetTempFileName()
$monpath = "$env:APPDATA\FluffMon"

Get-ScheduledTask -TaskName FLUFFIMonitoring -ErrorAction SilentlyContinue | Stop-ScheduledTask -ErrorAction SilentlyContinue
# Get-ScheduledTask -TaskName FLUFFIMonitoring -ErrorAction SilentlyContinue | Unregister-ScheduledTask -confirm:$false -ErrorAction SilentlyContinue
Unregister-ScheduledJob -Name FLUFFImonitoring -ErrorAction SilentlyContinue

if([System.IO.Directory]::Exists($monpath)){
    Remove-Item -recurse -path $monpath
}

# create folder in appdata
New-Item -ItemType directory -Path $monpath
# copy stuff there
Copy-Item -Path "$PSScriptRoot\sysmon.ps1" -Destination "$monpath\sysmon.ps1"
Copy-Item -Path "$PSScriptRoot\mqttcli.exe" -Destination "$monpath\mqttcli.exe"

# create job in task scheduler 
$trigger = New-JobTrigger -AtStartup -RandomDelay 00:00:30
Register-ScheduledJob -Trigger $trigger -FilePath "$monpath\sysmon.ps1" -Name FLUFFIMonitoring

$task = Get-ScheduledTask -TaskName FLUFFIMonitoring
$task.settings.executiontimelimit = 'PT0S'
$task | Set-ScheduledTask

Get-ScheduledTask -TaskName FLUFFIMonitoring | Start-ScheduledTask
