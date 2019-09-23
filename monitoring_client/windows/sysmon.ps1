<#
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann, Thomas Riedmaier
#>

§§
§§$BROKER = "mon.fluffi"
§§$mqttbin = "$env:APPDATA\FluffMon\mqttcli.exe"
§§
§§$NODE = $env:COMPUTERNAME
§§

§§function Get-PubCall {
§§    [CmdletBinding()]
§§    param(
§§        [string] $SubTopic,
§§        [string] $Message
§§    )    
§§    return "$mqttbin pub --host $BROKER -q 2 -t FLUFFI/state/$NODE/$SubTopic -m '$Message'"
§§}
§§
§§function Do-Publish{
§§    [CmdletBinding()]
§§    param(
§§        [string] $Measurement,
§§        [string] $Value
§§    )
§§    Invoke-Expression $(Get-PubCall -SubTopic "$Measurement" -Message "$Value" )
§§}
§§
§§function Get-CpuLoad {
§§    $a = (Get-WmiObject -Query "select PercentProcessorTime from Win32_PerfFormattedData_PerfOS_Processor")
§§    $b = 0
§§    $a | foreach-object { 
§§        #write-host "$($_.Name): $($_.PercentProcessorTime)" 
§§        $b = $b + $($_.PercentProcessorTime)
§§    }
§§    return $b/($a.Count-1)
§§}
§§
§§
§§# register with 'uname -a'
§§Do-Publish -Measurement "register" -Value "$((Get-WmiObject Win32_OperatingSystem).Caption) $((Get-WmiObject Win32_OperatingSystem).Version)"
§§
§§while ($true) {
§§    # cpu load as percentage 0.0-100.0
§§    Do-Publish -Measurement "cpuload" -Value ("$(Get-CpuLoad)" -replace ',','.')
§§
§§    # free memory in kB
§§    $os = Get-Ciminstance Win32_OperatingSystem
§§    Do-Publish -Measurement "availablemem" -Value ([math]::Round($os.FreePhysicalMemory,0))
§§    
§§    # swap/pagefile used
§§    Do-Publish -Measurement "swap" -Value "$((Get-Ciminstance Win32_PageFileUsage).CurrentUsage * 1024)"
§§
§§    # disk used of each volume in percent"
§§    Get-WmiObject Win32_LogicalDisk | Select-Object DeviceID,VolumeName,Size,FreeSpace | foreach-object {
§§        Do-Publish -Measurement ("vol:"+("$($_.DeviceID)" -replace ":","")+"-$($_.VolumeName)") -Value "$(100*[Math]::Round(($_.Size-$_.FreeSpace)/$_.Size,4))"
§§    }
§§    
§§    # uptime
§§    $uptime = [math]::Round( ((Get-Date) - ([Management.ManagementDateTimeConverter]::ToDateTime( (Get-WmiObject Win32_OperatingSystem).LastBootUpTime )) ).TotalSeconds, 2)
§§    Do-Publish -Measurement "uptime" -Value ("$uptime" -replace ',','.')
§§    
§§    Start-Sleep -s 5
§§}
§§
§§# unregister when finished with 'uname -a'
§§Do-Publish -Measurement "unregister" -Value "$((Get-WmiObject Win32_OperatingSystem).Caption) $((Get-WmiObject Win32_OperatingSystem).Version)"
§§#Remove-Item $mqttbin
§§
