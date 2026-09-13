<#
 **  @file   generate_wix_images.ps1
 **  @author slspencer
 **  @date   2026-09-12
 **
 **  @brief  One-time generator for the WiX banner and dialog bitmaps, built
 **          from the Seamly logo. Not part of the install-and-test loop;
 **          run again only when the source logo or target sizes change.
 **
 **  WiX's stock WixUI_Bmp_Banner is 493x58 px and appears behind the title
 **  bar of every Seamly wizard page (BannerBitmap control, smsi_ui.wxs).
 **  WixUI_Bmp_Dialog is 493x312 px and is the full-page background of
 **  WelcomeDlg and ExitDialog. Both are stock DialogRefs (smsi_ui.wxs)
 **  whose Title/Description text sits at dialog-unit X=135 (~180px here),
 **  so the left 180px of the dialog bitmap must stay clear of text and the
 **  right 313px must stay light enough for dark text to read.
 **
 **  @copyright
 **  This source code is part of the Seamly project, a suite of apparel CAD
 **  software.
 **  Copyright (C) 2026 Seamly2D Project
 **  All Rights Reserved.
 **
 **  SeamlyLayout is licensed under the MIT license.
#>

Add-Type -AssemblyName System.Drawing

$assetsDir = $PSScriptRoot
$logoPath  = Join-Path $assetsDir '..\..\src\libs\vmisc\share\resources\icon\logos\seamly_logo_192.png'
$logoPath  = (Resolve-Path $logoPath).Path
$logo      = [System.Drawing.Image]::FromFile($logoPath)

function New-WixBitmap {
    param(
        [int]$Width,
        [int]$Height,
        [int]$LogoSize,
        [int]$LogoCenterX,
        [string]$OutFile
    )

    $bmp = New-Object System.Drawing.Bitmap $Width, $Height, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $g   = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::White)

    $destX = $LogoCenterX - [int]($LogoSize / 2)
    $destY = [int](($Height - $LogoSize) / 2)
    $destRect = New-Object System.Drawing.Rectangle $destX, $destY, $LogoSize, $LogoSize
    $g.DrawImage($logo, $destRect, 0, 0, $logo.Width, $logo.Height, [System.Drawing.GraphicsUnit]::Pixel)

    $g.Dispose()
    $bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmp.Dispose()
}

# Banner: logo sits in the right margin; left ~350px stays clear for the
# Title/Description text every Seamly dialog draws at X=15/25.
New-WixBitmap -Width 493 -Height 58 -LogoSize 40 -LogoCenterX 463 `
    -OutFile (Join-Path $assetsDir 'wix_banner.bmp')

# Dialog: logo centered in the left 180px column, clear of the stock
# WelcomeDlg/ExitDialog text that starts at dialog-unit X=135 (~180px).
New-WixBitmap -Width 493 -Height 312 -LogoSize 150 -LogoCenterX 90 `
    -OutFile (Join-Path $assetsDir 'wix_dialog.bmp')

$logo.Dispose()

Write-Host "Wrote wix_banner.bmp and wix_dialog.bmp to $assetsDir"
