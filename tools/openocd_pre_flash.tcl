# OpenOCD 烧录辅助脚本（无 NRST / SYSRESETREQ 等效流程）
#
# 由 tools/flash_zncz_001.ps1 或 CLion 调用:
#   openocd -s <scripts> -f stm32f103c8_blue_pill.cfg -f tools/openocd_flash_zncz.tcl

init
reset halt

# 软件复位（AIRCR SYSRESETREQ），与 Keil 选项一致；halt 后再写 Flash
mww 0xE000ED0C 0x05FA0004
sleep 50
reset halt
