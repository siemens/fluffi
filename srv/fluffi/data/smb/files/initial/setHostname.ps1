§§<#
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier
§§#>
§§
§§# Retrieve the MAC-Address of the connected interface and see what host name this system is supposed to have
§§$0 = $myInvocation.MyCommand.Definition
§§$dp0 = [System.IO.Path]::GetDirectoryName($0)
§§$MAC=Get-CimInstance -ClassName Win32_NetworkAdapterConfiguration -Filter "IPEnabled='True' AND DHCPEnabled='True'" | Select-Object -Property MACAddress | % {echo $_.MACAddress}
§§# getmac /S $ipAddr /NH| %{ $line=$_.split(" ") | write-host $line[0]
§§if (-not ([string]::IsNullOrEmpty($MAC)))
§§{
§§	$ComputerName= Import-Csv "$($dp0)MAC2Host.csv" | Where-Object {$_.MAC -eq $MAC} | % {echo $_.Host}
§§
§§	if (-not ([string]::IsNullOrEmpty($ComputerName)))
§§	{
§§		Remove-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -name "Hostname" 
§§		Remove-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -name "NV Hostname" 
§§		New-PSDrive -name HKU -PSProvider "Registry" -Root "HKEY_USERS"
§§		Set-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Control\Computername\Computername" -name "Computername" -value $ComputerName
§§		Set-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Control\Computername\ActiveComputername" -name "Computername" -value $ComputerName
§§		Set-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -name "Hostname" -value $ComputerName
§§		Set-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -name "NV Hostname" -value  $ComputerName
§§		Set-ItemProperty -path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -name "AltDefaultDomainName" -value $ComputerName
§§		Set-ItemProperty -path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -name "DefaultDomainName" -value $ComputerName
§§	}
§§}
