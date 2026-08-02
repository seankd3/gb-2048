#requires -Version 5
<#
  Builds gb-2048 into build/gb2048.gbc.

  Cartridge header:
    -Wm-yC        CGB only (the Chromatic and any GBC run this; a DMG will not)
    -Wm-yt0x1B    MBC5 + RAM + BATTERY, so high scores survive a power cycle
    -Wm-ya1       one bank of save RAM
#>
param(
    [switch]$Clean,
    [string]$Gbdk = "$env:USERPROFILE\Tools\gbdk"
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$out  = Join-Path $root 'build\gb2048.gbc'
$lcc  = Join-Path $Gbdk 'bin\lcc.exe'

if (-not (Test-Path $lcc)) { throw "GBDK not found at $Gbdk" }

if ($Clean) {
    Get-ChildItem $root -Recurse -Include *.o,*.lst,*.sym,*.map,*.rel,*.ihx,*.asm,*.cdb,*.adb,*.noi |
        Remove-Item -Force -ErrorAction SilentlyContinue
}

python (Join-Path $root 'tools\gen_assets.py')
if ($LASTEXITCODE -ne 0) { throw 'asset generation failed' }

New-Item -ItemType Directory -Force (Join-Path $root 'build') | Out-Null

$sources = Get-ChildItem (Join-Path $root 'src\*.c') | ForEach-Object { $_.FullName }
$args = @('-Wm-yC', '-Wm-yn2048', '-Wm-yt0x1B', '-Wm-ya1', '-o', $out) + $sources

& $lcc @args
if ($LASTEXITCODE -ne 0) { throw "compile failed (exit $LASTEXITCODE)" }

$size = (Get-Item $out).Length
"built {0}  ({1:N0} bytes)" -f $out, $size
