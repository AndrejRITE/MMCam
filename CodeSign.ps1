#
# Copyright 2025      Rigaku Americas
#                     9009 New Trails Drive
#                     The Woodlands, TX, USA  77381
#
# The contents are unpublished proprietary source
# code of Rigaku Americas.
#
# All rights reserved.
#
# Powershell script for signing files using Azure Trusted Signing.
#
#
#
param(
    [string]$file = "",
    [string]$file1 = "",
    [string]$file2 = "",
    [string]$file3 = "",
    [string]$file4 = "",
    [string]$file5 = "",
    [boolean]$login = $true
)
if (-not $file) {
    Write-Host "Usage: CodeSign.ps1 -file <path to file to sign>"
    exit 1
}


if ($login) {
  $jsObj = Get-Content ".\CodeSignLoginData.json" | ConvertFrom-Json
  $tenent = "fffbbdfd-299d-49d6-ba56-a87bb9a3c67e"
  $secure = ConvertTo-SecureString $jsObj.secretText -AsPlainText -Force
  $cred = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList $jsObj.appId, $secure
  Connect-AzAccount -Tenant $tenent -Credential $cred -ServicePrincipal | Out-Null
}

$args = @(
    "sign", "/v", "/debug", "/fd", "SHA256",
    "/tr", "http://timestamp.acs.microsoft.com",
    "/td", "SHA256",
    "/dlib", "$env:LOCALAPPDATA\Microsoft\MicrosoftTrustedSigningClientTools\Azure.CodeSigning.Dlib.dll",
    "/dmdf", ".\CodeSignMetadata.json"
)
foreach ($f in @($file, $file1, $file2, $file3, $file4, $file5)) {
    if (![string]::IsNullOrWhiteSpace($f)) {
        $args += "$f"
    }
}

$kitPath = "C:\Program Files (x86)\Windows Kits\10\bin"
$files = Get-ChildItem -Path $kitPath -Recurse -File -Filter "signtool.exe" | Where-Object { $_ -like "*\x64\*" }
[System.Array]::Reverse($files)
$signtool = $files[0]

Write-Host "Using signing tool: $signtool"

& $signtool $args

if ($login) {
   Disconnect-AzAccount -TenantId $tenent -ApplicationId $jsObj.appId | Out-Null
}
