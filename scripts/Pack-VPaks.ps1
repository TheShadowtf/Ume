param(
    [string]$ProjectFile = "UmeEditor/SandboxProject/Sandbox.uproj",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug",

    [switch]$SkipBuild,

    [switch]$VerbosePack
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build-cmake"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake was not found in PATH."
}

$projectPath = if ([System.IO.Path]::IsPathRooted($ProjectFile)) { $ProjectFile } else { Join-Path $root $ProjectFile }
$projectPath = [System.IO.Path]::GetFullPath($projectPath)

if (-not (Test-Path $projectPath)) {
    throw "Project file was not found: $projectPath"
}

$projectName = [System.IO.Path]::GetFileNameWithoutExtension($projectPath)
$projectDir = Split-Path -Parent $projectPath

if (-not $SkipBuild) {
    Write-Host "[1/3] Configuring CMake ($Config)..."
    cmake -S $root -B $buildDir -G "Visual Studio 17 2022" -A x64 `
        -DUME_BUILD_EDITOR=ON `
        -DUME_BUILD_LAUNCHER=ON `
        -DUME_BUILD_SANDBOX=OFF `
        -DUME_BUILD_VPAK_TOOL=ON

    Write-Host "[2/3] Building UmeVPackTool ($Config)..."
    cmake --build $buildDir --config $Config --target UmeVPackTool
}

$toolExe = Join-Path $buildDir "bin/$Config/UmeVPackTool.exe"
if (-not (Test-Path $toolExe)) {
    throw "Could not find executable: $toolExe"
}

$outputDir = Join-Path $projectDir "Build/Paks"
$copyExport = Join-Path $projectDir "Export/Windows/Content/Paks"

Write-Host "[3/3] Building texture VPAKs..."
if ($VerbosePack) {
    & $toolExe --project $projectPath --output $outputDir --copy-to $copyExport --verbose
} else {
    & $toolExe --project $projectPath --output $outputDir --copy-to $copyExport
}

if ($LASTEXITCODE -ne 0) {
    throw "UmeVPackTool failed with exit code $LASTEXITCODE"
}

Write-Host "Done."
Write-Host "  Project: $projectName"
Write-Host "  Runtime output: $outputDir"
Write-Host "  Export output:  $copyExport"
