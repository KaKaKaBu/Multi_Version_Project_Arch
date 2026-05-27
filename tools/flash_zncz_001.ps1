# 无 NRST 烧录 ZNCZ_001（PowerShell）
# 用法: .\tools\flash_zncz_001.ps1

$ErrorActionPreference = "Stop"

$OpenOcd = "D:\ProgramFile\openocd\bin\openocd.exe"
$Scripts = "D:\ProgramFile\openocd\share\openocd\scripts"
$Cfg     = (Join-Path $PSScriptRoot "..\stm32f103c8_blue_pill.cfg" | Resolve-Path).Path
$Elf     = (Join-Path $PSScriptRoot "..\projects\ZNCZ_001\cmake-build-debug\ZNCZ_001.elf" | Resolve-Path).Path -replace '\\','/'

if (-not (Test-Path $Elf)) {
    Write-Error "ELF not found. Build ZNCZ_001 first."
}

Write-Host "Flash: $Elf"

# WORKAREASIZE 必须在 cfg 中为 0x1000（C8 仅 20KB SRAM，0x8000 会导致写 Flash 失败）
& $OpenOcd -s $Scripts -f $Cfg `
  -c "gdb_port disabled" `
  -c "tcl_port disabled" `
  -c "init" `
  -c "reset halt" `
  -c "program $Elf verify reset" `
  -c "reset run" `
  -c "shutdown"

if ($LASTEXITCODE -ne 0) {
    Write-Host "Retry with mass erase..." -ForegroundColor Yellow
    & $OpenOcd -s $Scripts -f $Cfg `
      -c "gdb_port disabled" `
      -c "tcl_port disabled" `
      -c "init" `
      -c "stm32f1x mass_erase 0" `
      -c "reset halt" `
      -c "flash write_image erase $Elf 0" `
      -c "verify_image $Elf 0" `
      -c "reset run" `
      -c "shutdown"
}
