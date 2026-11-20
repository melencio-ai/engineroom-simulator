# Installs a downloaded GitHub Actions PSP artifact without GitHub CLI.
# Close PPSSPP first. This installs a PREVIEW EBOOT and never modifies GitHub.
[CmdletBinding()]
param(
    [string]$Downloads = 'D:\Downloads',
    [string]$ZipPath = '',
    [string]$GameDirectory = (Join-Path $env:USERPROFILE 'Documents\PPSSPP\PSP\GAME\ENGINEWATCH')
)

$ErrorActionPreference = 'Stop'
$artifactPattern = 'engine-watch-psp-g38-humanoid-quality-preview*.zip'

if (Get-Process -Name 'PPSSPPWindows64','PPSSPPWindows' -ErrorAction SilentlyContinue) {
    throw 'Close PPSSPP completely before replacing EBOOT.PBP.'
}
if (-not $ZipPath) {
    if (-not (Test-Path -LiteralPath $Downloads -PathType Container)) {
        throw "Downloads directory not found: $Downloads"
    }
    $archive = Get-ChildItem -LiteralPath $Downloads -Filter $artifactPattern -File |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $archive) {
        throw "Artifact ZIP not found in $Downloads. Download '$artifactPattern' from the G3.8 humanoid quality GitHub Actions build, or supply -ZipPath."
    }
    $ZipPath = $archive.FullName
}
if (-not (Test-Path -LiteralPath $ZipPath -PathType Leaf)) {
    throw "Artifact ZIP not found: $ZipPath"
}

$temp = Join-Path ([System.IO.Path]::GetTempPath()) ("enginewatch-g38-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    Expand-Archive -LiteralPath $ZipPath -DestinationPath $temp -Force
    $eboot = Get-ChildItem -LiteralPath $temp -Filter 'EBOOT.PBP' -File -Recurse |
        Where-Object { $_.FullName -match '[\\/]PSP[\\/]GAME[\\/]ENGINEWATCH[\\/]EBOOT\.PBP$' } |
        Select-Object -First 1
    if (-not $eboot) {
        throw 'The ZIP does not contain PSP/GAME/ENGINEWATCH/EBOOT.PBP.'
    }
    if ($eboot.Length -lt 1024) { throw 'EBOOT.PBP looks unexpectedly small.' }
    $avatar = Join-Path $eboot.DirectoryName 'ENGINEER.EWM'
    if (-not (Test-Path -LiteralPath $avatar -PathType Leaf)) {
        throw 'The ZIP must contain PSP/GAME/ENGINEWATCH/ENGINEER.EWM (actual imported Quaternius body).'
    }
    $bytes = [System.IO.File]::ReadAllBytes($avatar)
    if ($bytes.Length -lt 64 -or $bytes.Length -gt 1350000 -or
        [System.Text.Encoding]::ASCII.GetString($bytes,0,4) -ne 'EWHM') {
        throw 'Imported Quaternius humanoid animation file is missing or invalid.'
    }
    New-Item -ItemType Directory -Path $GameDirectory -Force | Out-Null
    $target = Join-Path $GameDirectory 'EBOOT.PBP'
    if (Test-Path -LiteralPath $target) {
        $backup = "$target.bak-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
        Copy-Item -LiteralPath $target -Destination $backup -Force
        Write-Host "Backed up previous game to $backup"
    }
    $avatarTarget = Join-Path $GameDirectory 'ENGINEER.EWM'
    if (Test-Path -LiteralPath $avatarTarget) {
        $backupAvatar = "$avatarTarget.bak-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
        Copy-Item -LiteralPath $avatarTarget -Destination $backupAvatar -Force
        Write-Host "Backed up prior 3D character to $backupAvatar"
    }
    Copy-Item -LiteralPath $eboot.FullName -Destination $target -Force
    Copy-Item -LiteralPath $avatar -Destination $avatarTarget -Force
    Write-Host "Installed Engine Watch G3.8 GROUNDED ENGINEER PREVIEW: $target"
    Write-Host "Installed the actual animated Quaternius Worker: $avatarTarget"
    Write-Host 'Start a NEW game. SELECT: view improved Quaternius engineer in 3P. Check stair foot contact, descent pacing, body lean and station interactions.'
}
finally {
    if (Test-Path -LiteralPath $temp) {
        Remove-Item -LiteralPath $temp -Recurse -Force
    }
}
