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
Write-Host "Requires Rust 2021, Qt 6.11.1, and Visual Studio 2025 Community Edition with C++ workload installed..."
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

# Qt kit (Task 30): SeamlyLayout builds against the same Qt release as the
# seamly2d/seamlyme parent apps — Qt 6.11.1 msvc2022_64 or newer. Rather than
# pinning one patch release (which broke the moment 6.10.1 was uninstalled),
# scan C:\Qt for msvc2022_64 kits and take the newest one that meets the
# minimum required by qt_frontend/CMakeLists.txt's find_package(Qt6 6.11.1).
$QtMinimumVersion = [version]"6.11.1"
$QtRoot = "C:\Qt"
$QtPath = $null

if (Test-Path $QtRoot) {
    # Version-sort the kit directories so 6.11.10 beats 6.11.9 (string sort would not),
    # keep only those that actually ship an msvc2022_64 kit, and take the newest.
    $QtPath = Get-ChildItem -LiteralPath $QtRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $parsed = $null
            if ([version]::TryParse($_.Name, [ref]$parsed)) {
                [pscustomobject]@{ Version = $parsed; Path = (Join-Path $_.FullName "msvc2022_64") }
            }
        } |
        Where-Object { $_.Version -ge $QtMinimumVersion -and (Test-Path $_.Path) } |
        Sort-Object Version -Descending |
        Select-Object -First 1 -ExpandProperty Path
}

$VsPath = "C:\Program Files\Microsoft Visual Studio\18\Community"
$VcVarsAll = "$VsPath\VC\Auxiliary\Build\vcvars64.bat"

Write-Host "2 Verifying prerequisites..."
# Verify prerequisites
if (-not $QtPath) {
    Write-Error "No Qt $QtMinimumVersion+ msvc2022_64 kit found under '$QtRoot' - install Qt 6.11.1 (msvc2022_64) first."
    exit 1
}

# CMake wants forward slashes in CMAKE_PREFIX_PATH
$QtPath = $QtPath -replace '\\', '/'
Write-Host "  Qt kit: $QtPath"

# ---------------------------------------------------------------------------
# Task 47: pin the Qt that Cargo/cxx-qt-build sees to the kit selected above.
#
# CMake is told which Qt to use via -DCMAKE_PREFIX_PATH, but the Rust half of
# the build is not: cxx-qt-build locates Qt through the QMAKE environment
# variable, falling back to whatever bare `qmake` is first on PATH. On a
# machine with Qt Design Studio installed that fallback is
# C:\Qt\Tools\QtDesignStudio\qt6_design_studio_reduced_version\bin\qmake.exe —
# a stripped Qt with NO mkspecs directory — so the build fails naming that path
# instead of the real kit's mkspecs. Exporting QMAKE and putting the kit's bin\
# first on PATH makes the fallback unreachable, for this script and for any
# `cargo build` / `cargo test` run in the shell it spawns.
# ---------------------------------------------------------------------------
$QtBin   = (Join-Path ($QtPath -replace '/', '\') "bin")
$QtQmake = Join-Path $QtBin "qmake.exe"

if (-not (Test-Path $QtQmake)) {
    Write-Error "qmake.exe not found in the selected kit: $QtQmake"
    exit 1
}

# Guard against a Qt whose prefix ships no mkspecs (the Design Studio reduced
# Qt is exactly this). Failing here names the real problem; letting the build
# proceed produces a confusing "could not find qmake spec" pointing elsewhere.
$QtPrefix = (& $QtQmake -query QT_INSTALL_PREFIX) -replace '/', '\'
if (-not (Test-Path (Join-Path $QtPrefix "mkspecs"))) {
    Write-Error "Qt kit at '$QtPrefix' has no mkspecs directory - it is not a full Qt installation (a Qt Design Studio reduced Qt looks like this). Install/select a full msvc2022_64 kit."
    exit 1
}

$env:QMAKE = $QtQmake
$env:PATH  = "$QtBin;$env:PATH"
Write-Host "  qmake : $QtQmake (QMAKE exported, kit bin prepended to PATH)"

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
