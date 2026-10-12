param(
    [string]$ConfigFile,
    [string]$BuildPath = (Join-Path ([System.IO.Path]::GetTempPath()) 'DeskBuddy-build'),
    [ValidateRange(1, 8)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$taskCliArgs = @()
if ($ConfigFile) { $taskCliArgs += @('--config-file', (Resolve-Path -LiteralPath $ConfigFile).Path) }
$taskBoard = 'esp32:esp32:esp32:CPUFreq=240,FlashFreq=40,FlashMode=dio,FlashSize=4M,PartitionScheme=min_spiffs,PSRAM=disabled,UploadSpeed=115200'
$taskSketch = Join-Path $PSScriptRoot 'DeskBuddy'
$taskDisplayConfig = (Join-Path $taskSketch 'DisplayConfig.h').Replace('\', '/')
$taskFirmware = Join-Path $PSScriptRoot 'firmware'
$taskFlags = 'compiler.cpp.extra_flags=-include "' + $taskDisplayConfig + '"'
& arduino-cli @taskCliArgs compile --jobs $Jobs --fqbn $taskBoard --build-path $BuildPath --output-dir $taskFirmware --build-property $taskFlags $taskSketch
if ($LASTEXITCODE -ne 0) { throw 'DeskBuddy compilation failed.' }
