<#
 ******************************************************************************
 **  @file   installer_running_apps_test.ps1
 **  @author slspencer
 **
 **  @brief
 **  Tests the running-app custom actions inside the built MSI.
 **
 **  Opens the package without installing it, starts a stand-in process named
 **  seamlyme.exe, and runs SeamlyFindRunningApps and SeamlyCloseRunningApps
 **  against it. smsi.ps1 runs this test after every MSI build.
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  All Rights Reserved.
 **
 **  @license
 **  GPL-3.0-or-later
 ******************************************************************************
#>

[CmdletBinding()]
param(
    # The built MSI.
    [Parameter(Mandatory = $true)]
    [string]$Msi,

    # Scratch directory for the stand-in executable.
    [Parameter(Mandatory = $true)]
    [string]$WorkDirectory
)

$ErrorActionPreference = 'Stop'
$script:failures = @()

<#
.SYNOPSIS
    Records one test result.
#>
function Assert-That {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][bool]$Succeeded,
        [string]$Detail = ''
    )
    if ($Succeeded) {
        Write-Host "  ok      $Name"
    } else {
        Write-Host "  FAILED  $Name$(if ($Detail) { " - $Detail" })"
        $script:failures += $Name
    }
}

<#
.SYNOPSIS
    Runs one custom action in the open session and returns its status.
.OUTPUTS
    The msiDoActionStatus value: 1 is success.
#>
function Invoke-SessionAction {
    param([string]$Action)
    return [int]$script:session.GetType().InvokeMember('DoAction', 'InvokeMethod', $null, $script:session, @($Action))
}

<#
.SYNOPSIS
    Reads one property from the open session.
#>
function Get-SessionProperty {
    param([string]$Name)
    return [string]$script:session.GetType().InvokeMember('Property', 'GetProperty', $null, $script:session, @($Name))
}

# Real Seamly apps can run while this test runs. The find checks expect them,
# and the close check is skipped, because it would close them too.
$appProperties = [ordered]@{ seamly2d = 'SEAMLYRUNNING2D'; seamlyme = 'SEAMLYRUNNINGME'; seamlylayout = 'SEAMLYRUNNINGLAYOUT' }
$realApps = @(Get-Process -Name @($appProperties.Keys) -ErrorAction SilentlyContinue |
    ForEach-Object { $_.ProcessName.ToLowerInvariant() } | Sort-Object -Unique)

<#
.SYNOPSIS
    Checks that each running-app property matches the apps expected to run.
#>
function Assert-RunningProperties {
    param([string]$Name, [string[]]$Running)
    $wrong = @($appProperties.Keys | Where-Object {
        (Get-SessionProperty -Name $appProperties[$_]) -ne $(if ($Running -contains $_) { '1' } else { '' }) })
    $anyExpected = if ($Running.Count -gt 0) { '1' } else { '' }
    Assert-That -Name $Name `
        -Succeeded ($wrong.Count -eq 0 -and (Get-SessionProperty -Name 'SEAMLYRUNNINGAPPS') -eq $anyExpected) `
        -Detail "expected running: $(if ($Running.Count) { $Running -join ', ' } else { 'none' }); wrong: $($wrong -join ', ')"
}

# Build the stand-in straight to a Seamly executable name.
$standIn = Join-Path $WorkDirectory 'seamlyme.exe'
& cl.exe /nologo /W4 /WX /DUNICODE /D_UNICODE `
    "/Fo$(Join-Path $WorkDirectory 'installer_running_apps_test_window.obj')" "/Fe$standIn" `
    (Join-Path $PSScriptRoot 'installer_running_apps_test_window.cpp') `
    /link /NOLOGO /SUBSYSTEM:WINDOWS user32.lib | ForEach-Object { "$_" }
if ($LASTEXITCODE -ne 0) {
    throw "cl (stand-in seamlyme.exe) failed (exit code $LASTEXITCODE) - see output above."
}

$installer = New-Object -ComObject WindowsInstaller.Installer
# 2 = msiUILevelNone. OpenPackage flags must be 0: the IgnoreMachineState flag
# opens a restricted engine, which refuses to run DLL custom actions.
$installer.GetType().InvokeMember('UILevel', 'SetProperty', $null, $installer, @(2)) | Out-Null
$script:session = $installer.GetType().InvokeMember('OpenPackage', 'InvokeMethod', $null, $installer,
    @((Resolve-Path $Msi).Path, 0))
$process = $null

try {
    Write-Host "checking the running-app custom actions in $Msi"

    $status = Invoke-SessionAction -Action 'SeamlyFindRunningApps'
    Assert-That -Name 'the find action runs' -Succeeded ($status -eq 1) -Detail "status $status"
    Assert-RunningProperties -Name 'the find action reports the apps already running' -Running $realApps

    $process = Start-Process -FilePath $standIn -PassThru
    $process.WaitForInputIdle(10000) | Out-Null

    Invoke-SessionAction -Action 'SeamlyFindRunningApps' | Out-Null
    Assert-RunningProperties -Name 'the find action reports a started seamlyme.exe' `
        -Running (@($realApps) + 'seamlyme' | Sort-Object -Unique)

    if ($realApps.Count -gt 0) {
        Write-Host "  SKIPPED the close check: close $($realApps -join ', ') and build again to run it."
    } else {
        $status = Invoke-SessionAction -Action 'SeamlyCloseRunningApps'
        Assert-That -Name 'the close action runs' -Succeeded ($status -eq 1) -Detail "status $status"
        Assert-That -Name 'the close action closes the app through its window' -Succeeded ($process.HasExited)
        Assert-RunningProperties -Name 'the close action clears the running-app properties' -Running @()
    }
} finally {
    if ($process -and -not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($script:session) | Out-Null
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($installer) | Out-Null
}

Write-Host ''
if ($script:failures.Count -gt 0) {
    Write-Host "running-app custom action check FAILED - $($script:failures.Count) problem(s):"
    $script:failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
Write-Host 'running-app custom action check passed.'
exit 0
