# project: SeamlyLayout
# author: slspencer, copyright 2026
# MIT License: https://opensource.org/licenses/MIT
#
# build.ps1 — Build script for SeamlyLayout Qt frontend
# Sets up VS 2025 x64 environment, runs CMake configure + build, then launches the app
param(
    [ValidateSet("debug", "release")]
    [string]$Preset = "debug",
    [switch]$Clean
)

Write-Host "Starting build process for SeamlyLayout..."
Write-Host "Requires Rust 2021, Qt 6.10.1, and Visual Studio 2025 Community Edition with C++ workload installed..."
Write-Host "Ignore warnings about missing WrapVulkanHeaders..."
Write-Host "Ignore warnings about missing pthreads..."

$ErrorActionPreference = "Stop"

# Enable Claude prompt suggestion for code
$env:CLAUDE_CODE_ENABLE_PROMPT_SUGGESTION = "1"

Write-Host "1 Setting paths..."

# Paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$FrontendDir = $null

if ((Test-Path (Join-Path $ScriptDir "CMakeLists.txt")) -and (Test-Path (Join-Path $ScriptDir "CMakePresets.json"))) {
    $FrontendDir = $ScriptDir
} elseif ((Test-Path (Join-Path $ScriptDir "qt_frontend\CMakeLists.txt")) -and (Test-Path (Join-Path $ScriptDir "qt_frontend\CMakePresets.json"))) {
    $FrontendDir = Join-Path $ScriptDir "qt_frontend"
} else {
    Write-Error "Could not locate the Qt frontend directory from script location: $ScriptDir"
    exit 1
}

$QtPath = "C:/Qt/6.10.1/msvc2022_64"
$VsPath = "C:\Program Files\Microsoft Visual Studio\18\Community"
$VcVarsAll = "$VsPath\VC\Auxiliary\Build\vcvars64.bat"

Write-Host "2 Verifying prerequisites..."
# Verify prerequisites
if (-not (Test-Path $QtPath)) {
    Write-Error "Qt not found at: $QtPath"
    exit 1
}

if (-not (Test-Path $VcVarsAll)) {
    Write-Error "VS 2025 vcvars64.bat not found at: $VcVarsAll"
    exit 1
}

# Build directory based on preset. Resolve the source path before comparing it
# with CMake's cached absolute path so relocated workspaces configure cleanly.
$FrontendDir = (Resolve-Path -LiteralPath $FrontendDir).Path
$BuildDir = Join-Path $FrontendDir "build\$($Preset.Substring(0,1).ToUpper() + $Preset.Substring(1))"
$CacheFile = Join-Path $BuildDir "CMakeCache.txt"
$RecreatedBuildDir = $false

if ((-not $Clean) -and (Test-Path -LiteralPath $CacheFile)) {
    $CachedSourceEntry = Select-String -LiteralPath $CacheFile -Pattern '^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$' | Select-Object -First 1
    if ($CachedSourceEntry) {
        $CachedSourceDir = [System.IO.Path]::GetFullPath($CachedSourceEntry.Matches[0].Groups[1].Value).TrimEnd('\', '/')
        $CurrentSourceDir = [System.IO.Path]::GetFullPath($FrontendDir).TrimEnd('\', '/')
        if (-not [string]::Equals($CachedSourceDir, $CurrentSourceDir, [System.StringComparison]::OrdinalIgnoreCase)) {
            Write-Host "3 CMake cache belongs to a different workspace; recreating preset build directory..."
            Write-Host "  Cached source: $CachedSourceDir"
            Write-Host "  Current source: $CurrentSourceDir"
            Remove-Item -LiteralPath $BuildDir -Recurse -Force
            $RecreatedBuildDir = $true
        }
    }
}

if ($Clean) {
    Write-Host "3 Cleaning preset build directory..."
    if (Test-Path $BuildDir) {
        Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    }
} elseif (-not $RecreatedBuildDir) {
    Write-Host "3 Preserving existing build directory and QML tooling files..."
}

Write-Host "4 Ensuring build directory exists..."
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Write-Host "5 Stop any running SeamlyLayout process..."
# Stop any running instance before cleaning (exe may be locked)
Get-Process -Name "SeamlyLayout" -ErrorAction SilentlyContinue | Stop-Process -Force


Write-Host "6 Create temporary build batch file to run cmake..."
# Create a batch file that sets up VS environment and runs cmake

$TempBat = [System.IO.Path]::GetTempFileName() + ".bat"

$BatchContent = @"
@echo off
call "$VcVarsAll" >nul 2>&1
if errorlevel 1 (
    echo Failed to initialize VS 2025 x64 environment
    exit /b 1
)

cd /d "$FrontendDir"

echo.
echo === Configuring CMake ($Preset) ===
cmake --preset $Preset -DCMAKE_PREFIX_PATH="$QtPath"
if errorlevel 1 (
    echo CMake configure failed
    exit /b 1
)

echo.
echo === Building ===
cmake --build --preset $Preset
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

echo.
echo === Build successful ===
echo Executable: $BuildDir\SeamlyLayout.exe
"@

Set-Content -Path $TempBat -Value $BatchContent -Encoding ASCII

Write-Host "7 Run build batch file..."
try {
    # Run the batch file
    Write-Host "Initializing VS 2025 x64 environment and building..." -ForegroundColor Cyan
    & cmd.exe /c $TempBat
    $ExitCode = $LASTEXITCODE

    if ($ExitCode -ne 0) {
        Write-Error "Build failed with exit code $ExitCode"
        exit $ExitCode
    }

    # Always run after successful build
    $Exe = "$BuildDir\SeamlyLayout.exe"
    if (Test-Path $Exe) {
        Write-Host "`nLaunching SeamlyLayout..." -ForegroundColor Green
        & $Exe
    } else {
        Write-Error "Executable not found: $Exe"
        exit 1
    }
} catch {
    Write-Error "An error occurred during the build process: $_"
    exit 1
} finally {
    # Cleanup temp batch file
    Write-Host "8 Deleting temp build batch file..."
    if (Test-Path $TempBat) {
        Remove-Item $TempBat -Force
    }
}

Write-Host "9 Build process completed"
