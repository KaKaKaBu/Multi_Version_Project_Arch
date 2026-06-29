/* driver_all_test 固件入口：仅初始化 BSP、设备管理和调度器，用于验证驱动可链接。 */
#include "devmgr.h"
#include "hal_common.h"
#include "scheduler.h"
#include "version_config.h"

void app_main(void)
{
    bsp_init();
    sched_init();
    devmgr_init_all();
    sched_start();
}

int main(void)
{
    app_main();
    for (;;) {
    }
}
