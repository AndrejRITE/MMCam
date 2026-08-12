Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# -----------------------------------------------------------------------------
# Release configuration
# -----------------------------------------------------------------------------
$git_username = "AndrejRITE"
$repository_name = "MMCam"
$path_to_repository = "D:\Projects\RIGAKU\${repository_name}"

$solutionPath = "${path_to_repository}\${repository_name}.sln"
$temp_folder = "${path_to_repository}\.temp"
$about_folder = "${path_to_repository}\${repository_name}\src\About"
$release_folder = "${path_to_repository}\bin\x64\Release"
$other_files_folder = "${path_to_repository}\${repository_name}"
$redist_source_folder = "${path_to_repository}\redist"
$redist_release_folder = "${release_folder}\redist"
$source_files_folder = "${other_files_folder}\src"
$report_generator_source = "${source_files_folder}\ReportGenerator"
$report_generator_folder = "${release_folder}\src\ReportGenerator"
$src_folder = "${release_folder}\src"

$vcredist_2013_source = "${redist_source_folder}\vcredist_2013_x64.exe"
$vcredist_2015_2022_source = "${redist_source_folder}\VC_redist.x64.exe"

$msbuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
$sevenZipPath = "C:\Program Files\7-Zip\7z.exe"
$innoCompilerPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"

$buildConfiguration = "Release"
$platform = "x64"
$operatingSystem = "win"

$headerFilePath = "${path_to_repository}\${repository_name}\cMain.h"
$src_cpp = "${path_to_repository}\${repository_name}\cMain.cpp"
$src_bak = "${src_cpp}.bak"
$inno_setup_script = "${path_to_repository}\CreateInstaller.iss"
$inno_setup_script_temp = "${path_to_repository}\CreateInstallerTemp.iss"
$icon_full_path = "${path_to_repository}\${repository_name}\src\img\logo.ico"
$logPath = "${path_to_repository}\log.txt"
$oneDriveReleaseFolder = "C:\Users\Andrej Pcelovodov\OneDrive - Rigaku Americas Holding\EXPORT\${repository_name}"

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------
function Write-Log {
    param(
        [Parameter(Mandatory = $true)][string]$Message,
        [ValidateSet('INFO', 'WARN', 'ERROR')][string]$Level = 'INFO'
    )

    $line = "[{0}] [{1}] {2}" -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Level, $Message
    Write-Host $line
    Add-Content -LiteralPath $logPath -Value $line -Encoding UTF8
}

function Assert-FileExists {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file does not exist: $Path"
    }
}

function Assert-DirectoryExists {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "Required directory does not exist: $Path"
    }
}

function Ensure-Directory {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        New-Item -Path $Path -ItemType Directory -Force | Out-Null
    }
}

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter()][string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$Description
    )

    Assert-FileExists $Executable
    Write-Log "Starting: $Description"

    $global:LASTEXITCODE = 0
    & $Executable @Arguments
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        throw "$Description failed with exit code $exitCode."
    }

    Write-Log "Finished: $Description"
}

function Invoke-CommandLineTool {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter()][string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$Description
    )

    $resolvedCommand = Get-Command $Command -ErrorAction SilentlyContinue
    if ($null -eq $resolvedCommand) {
        throw "Required command was not found in PATH: $Command"
    }

    Write-Log "Starting: $Description"
    $global:LASTEXITCODE = 0
    & $Command @Arguments
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        throw "$Description failed with exit code $exitCode."
    }

    Write-Log "Finished: $Description"
}

function Copy-RequiredFile {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    Assert-FileExists $Source
    Copy-Item -LiteralPath $Source -Destination $Destination -Force

    if (-not (Test-Path -LiteralPath $Destination -PathType Leaf)) {
        throw "Copy operation did not create expected file: $Destination"
    }
}

function Get-RequiredEnvironmentDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$VariableName,
        [string]$Suffix = ''
    )

    $value = [Environment]::GetEnvironmentVariable($VariableName)
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "Required environment variable '$VariableName' is not defined."
    }

    $path = if ([string]::IsNullOrEmpty($Suffix)) { $value } else { Join-Path $value $Suffix }
    Assert-DirectoryExists $path
    return $path
}

function Get-RequiredRegexValue {
    param(
        [Parameter(Mandatory = $true)][string]$Content,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $match = [regex]::Match($Content, $Pattern)
    if (-not $match.Success -or [string]::IsNullOrWhiteSpace($match.Groups[1].Value)) {
        throw "Unable to extract $Name using pattern '$Pattern'."
    }

    return $match.Groups[1].Value
}

# -----------------------------------------------------------------------------
# Main release pipeline
# -----------------------------------------------------------------------------
$sourceBackupCreated = $false
$releaseSucceeded = $false

try {
    Assert-DirectoryExists $path_to_repository
    Set-Location -LiteralPath $path_to_repository

    Write-Output "-------------------------------------- [$(Get-Date)] --------------------------------------" | Add-Content -LiteralPath $logPath
    Write-Log "Release script started for ${repository_name}."

    # Resolve SDK locations only after logging is available.
    $advacam_folder = Get-RequiredEnvironmentDirectory -VariableName 'ADVACAM_LATEST'
    $libximc_folder = Get-RequiredEnvironmentDirectory -VariableName 'LIBXIMC_LATEST' -Suffix 'win64'
    $ximea_folder = Get-RequiredEnvironmentDirectory -VariableName 'XIMEA_LATEST'
    $misdk_folder = Get-RequiredEnvironmentDirectory -VariableName 'MISDK_LATEST' -Suffix 'x64'

    # Validate all tools and static inputs before changing any files.
    Write-Log 'Validating release prerequisites.'
    foreach ($requiredFile in @(
        $solutionPath,
        $headerFilePath,
        $src_cpp,
        $inno_setup_script,
        $icon_full_path,
        $msbuildPath,
        $sevenZipPath,
        $innoCompilerPath,
        $vcredist_2013_source,
        $vcredist_2015_2022_source,
        "${other_files_folder}\keyfile.sqlite",
        "${other_files_folder}\${repository_name}.ini",
        "${other_files_folder}\pixet.ini",
        "${other_files_folder}\table.txt",
        "${other_files_folder}\Xeryon.py",
        "${other_files_folder}\xeryon_goCenter.py",
        "${other_files_folder}\xeryon_setAbsolutePosition.py",
        "${other_files_folder}\requirements.txt",
        "${libximc_folder}\bindy.dll",
        "${libximc_folder}\libximc.dll",
        "${libximc_folder}\xiwrapper.dll",
        "${advacam_folder}\ftd2xx64.dll",
        "${advacam_folder}\pxcore.dll",
        "${ximea_folder}\xiapi64.dll",
        "${misdk_folder}\cXusb.dll",
        "${misdk_folder}\gXeth.dll"
    )) {
        Assert-FileExists $requiredFile
    }

    foreach ($requiredDirectory in @(
        $about_folder,
        $source_files_folder,
        $report_generator_source,
        "${advacam_folder}\hwlibs"
    )) {
        Assert-DirectoryExists $requiredDirectory
    }

    if ($null -eq (Get-Command git -ErrorAction SilentlyContinue)) {
        throw 'Git was not found in PATH.'
    }
    if ($null -eq (Get-Command gh -ErrorAction SilentlyContinue)) {
        throw 'GitHub CLI (gh) was not found in PATH.'
    }

    Ensure-Directory $release_folder
    Ensure-Directory $oneDriveReleaseFolder

    # Version information.
    $fileContent = Get-Content -LiteralPath $headerFilePath -Raw
    $major_version = Get-RequiredRegexValue -Content $fileContent -Pattern 'MAJOR_VERSION\s+(\d+)' -Name 'MAJOR_VERSION'
    $minor_version = Get-RequiredRegexValue -Content $fileContent -Pattern 'MINOR_VERSION\s+(\d+)' -Name 'MINOR_VERSION'

    $global:LASTEXITCODE = 0
    $commit_number = (& git rev-list --count HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $commit_number -notmatch '^\d+$') {
        throw "Unable to determine a valid Git commit count. Git exit code: $LASTEXITCODE; output: '$commit_number'."
    }

    $now = Get-Date
    $current_year = $now.ToString('yyyy')
    $current_month = $now.ToString('MM')
    $current_day = $now.ToString('dd')
    $build_version = "${major_version}.${minor_version}.${commit_number}"
    $tag_name = "v${build_version}"
    $archive_name = "${repository_name}_v${build_version}.7z"
    $archive_path = Join-Path $release_folder $archive_name
    $installer_name = "${repository_name}Installer_v${build_version}_${operatingSystem}_${platform}.exe"
    $installer_name_without_extension = [System.IO.Path]::GetFileNameWithoutExtension($installer_name)
    $installer_path = Join-Path $release_folder $installer_name

    Write-Log "Release version: $build_version"

    # Patch build metadata into cMain.cpp, build, then restore the source no matter what.
    Write-Log 'Patching build metadata into cMain.cpp.'
    Copy-Item -LiteralPath $src_cpp -Destination $src_bak -Force
    $sourceBackupCreated = $true

    try {
        $content = Get-Content -LiteralPath $src_cpp -Raw
        $content = $content.Replace('{#CommitNumber}', $commit_number)
        $content = $content.Replace('{#CurrentYear}', $current_year)
        $content = $content.Replace('{#CurrentMonth}', $current_month)
        $content = $content.Replace('{#CurrentDay}', $current_day)
        Set-Content -LiteralPath $src_cpp -Value $content -Encoding UTF8

        Invoke-NativeCommand -Executable $msbuildPath -Arguments @(
            $solutionPath,
            "/p:Configuration=$buildConfiguration",
            "/p:Platform=$platform",
            '/t:Build'
        ) -Description "MSBuild ${repository_name} ${buildConfiguration}|${platform}"
    }
    finally {
        if ($sourceBackupCreated -and (Test-Path -LiteralPath $src_bak -PathType Leaf)) {
            Move-Item -LiteralPath $src_bak -Destination $src_cpp -Force
            $sourceBackupCreated = $false
            Write-Log 'Restored original cMain.cpp.'
        }
    }

    $applicationExe = Join-Path $release_folder "${repository_name}.exe"
    Assert-FileExists $applicationExe

    # Create a clean staging area.
    if (Test-Path -LiteralPath $temp_folder) {
        Remove-Item -LiteralPath $temp_folder -Recurse -Force
    }
    Ensure-Directory $temp_folder

    # Prepare installer script.
    Copy-RequiredFile -Source $inno_setup_script -Destination $inno_setup_script_temp

    # Create About.zip from a clean output file.
    Copy-Item -Path (Join-Path $about_folder '*') -Destination $temp_folder -Recurse -Force
    $about_app_file = Join-Path $temp_folder 'about_app.html'
    Assert-FileExists $about_app_file

    $aboutContent = Get-Content -LiteralPath $about_app_file -Raw
    $aboutContent = $aboutContent.Replace('{#Major}', $major_version)
    $aboutContent = $aboutContent.Replace('{#Minor}', $minor_version)
    $aboutContent = $aboutContent.Replace('{#Year}', $current_year)
    Set-Content -LiteralPath $about_app_file -Value $aboutContent -Encoding UTF8

    $about_zip = Join-Path $release_folder 'About.zip'
    if (Test-Path -LiteralPath $about_zip) {
        Remove-Item -LiteralPath $about_zip -Force
    }
    Invoke-NativeCommand -Executable $sevenZipPath -Arguments @('a', '-tzip', $about_zip, (Join-Path $temp_folder '*')) -Description 'Create About.zip'
    Assert-FileExists $about_zip

    # Remove previous installers and release archives only after a successful build.
    Get-ChildItem -LiteralPath $release_folder -Filter '*.exe' -File |
        Where-Object { $_.Name -ne "${repository_name}.exe" } |
        Remove-Item -Force

    Get-ChildItem -LiteralPath $release_folder -Filter '*.7z' -File | Remove-Item -Force

    # Runtime dependencies.
    Write-Log 'Copying runtime dependencies.'
    Ensure-Directory $redist_release_folder
    Copy-RequiredFile $vcredist_2013_source (Join-Path $redist_release_folder 'vcredist_2013_x64.exe')
    Copy-RequiredFile $vcredist_2015_2022_source (Join-Path $redist_release_folder 'VC_redist.x64.exe')

    Copy-RequiredFile "${libximc_folder}\bindy.dll" (Join-Path $release_folder 'bindy.dll')
    Copy-RequiredFile "${libximc_folder}\libximc.dll" (Join-Path $release_folder 'libximc.dll')
    Copy-RequiredFile "${libximc_folder}\xiwrapper.dll" (Join-Path $release_folder 'xiwrapper.dll')
    Copy-RequiredFile "${other_files_folder}\keyfile.sqlite" (Join-Path $release_folder 'keyfile.sqlite')
    Copy-RequiredFile "${other_files_folder}\${repository_name}.ini" (Join-Path $release_folder "${repository_name}.ini")
    Copy-RequiredFile "${other_files_folder}\pixet.ini" (Join-Path $release_folder 'pixet.ini')
    Copy-RequiredFile "${advacam_folder}\ftd2xx64.dll" (Join-Path $release_folder 'ftd2xx64.dll')
    Copy-RequiredFile "${advacam_folder}\pxcore.dll" (Join-Path $release_folder 'pxcore.dll')

    $hwlibsDestination = Join-Path $release_folder 'hwlibs'
    if (Test-Path -LiteralPath $hwlibsDestination) {
        Remove-Item -LiteralPath $hwlibsDestination -Recurse -Force
    }
    Copy-Item -LiteralPath "${advacam_folder}\hwlibs" -Destination $release_folder -Recurse -Force
    Assert-DirectoryExists $hwlibsDestination

    Copy-RequiredFile "${ximea_folder}\xiapi64.dll" (Join-Path $release_folder 'xiapi64.dll')
    Copy-RequiredFile "${misdk_folder}\cXusb.dll" (Join-Path $release_folder 'cXusb.dll')
    Copy-RequiredFile "${misdk_folder}\gXeth.dll" (Join-Path $release_folder 'gXeth.dll')
    Copy-RequiredFile "${other_files_folder}\table.txt" (Join-Path $release_folder 'table.txt')
    Copy-RequiredFile "${other_files_folder}\Xeryon.py" (Join-Path $release_folder 'Xeryon.py')
    Copy-RequiredFile "${other_files_folder}\xeryon_goCenter.py" (Join-Path $release_folder 'xeryon_goCenter.py')
    Copy-RequiredFile "${other_files_folder}\xeryon_setAbsolutePosition.py" (Join-Path $release_folder 'xeryon_setAbsolutePosition.py')
    Copy-RequiredFile "${other_files_folder}\requirements.txt" (Join-Path $release_folder 'requirements.txt')

    # JSON resources.
    Ensure-Directory $src_folder
    $json_files = @(Get-ChildItem -LiteralPath $source_files_folder -Filter '*.json' -File |
        Where-Object { $_.Name -notlike 'debug_*.json' })
    if ($json_files.Count -eq 0) {
        throw "No release JSON files were found in: $source_files_folder"
    }
    foreach ($file in $json_files) {
        Copy-Item -LiteralPath $file.FullName -Destination $src_folder -Force
    }

    # ReportGenerator resources.
    if (Test-Path -LiteralPath $report_generator_folder) {
        Remove-Item -LiteralPath $report_generator_folder -Recurse -Force
    }
    Ensure-Directory $report_generator_folder

    $reportFiles = @(Get-ChildItem -LiteralPath $report_generator_source -Recurse -File |
        Where-Object {
            $_.Extension -match '^\.(py|tex|txt|png)$' -and $_.FullName -notlike '*\.venv*'
        })
    if ($reportFiles.Count -eq 0) {
        throw "No ReportGenerator release resources were found in: $report_generator_source"
    }
    foreach ($file in $reportFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $report_generator_folder $file.Name) -Force
    }

    # Temp staging is no longer needed.
    Remove-Item -LiteralPath $temp_folder -Recurse -Force

    # Build release archive from verified inputs.
    $files_to_archive = @(
        $src_folder,
        $redist_release_folder,
        $about_zip,
        (Join-Path $release_folder 'keyfile.sqlite'),
        $applicationExe,
        (Join-Path $release_folder "${repository_name}.ini"),
        (Join-Path $release_folder 'table.txt'),
        (Join-Path $release_folder 'Xeryon.py'),
        (Join-Path $release_folder 'xeryon_goCenter.py'),
        (Join-Path $release_folder 'xeryon_setAbsolutePosition.py'),
        (Join-Path $release_folder 'requirements.txt')
    )
    $files_to_archive += @(Get-ChildItem -LiteralPath $release_folder -Filter '*.dll' -File | ForEach-Object { $_.FullName })

    foreach ($archiveInput in $files_to_archive) {
        if (-not (Test-Path -LiteralPath $archiveInput)) {
            throw "Archive input is missing: $archiveInput"
        }
    }

    if (Test-Path -LiteralPath $archive_path) {
        Remove-Item -LiteralPath $archive_path -Force
    }
    Invoke-NativeCommand -Executable $sevenZipPath -Arguments (@('a', '-t7z', $archive_path) + $files_to_archive) -Description "Create release archive $archive_name"
    Assert-FileExists $archive_path

    # Release notes source.
    $global:LASTEXITCODE = 0
    $commit_message = (& git log -1 --pretty=%B | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($commit_message)) {
        throw "Unable to retrieve the latest Git commit message. Git exit code: $LASTEXITCODE."
    }

    # Patch Inno Setup script without regex replacement semantics.
    $innoContent = Get-Content -LiteralPath $inno_setup_script_temp -Raw
    $innoContent = $innoContent.Replace('{#Major}', $major_version)
    $innoContent = $innoContent.Replace('{#Minor}', $minor_version)
    $innoContent = $innoContent.Replace('{#Build}', $commit_number)
    $innoContent = $innoContent.Replace('{#RepoName}', $repository_name)
    $innoContent = $innoContent.Replace('{#OutputBaseFilename}', $installer_name_without_extension)
    $innoContent = $innoContent.Replace('{#OutputDir}', $release_folder)
    $innoContent = $innoContent.Replace('{#IconFullPath}', $icon_full_path)
    Set-Content -LiteralPath $inno_setup_script_temp -Value $innoContent -Encoding UTF8

    Invoke-NativeCommand -Executable $innoCompilerPath -Arguments @($inno_setup_script_temp) -Description 'Generate installer with Inno Setup'
    Assert-FileExists $installer_path

    # Sign and independently verify the resulting installer.
    $code_sign_script = Join-Path $PSScriptRoot 'CodeSign.ps1'
    Assert-FileExists $code_sign_script
    Write-Log "Signing installer: $installer_name"
    $global:LASTEXITCODE = 0
    & $code_sign_script -file $installer_path
    if ($LASTEXITCODE -ne 0) {
        throw "CodeSign.ps1 failed with exit code $LASTEXITCODE."
    }

    $signature = Get-AuthenticodeSignature -LiteralPath $installer_path
    if ($signature.Status -ne 'Valid') {
        throw "Installer signature verification failed. Authenticode status: $($signature.Status). Status message: $($signature.StatusMessage)"
    }
    Write-Log "Installer signature verified. Signer: $($signature.SignerCertificate.Subject)"

    $fileHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $installer_path).Hash
    if ([string]::IsNullOrWhiteSpace($fileHash)) {
        throw 'Unable to calculate installer SHA256 hash.'
    }
    Write-Log "Installer SHA256: $fileHash"

    $release_notes = @"
## Release Notes for ${repository_name}_v${build_version}

### Commit Message
- ${commit_message}

### Download Links
- [Download ${installer_name}](https://github.com/${git_username}/${repository_name}/releases/download/${tag_name}/${installer_name})
- [Download ${archive_name}](https://github.com/${git_username}/${repository_name}/releases/download/${tag_name}/${archive_name})

### SHA256
```
${fileHash}
```
"@

    # Publish only after all local artifacts have been built and verified.
    Invoke-CommandLineTool -Command 'gh' -Arguments @(
        'release', 'create', $tag_name,
        $installer_path, $archive_path,
        '--title', "Release $tag_name",
        '--notes', $release_notes
    ) -Description "Publish GitHub release $tag_name"

    # OneDrive copy is part of the release contract, therefore failures are fatal.
    Write-Log 'Publishing release artifacts to OneDrive.'
    $oneDriveInstaller = Join-Path $oneDriveReleaseFolder $installer_name
    $oneDriveArchive = Join-Path $oneDriveReleaseFolder $archive_name
    Copy-RequiredFile $installer_path $oneDriveInstaller
    Copy-RequiredFile $archive_path $oneDriveArchive

    $releaseSucceeded = $true
    Write-Log "Release $tag_name completed successfully."
}
catch {
    $errorMessage = $_.Exception.Message
    try {
        Write-Log "Release failed: $errorMessage" 'ERROR'
        if ($_.ScriptStackTrace) {
            Write-Log "Stack trace: $($_.ScriptStackTrace)" 'ERROR'
        }
    }
    catch {
        Write-Error "Release failed: $errorMessage"
    }

    throw
}
finally {
    # Defensive cleanup in case failure happened during a transient stage.
    if ($sourceBackupCreated -and (Test-Path -LiteralPath $src_bak -PathType Leaf)) {
        try {
            Move-Item -LiteralPath $src_bak -Destination $src_cpp -Force
        }
        catch {
            Write-Warning "CRITICAL: Failed to restore source backup '$src_bak': $($_.Exception.Message)"
        }
    }

    if (Test-Path -LiteralPath $inno_setup_script_temp -PathType Leaf) {
        Remove-Item -LiteralPath $inno_setup_script_temp -Force -ErrorAction SilentlyContinue
    }

    if (Test-Path -LiteralPath $temp_folder -PathType Container) {
        Remove-Item -LiteralPath $temp_folder -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (-not $releaseSucceeded) {
        Write-Warning 'Release pipeline terminated before successful completion. See log.txt for details.'
    }
}