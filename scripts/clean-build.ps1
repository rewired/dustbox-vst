[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$targets = @(
    Join-Path $repoRoot 'build/windows-msvc-debug',
    Join-Path $repoRoot 'build/windows-msvc-release',
    Join-Path $repoRoot '_deploy'
)

Write-Host "Dustbox clean build helper" -ForegroundColor Cyan
Write-Host "This will remove Visual Studio build trees and the _deploy folder." -ForegroundColor Yellow

if(-not $Force) {
    $prompt = Read-Host "Proceed with deletion? [y/N]"
    if($prompt.ToLowerInvariant() -ne 'y') {
        Write-Host "Aborted by user."
        exit 0
    }
}

foreach($path in $targets) {
    if(Test-Path $path) {
        Write-Host "Removing $path"
        Remove-Item -Recurse -Force $path
    } else {
        Write-Host "Skipping $path (not found)"
    }
}

Push-Location $repoRoot
try {
    Write-Host "Configuring Visual Studio Debug preset..."
    cmake --preset windows-msvc-debug
    Write-Host "Building and installing Debug configuration..."
    cmake --build --preset windows-msvc-debug --config Debug --target INSTALL
}
finally {
    Pop-Location
}

Write-Host "Clean build complete. VST3 bundle is under _deploy/VST3/Debug." -ForegroundColor Green
