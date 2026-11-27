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
# Signing and timestamping are done in two steps:
#   1. signtool sign      - the digest is signed by Azure Trusted Signing.
#   2. signtool timestamp - an RFC 3161 timestamp is added, trying several
#                           timestamp servers with retries and a time limit.
# Trusted Signing certificates are only valid for a few days, so a signature
# without a timestamp stops being valid soon after; the script therefore fails
# (exit code 1) unless every file ends up signed AND timestamped.
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

# Timestamp servers in order of preference (Microsoft's is the one recommended for
# Trusted Signing; the others are public RFC 3161 services used as fallbacks).
$timestampServers = @(
    "http://timestamp.acs.microsoft.com",
    "http://timestamp.digicert.com",
    "http://timestamp.sectigo.com",
    "http://timestamp.globalsign.com/tsa/r6advanced1"
)
$timestampRounds = 3            # passes over the whole server list
$timestampTimeoutSec = 60       # per signtool call; a stalled request must not hang the release
$retryDelaysSec = @(0, 10, 30)  # pause before each pass

$files = @($file, $file1, $file2, $file3, $file4, $file5) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
foreach ($f in $files) {
    if (-not (Test-Path -LiteralPath $f -PathType Leaf)) {
        Write-Host "File to sign does not exist: $f"
        exit 1
    }
}

$kitPath = "C:\Program Files (x86)\Windows Kits\10\bin"
$signtool = Get-ChildItem -Path $kitPath -Recurse -File -Filter "signtool.exe" |
    Where-Object { $_.FullName -like "*\x64\*" } |
    Sort-Object FullName -Descending |
    Select-Object -First 1
if (-not $signtool) {
    Write-Host "signtool.exe (x64) was not found under $kitPath"
    exit 1
}
Write-Host "Using signing tool: $($signtool.FullName)"

# Runs signtool with a time limit; returns its exit code (or -1 on timeout).
function Invoke-SignTool([string[]]$arguments, [int]$timeoutSec) {
    $quoted = $arguments | ForEach-Object { if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ } }
    $process = Start-Process -FilePath $signtool.FullName -ArgumentList $quoted -NoNewWindow -PassThru
    if (-not $process.WaitForExit($timeoutSec * 1000)) {
        try { $process.Kill() } catch { }
        Write-Host "signtool did not finish within $timeoutSec s and was stopped."
        return -1
    }
    return $process.ExitCode
}

$tenant = "fffbbdfd-299d-49d6-ba56-a87bb9a3c67e"
$connected = $false
$exitCode = 1
try {
    if ($login) {
        $jsObj = Get-Content ".\CodeSignLoginData.json" -Raw | ConvertFrom-Json
        $secure = ConvertTo-SecureString $jsObj.secretText -AsPlainText -Force
        $cred = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList $jsObj.appId, $secure
        Connect-AzAccount -Tenant $tenant -Credential $cred -ServicePrincipal -ErrorAction Stop | Out-Null
        $connected = $true
    }

    # 1. Sign (no timestamp yet)
    $signArgs = @(
        "sign", "/v", "/debug", "/fd", "SHA256",
        "/dlib", "$env:LOCALAPPDATA\Microsoft\MicrosoftTrustedSigningClientTools\Azure.CodeSigning.Dlib.dll",
        "/dmdf", ".\CodeSignMetadata.json"
    ) + $files
    & $signtool.FullName $signArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Signing failed (signtool exit code $LASTEXITCODE)."
    }

    # 2. Timestamp each file, trying the servers in turn
    foreach ($f in $files) {
        $stamped = $false
        for ($round = 0; $round -lt $timestampRounds -and -not $stamped; $round++) {
            if ($retryDelaysSec[$round] -gt 0) {
                Write-Host "Waiting $($retryDelaysSec[$round]) s before the next timestamp attempt..."
                Start-Sleep -Seconds $retryDelaysSec[$round]
            }
            foreach ($server in $timestampServers) {
                Write-Host "Timestamping '$f' with $server (pass $($round + 1)/$timestampRounds)..."
                $code = Invoke-SignTool @("timestamp", "/tr", $server, "/td", "SHA256", $f) $timestampTimeoutSec
                if ($code -eq 0) {
                    Write-Host "Timestamp added by $server."
                    $stamped = $true
                    break
                }
                Write-Host "Timestamp server $server failed (exit code $code)."
            }
        }
        if (-not $stamped) {
            throw "Could not timestamp '$f' with any timestamp server. The file is signed but NOT timestamped, so its signature would expire with the short-lived certificate."
        }

        $signature = Get-AuthenticodeSignature -LiteralPath $f
        if ($signature.Status -ne 'Valid' -or -not $signature.TimeStamperCertificate) {
            throw "Verification failed for '$f': status $($signature.Status), timestamped: $([bool]$signature.TimeStamperCertificate)."
        }
        Write-Host "Signed and timestamped: $f (timestamp by $($signature.TimeStamperCertificate.Subject))"
    }

    $exitCode = 0
}
catch {
    Write-Host "CodeSign failed: $($_.Exception.Message)"
    if ($_.Exception.InnerException) {
        # Azure hides the real reason (e.g. AADSTS7000215 = invalid client secret) in the inner exception
        Write-Host "Details: $($_.Exception.InnerException.Message)"
    }
    $exitCode = 1
}
finally {
    if ($connected) {
        Disconnect-AzAccount -TenantId $tenant -ApplicationId $jsObj.appId -ErrorAction SilentlyContinue | Out-Null
    }
}

# CreateRelease.ps1 checks $LASTEXITCODE after calling this script
$global:LASTEXITCODE = $exitCode
exit $exitCode
