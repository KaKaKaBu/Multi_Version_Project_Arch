param(
    [Parameter(Mandatory = $true)]
    [string]$Text,

    [Parameter(Mandatory = $true)]
    [string]$Output,

    [string]$FontName = 'SimHei',
    [int]$Width = 12,
    [int]$Height = 12,
    [int]$FontSize = 12,
    [int]$XOffset = 0,
    [int]$YOffset = 0,
    [int]$CanvasPadding = 8,
    [int]$TopMargin = 0,
    [int]$LeftMargin = 0,
    [switch]$DisableAutoCrop,
    [string]$SymbolName = 'project_oled_font'
)

Add-Type -AssemblyName System.Drawing

$chars = New-Object System.Collections.Generic.SortedSet[int]
foreach ($ch in $Text.ToCharArray()) {
    $cp = [int][char]$ch
    if ($cp -ge 128) {
        [void]$chars.Add($cp)
    }
}

$bytesPerRow = [int][Math]::Ceiling($Width / 8.0)
$glyphLines = New-Object System.Collections.Generic.List[string]
$entryLines = New-Object System.Collections.Generic.List[string]
$font = New-Object System.Drawing.Font($FontName, $FontSize, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)

foreach ($cp in $chars) {
    $ch = [string][char]$cp
    $bitmap = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Black)
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit

    if ($DisableAutoCrop.IsPresent) {
        $graphics.DrawString($ch, $font, [System.Drawing.Brushes]::White, $XOffset, $YOffset)
    } else {
        $graphics.Dispose()
        $graphics = $null

        $sourceWidth = [int][Math]::Max($Width * 3, $Width + ($CanvasPadding * 2) + $FontSize)
        $sourceHeight = [int][Math]::Max($Height * 3, $Height + ($CanvasPadding * 2) + $FontSize)
        $sourceBitmap = New-Object System.Drawing.Bitmap($sourceWidth, $sourceHeight, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $sourceGraphics = [System.Drawing.Graphics]::FromImage($sourceBitmap)
        $sourceGraphics.Clear([System.Drawing.Color]::Black)
        $sourceGraphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
        $sourceGraphics.DrawString($ch, $font, [System.Drawing.Brushes]::White, $CanvasPadding, $CanvasPadding)

        $minX = $sourceWidth
        $minY = $sourceHeight
        $maxX = -1
        $maxY = -1
        for ($sy = 0; $sy -lt $sourceHeight; $sy++) {
            for ($sx = 0; $sx -lt $sourceWidth; $sx++) {
                $pixel = $sourceBitmap.GetPixel($sx, $sy)
                if ($pixel.R -gt 0 -or $pixel.G -gt 0 -or $pixel.B -gt 0) {
                    if ($sx -lt $minX) { $minX = $sx }
                    if ($sy -lt $minY) { $minY = $sy }
                    if ($sx -gt $maxX) { $maxX = $sx }
                    if ($sy -gt $maxY) { $maxY = $sy }
                }
            }
        }

        if (($maxX -ge $minX) -and ($maxY -ge $minY)) {
            $glyphWidth = $maxX - $minX + 1
            $glyphHeight = $maxY - $minY + 1
            if ($glyphWidth -ge $Width) {
                $destX = 0
            } else {
                $destX = [int][Math]::Floor(($Width - $glyphWidth) / 2.0) + $LeftMargin + $XOffset
            }
            $destY = $TopMargin + $YOffset
            if (($destY + $glyphHeight) -gt $Height) {
                $destY = [int][Math]::Max(0, $Height - $glyphHeight)
            }
            if ($destY -lt 0) {
                $destY = 0
            }

            for ($sy = $minY; $sy -le $maxY; $sy++) {
                $ty = $destY + ($sy - $minY)
                if ($ty -lt 0 -or $ty -ge $Height) {
                    continue
                }
                for ($sx = $minX; $sx -le $maxX; $sx++) {
                    $tx = $destX + ($sx - $minX)
                    if ($tx -lt 0 -or $tx -ge $Width) {
                        continue
                    }
                    $pixel = $sourceBitmap.GetPixel($sx, $sy)
                    if ($pixel.R -gt 0 -or $pixel.G -gt 0 -or $pixel.B -gt 0) {
                        $bitmap.SetPixel($tx, $ty, [System.Drawing.Color]::White)
                    }
                }
            }
        }

        $sourceGraphics.Dispose()
        $sourceBitmap.Dispose()
    }

    $values = New-Object System.Collections.Generic.List[string]
    for ($y = 0; $y -lt $Height; $y++) {
        for ($byteIndex = 0; $byteIndex -lt $bytesPerRow; $byteIndex++) {
            $value = 0
            for ($bit = 0; $bit -lt 8; $bit++) {
                $x = $byteIndex * 8 + $bit
                if ($x -lt $Width) {
                    $pixel = $bitmap.GetPixel($x, $y)
                    if ($pixel.R -gt 0 -or $pixel.G -gt 0 -or $pixel.B -gt 0) {
                        $value = $value -bor (0x80 -shr $bit)
                    }
                }
            }
            $values.Add(('0x{0:X2}' -f $value))
        }
    }

    $arrayName = ('{0}_u{1:X4}' -f $SymbolName, $cp)
    $glyphLines.Add(('static const uint8_t {0}[] = {{ {1} }};' -f $arrayName, ($values -join ', ')))
    $entryLines.Add(('    {{ 0x{0:X4}U, {1}U, {2}U, {3} }}, /* {4} */' -f $cp, $Width, $Height, $arrayName, $ch))

    if ($graphics -ne $null) {
        $graphics.Dispose()
    }
    $bitmap.Dispose()
}

$count = $chars.Count
$content = @"
/* Auto-generated by tools/gen_oled_custom_font.ps1. Do not edit manually. */
#ifndef KQZL2_OLED_FONT_H
#define KQZL2_OLED_FONT_H

#include "display_font.h"

$(($glyphLines -join "`n"))

static const display_glyph_t ${SymbolName}_glyphs[] = {
$(($entryLines -join "`n"))
};

static const display_font_t ${SymbolName} = {
    ${Width}U,
    ${Height}U,
    ${bytesPerRow}U,
    0U,
    0U,
    0,
    ${SymbolName}_glyphs,
    ${count}U
};

#endif
"@

$outputPath = [System.IO.Path]::GetFullPath($Output)
[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($outputPath)) | Out-Null
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($outputPath, $content, $utf8NoBom)
Write-Output "Generated $outputPath ($count glyphs, ${Width}x${Height}, font=$FontName)"
