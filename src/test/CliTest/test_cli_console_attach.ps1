# @file   test_cli_console_attach.ps1
# @author slspencer
# @brief  Runs Seamly2D in a console without redirection and saves what the console shows.
# @copyright Copyright (C) 2026 Seamly2D Project
# @license GPL-3.0-or-later
#
# The runner starts this script in a new, hidden console. Seamly2D (a GUI-subsystem
# program) gets no standard handles, so its text reaches the console only through
# AttachConsole. After Seamly2D exits, the script reads the console screen buffer
# and writes the text and the exit code to -ResultFile as JSON.

param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$ArgumentFile,
    [Parameter(Mandatory = $true)][string]$ResultFile
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class ConsoleScreenReader
{
    [StructLayout(LayoutKind.Sequential)] struct Coord { public short X; public short Y; }
    [StructLayout(LayoutKind.Sequential)] struct SmallRect { public short Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] struct BufferInfo
    {
        public Coord Size; public Coord Cursor; public short Attributes; public SmallRect Window; public Coord MaximumSize;
    }

    [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr CreateFileW(
        [MarshalAs(UnmanagedType.LPWStr)] string name, uint access, uint share, IntPtr security, uint creation,
        uint flags, IntPtr template);
    [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetConsoleScreenBufferInfo(IntPtr output, out BufferInfo info);
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)] static extern bool ReadConsoleOutputCharacterW(
        IntPtr output, StringBuilder text, uint length, Coord start, out uint read);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);

    // Text of every row from the top of the buffer to the cursor row.
    public static string Read()
    {
        IntPtr output = CreateFileW("CONOUT$", 0xC0000000, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);
        BufferInfo info;
        if (!GetConsoleScreenBufferInfo(output, out info)) { CloseHandle(output); return ""; }
        StringBuilder all = new StringBuilder();
        for (short row = 0; row <= info.Cursor.Y; row++)
        {
            StringBuilder line = new StringBuilder(info.Size.X);
            uint read;
            ReadConsoleOutputCharacterW(output, line, (uint)info.Size.X, new Coord { X = 0, Y = row }, out read);
            all.AppendLine(line.ToString(0, (int)read).TrimEnd());
        }
        CloseHandle(output);
        return all.ToString();
    }
}
'@

# The argument list arrives as JSON so that no shell parses it on the way.
# Windows PowerShell passes a JSON array on as one object; the pipe below enumerates it.
$parsed = Get-Content -LiteralPath $ArgumentFile -Raw -Encoding UTF8 | ConvertFrom-Json
$arguments = @($parsed | ForEach-Object { [string]$_ })
. (Join-Path $PSScriptRoot 'test_cli_output_checks.ps1')
$info = New-Object System.Diagnostics.ProcessStartInfo
$info.FileName = $Executable
$info.Arguments = ($arguments | ForEach-Object { Format-WindowsArgument $_ }) -join ' '
$info.UseShellExecute = $false
$process = [System.Diagnostics.Process]::Start($info)
$finished = $process.WaitForExit(300000)
if (-not $finished) { $process.Kill() }
$process.WaitForExit()
# The attached process writes asynchronously to the shared console; give it a moment to land.
Start-Sleep -Milliseconds 500
[pscustomobject]@{
    ExitCode = if ($finished) { $process.ExitCode } else { -1 }
    Console  = [ConsoleScreenReader]::Read()
} | ConvertTo-Json | Set-Content -LiteralPath $ResultFile -Encoding UTF8
