param(
    [ValidateSet("UmeLauncher", "UmeEditor")]
    [string]$Target = "UmeLauncher",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug",

    [string]$ProjectFile = "UmeEditor/SandboxProject/Sandbox.uproj",

    [switch]$PackVpaks
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root "build-cmake"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake was not found in PATH."
}

Write-Host "[1/3] Configuring CMake ($Config)..."
cmake -S $root -B $buildDir -G "Visual Studio 17 2022" -A x64 `
    -DUME_BUILD_EDITOR=ON `
    -DUME_BUILD_LAUNCHER=ON `
    -DUME_BUILD_SANDBOX=OFF `
    -DUME_BUILD_VPAK_TOOL=ON

Write-Host "[2/3] Building UmeEditor + UmeLauncher ($Config)..."
cmake --build $buildDir --config $Config --target UmeEditor UmeLauncher

if ($PackVpaks) {
    $packScript = Join-Path $root "scripts/Pack-VPaks.ps1"
    if (-not (Test-Path $packScript)) {
        throw "Could not find VPAK pack script: $packScript"
    }

    Write-Host "[2.5/3] Packing VPAK assets..."
    & $packScript -ProjectFile $ProjectFile -Config $Config -SkipBuild
    if ($LASTEXITCODE -ne 0) {
        throw "VPAK packaging failed with exit code $LASTEXITCODE"
    }
}

$exe = Join-Path $buildDir "bin\$Config\$Target.exe"
if (-not (Test-Path $exe)) {
    throw "Could not find executable: $exe"
}

Write-Host "[3/3] Running $Target..."
$workingDir = if ($Target -eq "UmeEditor") { Join-Path $root "UmeEditor" } else { Join-Path $root "UmeLauncher" }
Push-Location $workingDir
try {
    & $exe
}
finally {
    Pop-Location
}
