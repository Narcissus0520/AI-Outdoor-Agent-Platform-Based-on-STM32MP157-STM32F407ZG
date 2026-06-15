param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourceDir = Join-Path $repoRoot "f407/sensor-hub/firmware"
$buildDir = Join-Path $sourceDir "build"
$toolchainFile = Join-Path $sourceDir "cmake/arm-none-eabi-gcc.cmake"

$configureArgs = @(
    "-S", $sourceDir,
    "-B", $buildDir,
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

$cmakeCache = Join-Path $buildDir "CMakeCache.txt"
if (-not (Test-Path -LiteralPath $cmakeCache)) {
    $configureArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile"
}

$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninja) {
    $configureArgs += @("-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=$($ninja.Source)")
}

& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build $buildDir
exit $LASTEXITCODE
